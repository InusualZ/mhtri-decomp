/*
 * enemy/em016_prog.cpp - enemy 016's program: the per-map/area step tables and their dispatchers, the
 *   multi-motion action band (`fn_8018B3C8`/`fn_8018D250` on `state_sub`, `fn_8018D2B0` on `action`, the
 *   per-(team, motion) frame-window driver `fn_8018D8C8` and its effect router `fn_8018D558`) and the aim/action
 *   group of the `ResUserDataAc` class set.
 * RANGE. .text 0x80182C40-0x80192348 (155 functions); extab 0x8000E9E4-0x8000EDC4, extabindex
 *   0x80029DC0-0x8002A390, .ctors 0x8056F33C-0x8056F340, .rodata 0x8056FF50-0x80570050, .data 0x805AA930-0x805AD370
 *   (`em016_prog_tbl` first; the class tables at 0x805AA960/0x805AA9CC/0x805AAA38 hold this range's entry points),
 *   .bss 0x806A7A00-0x806A7A6C, .sdata 0x80791A60-0x80791A78, .sbss 0x80794AA0-0x80794AA8, .sdata2
 *   0x80797E88-0x80798238.
 * FLAGS. `cflags_main`; `#pragma peephole off` from `fn_80191598` to the end (the peephole fuses the `extsh` +
 *   `cmpwi` of a 16-bit test into `extsh.`), on before it.
 * NAMES. The file name follows the runtime dump's `em016_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions.  Callees whose call sites disagree with the owner's header are called through
 *   `<name>_viewN`/`<name>_cN` cast macros (the same direct call), and `#define stage_map_kind_get
 *   stage_map_kind_get_hidden_<header>` hides the disagreeing declaration.  `EmActWork` is this unit's own view of
 *   the work record (0xB18 apart): it reads +0x328..+0x33C as fixed-point angle words and +0x344..+0x34C as floats.
 *   The `.bss` record names (`vec_pair_80191598_*`, `vec_default_80191598`) are GUESSes.
 *   `EmActWork` and its records live in `enemy/em016_prog_types.h`, shared with `enemy/em018_prog.cpp`.
 * RESIDUALS. 66 rows unwritten: 0x80183040-0x80183440, 0x8018347C-0x80183A54, 0x80184D98-0x801850F8,
 *   0x80185218-0x801856B8, 0x80185938-0x80185B0C, 0x80185C6C-0x80185D60, 0x80185DFC-0x80185FA0,
 *   0x80186020-0x8018B3B8.  `fn_80183040` builds the 0xC-byte helper through `__nw__FUl` + `fn_80183440` and
 *   writes the 0x328..0x35F block; its stores need the `lis r3,1; subi r0,r3,imm` constant shape first.
 *   25 partial rows, including:
 *  - `fn_80182D5C`, `fn_8018484C`, `fn_8018493C`, `fn_80185D60`, `fn_8018D370`: retail evaluates the float
 *    argument of `em_motion_param_set`/`em_approach_start`/`em_water_check`/`eft009_spawn_at_joint` before the
 *    integer one; MWCC follows the declaration's `(self, s32, f32)` order;
 *  - `fn_80183E9C`, `fn_8018493C`: retail keeps `subi r0,rX,1; stw; cmpwi r0,0; bgt`, every spelling tried fuses
 *    `subic.` (4 bytes short);
 *  - `fn_80185D60`: retail truncates the selected motion id at the call (`clrlwi r4,r4,16`), MWCC folds the
 *    `(u16)` of a two-constant conditional away;
 *  - `fn_80183440`: retail materialises `lbl_805AD340` into r0 for the table store, ours into r3;
 *  - `fn_801841B4`, `fn_801842FC`: every opcode and operand pairs; the remaining row is a branch target;
 *  - `fn_801850F8`: retail restores f31 with `li r0,0x18; psq_lx`, ours with `psq_l f31,0x18(r1)`;
 *  - `fn_8018B418`, `fn_8018C5CC`, `fn_8018CE84`, `fn_8018D558`: a few rows on the shared `fn_8018D558` call tail
 *    and a `b` that is a fall-through in retail (or the reverse);
 *  - `fn_80191038`: the float-first argument order, and retail stores the `_GXColor` bytes b, g, r, a (its frame
 *    is 52 bytes smaller);
 *  - `fn_801913FC`: retail keeps `(lbl_80797ED0 + fn_8013026C(self)) * scale` as `fmuls` + `fsubs`, ours fuses
 *    `fnmsubs`;
 *  - `fn_80191E30`: the item and work pointers sit in r31/r30 swapped, and ours falls into the next case's `li`
 *    where retail's three inner cases branch to one continuation (two more `b`, 12 bytes);
 *  - `fn_80191598`: one instruction (4 bytes) long; `fn_80191B4C`: one callee-saved register coloured differently.
 *   The other 6 partial rows have no recorded cause.
 *   flipcheck: `.ctors`/`.rodata`/`.sbss`/`.sdata`/`.sdata2` claimed, not emitted; `.data` 0x544 against 0x2A40;
 *   `.text` (0x8B74 of 0xF708), extab (0x240 of 0x3E0) and extabindex (0x360 of 0x5D0) short of the claim and
 *   differing.
 * SHAPES. A one-case `switch (x) { case 3: ... }` gives the signed `cmpwi` where `if (x == 3)` gives `cmplwi`
 *   (`fn_80191990`, `fn_80191AE8`, `fn_80191B4C`, `fn_80191CE4`, `fn_80191E30`).  `fn_80191CE4`'s frame holds a
 *   0x24-byte 3x3 matrix (`EmMtx33`) at +0x14, not a second `MTX34` (that frame is 0x90 against retail's 0x80).
 *   Naming the float in an `f32` local gives the float-first order for `fn_80182D5C`'s first call.
 *   `fn_801846BC` passes the motion through a `u32` local, which keeps the `(u16)` truncation.
 */

#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "ef/fn_80117E58.h" /* fn_80117E58 (rule 2: the owner's header) */
#include "enemy/em_action_finish.h" /* em_action_finish (rule 2: the owner's header) */
#include "enemy/em_hit_window_set_default.h" /* em_hit_window_set_default (rule 2: the owner's header) */
#include "enemy/em_busy_set.h" /* em_busy_set (rule 2: the owner's header) */
#include "enemy/em_alt_mode_ck.h" /* em_alt_mode_ck (rule 2: the owner's header) */
#include "enemy/fn_8012EC3C.h" /* fn_8012EC3C (rule 2: the owner's header) */
#include "enemy/em_mot_set_blend.h" /* em_mot_set_blend (rule 2: the owner's header) */
#include "enemy/em_mot_set.h" /* em_mot_set (rule 2: the owner's header) */
#include "enemy/em_mot_speed_set.h" /* em_mot_speed_set (rule 2: the owner's header) */
#include "enemy/em_mot_end_ck.h" /* em_mot_end_ck (rule 2: the owner's header) */
#include "enemy/fn_8013026C.h" /* fn_8013026C (rule 2: the owner's header) */
#include "enemy/em_move_mode_set.h" /* em_move_mode_set (rule 2: the owner's header) */
#include "enemy/em_frame_flag_set.h" /* em_frame_flag_set (rule 2: the owner's header) */
#include "enemy/em_busy_timer_reset.h" /* em_busy_timer_reset (rule 2: the owner's header) */
#include "enemy/fn_801321DC.h" /* fn_801321DC (rule 2: the owner's header) */
#include "enemy/fn_80133C3C.h" /* fn_80133C3C (rule 2: the owner's header) */
#include "enemy/fn_80133DB0.h" /* fn_80133DB0 (rule 2: the owner's header) */
#include "enemy/em_turn_in_window.h" /* em_turn_in_window (rule 2: the owner's header) */
#include "enemy/em_flags836_ck.h" /* em_flags836_ck (rule 2: the owner's header) */
#include "enemy/em_camera_req.h" /* em_camera_req (rule 2: the owner's header) */
#include "enemy/em_shake_req_set.h" /* fn_80136D14 (rule 2: the owner's header) */
#include "enemy/fn_80129D3C.h" /* fn_80129D3C (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"       /* MHchar, with the pointer-taking setTevKColor overload */
#include "enemy/ENEMY_WORK.h"
#define fn_8012EC3C fn_8012EC3C_hidden_fn_8012E968_h
#define em_alt_mode_ck em_alt_mode_ck_hidden_fn_8012E968_h
#include "unsplit/enemy.h"
#undef em_alt_mode_ck
#undef fn_8012EC3C
#include "unsplit/unknown.h"   /* system_w */
#include "enemy/fn_801251D0.h" /* fn_80126278/fn_80126324 + the 0x80129xxx helpers */
#include "enemy/fn_8012EC74.h" /* fn_8013026C */
#include "enemy/fn_8012BDF4.h" /* fn_8012E5A8 */
#include "ef.h"
#include "enemy/fn_80138074.h" /* fn_8013A654/fn_8013918C + the EmUserData record */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "fn_8004CAD8.h"       /* fn_8005024C/fn_80051378/rotVecY/calcDistanceSqXZ */
#include "mh3_pad.h"           /* VEC3_ctor/copyVec3/setVec3 */
#include "sys_mem.h"           /* operator delete (the `__dl__FPv` global deleter) */
#define stage_map_kind_get stage_map_kind_get_hidden_stg_w_h
#include "stage/stg_w.h"
#undef stage_map_kind_get
#include "enemy/fn_801251D0.h"
#include "ef.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80147CE0.h"
#include "fn_8004CAD8.h"
#include "ef/fn_80105314.h"
#include "mh3_pad.h"
#include "sys_mem.h"
#include "enemy/fn_8012EC74.h" /* fn_8013026C/fn_8012FE3C/fn_801337FC/fn_80136D4C */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */
#include "enemy/em016_prog_types.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_80117E58_c1 ((void (*)(_ENEMY_WORK*, u32, nw4r::math::VEC3*, u32, f32))fn_80117E58)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_alt_mode_ck_c1 ((u32 (*)(_ENEMY_WORK*))em_alt_mode_ck)
#define fn_8012EC3C_c1 ((u32 (*)(_ENEMY_WORK*))fn_8012EC3C)
#define fn_80129D3C_c1 ((u32 (*)(EmActWork*))fn_80129D3C)
#define fn_80182D5C_noarg ((void (*)(void))fn_80182D5C)
#define stage_map_kind_get_view3 ((u8 (*)(u8))stage_map_kind_get)
#define stage_map_kind_get_view1 ((u8 (*)(u8))stage_map_kind_get)
#define fn_80192080_view1 ((void (*)(_ENEMY_WORK*))fn_80192080)
#define fn_80191EF8_view1 ((void (*)(_ENEMY_WORK*))fn_80191EF8)
#define fn_80182D5C_view1 ((void (*)(void))fn_80182D5C)
#define fn_8013A654_view1 ((void (*)(EmUserData*, u32))fn_8013A654)
#define fn_8013918C_view1 ((void (*)(void*, s16))fn_8013918C)
#define fn_801321DC_view1 ((u32 (*)(EmActWork*))fn_801321DC)
#define fn_8012EC3C_view1 ((u32 (*)(EmActWork*))fn_8012EC3C)
#define fn_8012A204_view1 ((s32 (*)(EmActWork*))fn_8012A204)
#define fn_8012A014_view1 ((u32 (*)(EmActWork*, u32, u32, u16, u32, u8*))fn_8012A014)
#define fn_80129DB8_view1 ((u8 (*)(EmActWork*))fn_80129DB8)
#define fn_80129A70_view1 ((u32 (*)(EmActWork*, u16))fn_80129A70)
#define fn_80126324_view1 ((void (*)(EmActWork*, u8, u8, f32))em_move_target_set)
#define em_water_check_view1 ((u32 (*)(struct _ENEMY_WORK*))em_water_check)
#define em_move_mode_set_view1 ((void (*)(EmActWork*, u32))em_move_mode_set)
#define em_mot_set_view1 ((void (*)(EmActWork*, s32, s32, s32))em_mot_set)
#define em_fall_start_view1 ((void (*)(_ENEMY_WORK*, f32))em_fall_start)
#define em_alt_mode_ck_view1 ((u32 (*)(EmActWork*))em_alt_mode_ck)
#define eft_spawn_type11_view1 ((void (*)(_ENEMY_WORK*, nw4r::math::VEC3*, u32, f32))eft_spawn_type11)
#define assignVec3_view1 ((void (*)(void*, const void*))assignVec3)

extern "C" {
void fn_80182C40(_ENEMY_WORK* self, u32 kind, void* out);
void fn_80182D44(_ENEMY_WORK* self, u32 arg);

extern f32 lbl_80797E88;
extern f32 lbl_80797E8C;
extern f32 lbl_80797E90;
extern f32 lbl_80797E94;
extern f32 lbl_80797E98;

/* `stage/stg_w.cpp`'s byte-table lookup (0xFF in `lbl_805CED40` returns the argument), declared with the
 * `u32` return this block's call sites mask themselves (`clrlwi r0,r3,24`). */
u32 stage_map_kind_get(u32 kind);

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions: declared up front so the dispatchers can tail-call them.
 * ------------------------------------------------------------------------------------------------ */
void fn_80183718(_ENEMY_WORK* self);
void fn_801837B0(_ENEMY_WORK* self);
void fn_801841B4(_ENEMY_WORK* self);
void fn_80184280(_ENEMY_WORK* self);
void fn_801842FC(_ENEMY_WORK* self);
void fn_801843D4(_ENEMY_WORK* self);
void fn_8018454C(_ENEMY_WORK* self, u32 arg);
void fn_8018460C(_ENEMY_WORK* self, u32 arg);
void fn_801846BC(_ENEMY_WORK* self, u32 arg1, u32 arg2);
void fn_8018479C(_ENEMY_WORK* self);
void fn_8018484C(_ENEMY_WORK* self, u32 arg1, u32 arg2);
void fn_8018493C(_ENEMY_WORK* self, u32 arg);
void fn_801844B8(_ENEMY_WORK* self);
void fn_80184A5C(_ENEMY_WORK* self);
void fn_80184B0C(_ENEMY_WORK* self);
void fn_80184B78(_ENEMY_WORK* self);
void fn_80184434(_ENEMY_WORK* self);
void fn_80184458(_ENEMY_WORK* self);

extern f32 lbl_80797E9C;
extern f32 lbl_80797EA0;
extern f32 lbl_80797EA4;
extern f32 lbl_80797EA8;
extern f32 lbl_80797EAC;

extern f32 lbl_80797EB4;

extern f32 lbl_80797EC0;
extern f32 lbl_80797EC4;
extern f32 lbl_80797EC8;
extern f32 lbl_80797ECC;
/* The two action tables `fn_8018454C` picks between (`arg` 1 selects the second). */
extern u32 lbl_8056FF50[];
extern u32 lbl_8056FF90[];
/* The action table `fn_80184CE0` arms. */
extern u32 lbl_80570010[];
extern u32 lbl_8056FFD0[];
extern f32 lbl_80797ED0;
extern f32 lbl_80797ED4;
extern f32 lbl_80797ED8;
extern f32 lbl_80797EDC;
extern f32 lbl_80797EE0;
extern f32 lbl_80797EE4;
extern f32 lbl_80797EE8;
extern f32 lbl_80797EEC;
extern f32 lbl_80797F00;
extern f32 lbl_80797F04;
/* The table the 0xC-byte helper's constructors install at +0x00, in this unit's `.data`. */
extern u32 lbl_805AD340[];
}

/* The mangled callees, outside `extern "C"` with the widths each mangling encodes (rule 9). */
u16 calcVecAngX(nw4r::math::VEC3* v);
void eft009_set_pos(u8 id, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u32 arg);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u16 em_get_mot_no(struct _ENEMY_WORK* self);
f32 get_em_chg_scale(struct _ENEMY_WORK* self);
f32 get_em_scale(struct _ENEMY_WORK* self);
void get_joint_wmat_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

extern "C" {
/* ------------------------------------------------------------------------------------------------ *
 * This unit's own functions, declared up front so the dispatchers can call them.
 * ------------------------------------------------------------------------------------------------ */
void fn_8018B3B8(_ENEMY_WORK* self);
void fn_8018B3BC(_ENEMY_WORK* self);
void fn_8018B3C8(_ENEMY_WORK* self);
void fn_8018B418(_ENEMY_WORK* self);
void fn_8018BBD8(_ENEMY_WORK* self);
void fn_8018BC7C(_ENEMY_WORK* self);
void fn_8018BF4C(_ENEMY_WORK* self);
void fn_8018BFF0(_ENEMY_WORK* self);
void fn_8018C2CC(_ENEMY_WORK* self);
void fn_8018C370(_ENEMY_WORK* self);
void fn_8018C528(_ENEMY_WORK* self);
void fn_8018C5CC(_ENEMY_WORK* self);
void fn_8018C998(_ENEMY_WORK* self);
void fn_8018CA3C(_ENEMY_WORK* self);
void fn_8018CDE0(_ENEMY_WORK* self);
void fn_8018CE84(_ENEMY_WORK* self);
void fn_8018D1AC(_ENEMY_WORK* self);
void fn_8018D250(_ENEMY_WORK* self);
void fn_8018D2B0(_ENEMY_WORK* self);
void fn_8018D370(_ENEMY_WORK* self);
void fn_8018D558(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5);
void fn_8018D8C8(_ENEMY_WORK* self);
void fn_80191038(_ENEMY_WORK* self);
s32 fn_801913FC(_ENEMY_WORK* self, u8 arg1);

void fn_80184D98(_ENEMY_WORK* self, u32 a, u32 b);
void fn_80183DCC(_ENEMY_WORK* self);
void fn_80184488(_ENEMY_WORK* self);
void fn_80184BF8(_ENEMY_WORK* self);
void fn_801861E4(_ENEMY_WORK* self);
void fn_80186960(_ENEMY_WORK* self);
void fn_80189C7C(_ENEMY_WORK* self);
void fn_8018A974(_ENEMY_WORK* self);
void fn_8018AB64(_ENEMY_WORK* self);
void fn_8018AB94(_ENEMY_WORK* self);
void fn_8018AC64(_ENEMY_WORK* self);
void fn_8018ACEC(_ENEMY_WORK* self);
void fn_8018AD68(_ENEMY_WORK* self);
void fn_8018ADE8(_ENEMY_WORK* self);
void fn_8018AE7C(_ENEMY_WORK* self);
void fn_8018B1C4(_ENEMY_WORK* self);
void fn_8018B258(_ENEMY_WORK* self);
s16 em_demo_frame_get(void);
u32 em_demo_time_ck(u32 a);
void em_demo_pos_set(_ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_rot_set(_ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_reset(_ENEMY_WORK* self, u32 a);
void em_demo_enable(_ENEMY_WORK* self);
void em_demo_key3_apply(_ENEMY_WORK* self, s16 a, const void* tbl, u32 b);
void em_demo_key_apply(_ENEMY_WORK* self, s16 a, const void* tbl, const void* tbl2, u32 b, u32 c);
void eft009_spawn_at_joint(_ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_801049D0(_ENEMY_WORK* self, u32 a, u32 b, u32 c, nw4r::math::VEC3* p, f32 d);
void eft_spawn_type10(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, f32 d);
void eft_spawn_pos_in_area(nw4r::math::VEC3* p, u8 a, u32 b, u32 c, f32 d);
void fn_8011D448(_ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_8011D4FC(_ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d, f32 e);
void fn_8011D690(_ENEMY_WORK* self, u32 a, u32 b, f32 c);
void eft_em_spawn_joint(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, u32 c, f32 d);
void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, f32 c);
void addVec3(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 fn_8005024C(u16 a);

extern f32 lbl_80797EB8;

extern f32 lbl_80797EF8;

extern f32 lbl_80797F08;
extern f32 lbl_80797F0C;
extern f32 lbl_80797F10;
extern f32 lbl_80797F14;
extern f32 lbl_80797F34;
extern f32 lbl_80797F38;
extern f32 lbl_80797F40;
extern f32 lbl_80797F44;
extern f32 lbl_80797F4C;
extern f32 lbl_80797F50;
extern f32 lbl_80797F58;
extern f32 lbl_80797F5C;
extern f32 lbl_80797F64;
extern f32 lbl_80797F68;
extern f32 lbl_80797F6C;
extern f32 lbl_80797F70;
extern f32 lbl_80797F78;
extern f32 lbl_80797F7C;
extern f32 lbl_80797F80;
extern f32 lbl_80797F84;
extern f32 lbl_80797F90;
extern f32 lbl_80797F98;
extern f32 lbl_80797F9C;
extern f32 lbl_80797FA4;
extern f32 lbl_80797FA8;
extern f32 lbl_80797FB0;
extern f32 lbl_80797FC8;
extern f32 lbl_80797FCC;
extern f32 lbl_80797FE0;
extern f32 lbl_80797FF4;
extern f32 lbl_80797FF8;

extern f32 lbl_80798004;
extern f32 lbl_8079800C;
extern f32 lbl_80798010;
extern f32 lbl_80798014;
extern f32 lbl_80798018;
extern f32 lbl_8079801C;
extern f32 lbl_80798020;
extern f32 lbl_80798024;
extern f32 lbl_80798028;
extern f32 lbl_8079802C;
extern f32 lbl_80798030;
extern f32 lbl_80798034;
extern f32 lbl_80798038;
extern f32 lbl_8079803C;
extern f32 lbl_80798040;
extern f32 lbl_80798044;
extern f32 lbl_80798048;
extern f32 lbl_8079804C;
extern f32 lbl_80798050;
extern f32 lbl_80798054;
extern f32 lbl_80798058;
extern f32 lbl_8079805C;
extern f32 lbl_80798060;
extern f32 lbl_80798064;
extern f32 lbl_80798068;
extern f32 lbl_8079806C;
extern f32 lbl_80798070;
extern f32 lbl_80798074;
extern f32 lbl_80798078;
extern f32 lbl_8079807C;
extern f32 lbl_80798080;
extern f32 lbl_80798084;
extern f32 lbl_80798088;
extern f32 lbl_8079808C;
extern f32 lbl_80798090;
extern f32 lbl_80798094;
extern f32 lbl_80798098;
extern f32 lbl_8079809C;
extern f32 lbl_807980A0;
extern f32 lbl_807980A4;
extern f32 lbl_807980A8;
extern f32 lbl_807980AC;
extern f32 lbl_807980B0;
extern f32 lbl_807980B4;
extern f32 lbl_807980B8;
extern f32 lbl_807980BC;
extern f32 lbl_807980C0;
extern f32 lbl_807980C4;
extern f32 lbl_807980C8;
extern f32 lbl_807980CC;
extern f32 lbl_807980D0;
extern f32 lbl_807980D4;
extern f32 lbl_807980D8;
extern f32 lbl_807980DC;
extern f32 lbl_807980E0;
extern f32 lbl_807980E4;
extern f32 lbl_807980E8;
extern f32 lbl_807980EC;
extern f32 lbl_807980F0;
extern f32 lbl_807980F4;
extern f32 lbl_807980F8;
extern f32 lbl_807980FC;
extern f32 lbl_80798100;
extern f32 lbl_80798104;
extern f32 lbl_80798108;
extern f32 lbl_8079810C;
extern f32 lbl_80798110;
extern f32 lbl_80798114;
extern f32 lbl_80798118;
extern f32 lbl_8079811C;
extern f32 lbl_80798120;
extern f32 lbl_80798124;
extern f32 lbl_80798128;
extern f32 lbl_8079812C;
extern f32 lbl_80798130;
extern f32 lbl_80798134;
extern f32 lbl_80798138;
extern f32 lbl_8079813C;
extern f32 lbl_80798140;
extern f32 lbl_80798144;
extern f32 lbl_80798148;
extern f32 lbl_8079814C;
extern f32 lbl_80798150;
extern f32 lbl_80798154;
extern f32 lbl_80798158;
extern f32 lbl_8079815C;
extern f32 lbl_80798160;
extern f32 lbl_80798164;
extern f32 lbl_80798168;
extern f32 lbl_8079816C;
extern f32 lbl_80798170;
extern f32 lbl_80798174;
extern f32 lbl_80798178;
extern f32 lbl_8079817C;
extern f32 lbl_80798180;
extern f32 lbl_80798184;
extern f32 lbl_80798188;
extern f32 lbl_8079818C;
extern f32 lbl_80798190;
extern f32 lbl_80798194;
extern f32 lbl_80798198;
extern f32 lbl_8079819C;
extern f32 lbl_807981A0;
extern f32 lbl_807981A4;
extern f32 lbl_807981A8;
extern f32 lbl_807981AC;
extern f32 lbl_807981B0;
extern f32 lbl_807981B4;
extern f32 lbl_807981B8;
extern f32 lbl_807981BC;
extern f32 lbl_807981C0;
extern f32 lbl_807981C4;
extern f32 lbl_807981C8;
extern f32 lbl_807981CC;
extern f32 lbl_807981D0;
extern f32 lbl_807981D4;
extern f32 lbl_807981D8;
extern f32 lbl_807981DC;
extern f32 lbl_807981E0;
extern f32 lbl_807981E4;
extern f32 lbl_807981E8;
extern f32 lbl_807981EC;
extern f32 lbl_807981F0;
extern f32 lbl_807981F4;
extern f32 lbl_807981F8;
extern f32 lbl_807981FC;
extern f32 lbl_80798200;
extern f32 lbl_80798204;
extern f32 lbl_80798208;
extern f32 lbl_8079820C;
extern f32 lbl_80798210;
extern f32 lbl_80798214;
extern f32 lbl_80798218;
extern f32 lbl_8079821C;
extern f32 lbl_80798220;
extern f32 lbl_80798224;
extern f32 lbl_80798228;
extern u32 lbl_805ABDB8[];
extern u32 lbl_805ABF30[];
extern u32 lbl_805AC1F0[];
extern u32 lbl_805AC3C8[];
extern u32 lbl_805AC698[];
extern u32 lbl_805AC9A8[];
extern u32 lbl_805ACC00[];
extern u32 lbl_805ACD80[];
}

/* `_CP_VECTOR` is `ef.h`'s three-word type; `ef.h` is not included because its `setVec3` returns `void`
 * where these call sites read r3. */
struct _CP_VECTOR;

/* The 3x3 matrix `fn_805012E8` fills (nine floats; `fn_80191CE4`'s frame is sized for it).
 * size: 0x24 */
struct EmMtx33 {
    /* +0x00 */ f32 m[3][3];
};

/* The user-data item the class methods take (`enemy/fn_80138074.c`'s `UserDataItem`; only +0x04 is read).
 * size: 0x18 */
struct EmUserItem {
    /* +0x00 */ u32 key_0x00;
    /* +0x04 */ u8 type_0x04;   /* 0xFF admits the call */
    /* +0x05 */ u8 unused_0x05[0x13];
};

/* The matrix-pointer holder `joint_mtx_store`/`joint_mtx_load` take (the owner's `MtxHolder`).  size: 0x04 */
struct EmMtxHolder {
    /* +0x00 */ MTX34* mtx_0x00;
};

/* The static vector pair `fn_80192204` builds (`VEC3[2]` per 0x18-byte object).
 * size: 0x18 */
struct EmVecPair {
    /* +0x00 */ VEC3 vec_0x00;
    /* +0x0C */ VEC3 vec_0x0C;
};

/* The request record `fn_80192108` fills in.
 * size: 0x18 */
struct EmEffRequest {
    /* +0x00 */ u32 id_0x00;      /* always 0x17 */
    /* +0x04 */ VEC3 pos_0x04;
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 unused_0x11;
    /* +0x12 */ s16 field_0x12;
    /* +0x14 */ s16 field_0x14;
    /* +0x16 */ u8 unused_0x16[0x02];
};

/* ------------------------------------------------------------------------------------------------ */
/* the unit's pool, declared, not defined                                                            */
/* ------------------------------------------------------------------------------------------------ */
extern const f32 lbl_80797F24;
extern const f32 lbl_8079822C;
extern const f32 lbl_80798230;
extern const f32 lbl_80798234;

/* The five static vectors `fn_80192204` builds (the last one is the default `fn_80192108` copies
 * from). */
extern EmVecPair vec_pair_80191598_0;
extern EmVecPair vec_pair_80191598_1;
extern EmVecPair vec_pair_80191598_2;
extern EmVecPair vec_pair_80191598_3;
extern VEC3 vec_default_80191598;

/* The one-shot latch `fn_80192108` sets. */
extern s8 lbl_80794AA0;

/* The `.data` word `fn_80191B4C` hands to `fn_8012A014`, in the unit's class-table block. */
extern u8 lbl_805AAA74[];

u8 em_parts_damage_level_get(struct _ENEMY_WORK* work, u8 part);

void cpSetRotMatrix(struct _CP_VECTOR* rot, MTX34* mtx);

extern "C" {
/* The plain callees, declared with the call sites' arity and return widths. */
void fn_803B9BA0(EmActWork* self, VEC3* pos, s32 value);

void joint_mtx_store(EmMtxHolder* holder, void* src);
void joint_mtx_load(EmMtxHolder* holder, void* mtx);
void fn_8008E8D0(void* holder, void* vec);
void fn_8008EE68(void* holder, void* mtx);
void fn_800FBB90(MTX34* mtx, VEC3* vec);
void eft_rot_vec_copy(void* dst, void* src);
void fn_805012E8(EmMtx33* dst, const MTX34* src);

void fn_800516F0(void* mtx);

/* This block's own entry points. */
void fn_80191598(EmActWork* self, u8* out_class, u8* out_state);
void fn_80191990(EmActWork* self);
u32 fn_80191A6C(EmActWork* self);
u32 fn_80191AD8(EmActWork* self);
void fn_80191AE8(EmActWork* self, u32 kind);
s32 fn_80191B4C(EmActWork* self, u32 arg);
void fn_80191CDC(EmUserData* self);
void fn_80191CE4(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item);
void fn_80191E30(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item);
void fn_80191EF8(EmActWork* self);
void fn_80192080(EmActWork* self);
void fn_80192108(EmEffRequest* out, u8 a, s16 b, s16 c);
void* fn_801921A8(void* p, u32 flag);
void fn_80192204(void);
}

/* Rotates a per-kind offset into the work's position and writes the resulting relative vector the movement code
 * steers by. */
void fn_80182C40(_ENEMY_WORK* self, u32 kind, void* out) {
    VEC3 v;
    VEC3 rel;
    VEC3_ctor(&v);
    v.x = lbl_80797E88;
    v.y = lbl_80797E88;
    v.z = lbl_80797E8C;
    s32 offset;
    switch (kind & 0xFF) {
    case 1:
        offset = 0x9555;
        v.z = lbl_80797E8C;
        break;
    case 2:
        offset = 0xAAAB;
        v.z = lbl_80797E94;
        break;
    case 10:
        offset = 0xA000;
        v.z = lbl_80797E98;
        break;
    case 11:
        offset = 0x4E39;
        v.z = lbl_80797E90;
        break;
    default:
        offset = 0x11C7;
        v.z = lbl_80797E90;
        break;
    }
    rotVecY(&v, (u16)(self->field_0x1C0 + offset));
    addVec3(&rel, &self->pos, &v);
    copyVec3((nw4r::math::VEC3*)out, &rel);
}

/* Forwards the value to the motion setter only for team 0x10. */
void fn_80182D44(_ENEMY_WORK* self, u32 arg) {
    if (self->team == 0x10) {
        fn_80130CDC(self, (s16)arg);
    }
}

/* Per-(map,area) motion start: two timed motion sets, then the area's pair. */
extern "C" void fn_80182D5C(_ENEMY_WORK* self) {
    {
        f32 t = lbl_80797E88;
        em_motion_param_set(self, 0, t);
    }
    {
        f32 t = lbl_80797E9C;
        em_motion_param_set(self, 0x1E, t);
    }

    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 5:
            em_move_target_set(self, 6, 7, lbl_80797EA0);
            break;
        case 6:
            em_move_target_set(self, 9, 10, lbl_80797EA4);
            break;
        case 7:
            em_move_target_set(self, 0xD, 0xE, lbl_80797EA0);
            break;
        case 8:
            em_move_target_set(self, 8, 9, lbl_80797EA0);
            break;
        case 0xC:
            em_move_target_set(self, 0x19, 0x1A, lbl_80797EA0);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 1:
            em_move_target_set(self, 6, 7, lbl_80797EA8);
            break;
        case 2:
            em_move_target_set(self, 7, 8, lbl_80797EA0);
            break;
        case 3:
            em_move_target_set(self, 9, 10, lbl_80797EAC);
            break;
        case 4:
            em_move_target_set(self, 5, 6, lbl_80797EA0);
            break;
        case 5:
            em_move_target_set(self, 7, 8, lbl_80797EA0);
            break;
        case 6:
            em_move_target_set(self, 0xD, 0xE, lbl_80797EAC);
            break;
        case 7:
            em_move_target_set(self, 0, 2, lbl_80797EA0);
            break;
        case 8:
            em_move_target_set(self, 6, 7, lbl_80797EA0);
            break;
        case 10:
            em_move_target_set(self, 0, 1, lbl_80797EAC);
            break;
        }
        break;
    }
}

/* Whether this map/area pair has a motion set at all. */
extern "C" u32 fn_80182F60(_ENEMY_WORK* self) {
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if ((u32)(self->area_no - 5) <= 1U || (s32)self->area_no == 8) {
            return 1;
        }
        break;
    case 3:
        if ((s32)self->area_no == 1 || (s32)self->area_no == 5 || (s32)self->area_no == 7
            || (s32)self->area_no == 10) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Arms the motion that this map/area pair starts with. */
extern "C" void fn_80182FF8(_ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    fn_80128AAC(self, 6, 5);
    fn_80133BB4(self);
}

/* The 0xC-byte helper's second constructor: base first, then this class's vtable. */
extern "C" void* fn_80183440(void* self) {
    em_res_user_data_ctor(self);
    *(void**)self = lbl_805AD340;
    return self;
}

/* The map-0x15 (teardown) step: run it once `em_die_ck` and the state check agree. */
extern "C" void fn_80183A54(_ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        if (em_hit_timer_ck(self) == 1) {
            em_attack_start(self);
        }
    }
}

/* ---- the per-team step dispatch (`team` 0x10/0x11/0x15) and its steps ---- */

/* The first of five armed-motion steps: phase 0 latches the state byte and arms the motion pair, phase 1
 * waits for `em_mot_end_ck` and runs the finish call. */
extern "C" void fn_80183AD0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80183B4C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x14, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80183BC8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1D, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80183C44(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x28, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80183CC4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x36, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80183D44(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0xCA, 6, 0);
        fn_801303EC(self, lbl_80797E88);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* Per-`team` step dispatch into the five armed-motion steps above (tail calls). */
extern "C" void fn_80183AA0(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_80183718(self);
        return;
    case 0x11:
        fn_801837B0(self);
        return;
    case 0x15:
        fn_80183A54(self);
        return;
    }
}

/* Per-`state_sub` step dispatch into the same five steps (tail calls). */
extern "C" void fn_80183DCC(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80183AD0(self);
        return;
    case 1:
        fn_80183B4C(self);
        return;
    case 2:
        fn_80183BC8(self);
        return;
    case 4:
        fn_80183C44(self);
        return;
    case 5:
        fn_80183CC4(self);
        return;
    case 7:
        fn_80183D44(self);
        return;
    }
}

/* The armed-motion step with the (0x1A, 0xA) pair. */
extern "C" void fn_80183E20(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 0xA, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The four-phase armed-motion step: arm, run the sub-action and its 0x708-frame timer, then count it down and
 * finish. */
extern "C" void fn_80183E9C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 6, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            u8 state = self->state;
            self->state = state + 1;
            em_mot_set(self, 4, 0, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2: {
        fn_8013221C(self, lbl_80797EC0, 1, 0xF);
        s32 left = self->timer_0x020 - 1;
        self->timer_0x020 = left;
        if (left <= 0) {
            u8 state = self->state;
            self->state = state + 1;
            em_mot_set(self, 5, 4, 0);
            fn_80132264(self);
        }
        break;
    }
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The first of four more armed-motion steps, the same two phases. */
extern "C" void fn_80183FB8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0x14, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80184038(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801840B4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 2, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 1, 7);
        }
        break;
    }
}

extern "C" void fn_80184138(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step with two frame checks and its finish step. */
extern "C" void fn_801841B4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 4, 0);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797EC4, lbl_80797E88) == 1) {
            em_shake_req_set(self);
            if (em_frame_check(self, 1, lbl_80797EA4, lbl_80797E88) == 1) {
                em_motion_timer_arm(self);
                em_fx_flag_set(self);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_attack_done_set(self);
        }
        break;
    }
}

/* The first of the second family's last two armed-motion steps. */
extern "C" void fn_80184280(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 6, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801842FC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        fn_801303EC(self, lbl_80797E88);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797EC8, lbl_80797E88) == 1) {
            em_shake_req_set(self);
            if (em_frame_check(self, 1, lbl_80797ECC, lbl_80797E88) == 1) {
                em_motion_timer_arm(self);
                em_fx_flag_set(self);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_attack_done_set(self);
        }
        break;
    }
}

/* The first of the second family's three per-`state_sub` dispatchers (tail calls). */
extern "C" void fn_801843D4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80183E20(self);
        return;
    case 1:
        fn_80183E9C(self);
        return;
    case 2:
        fn_80183FB8(self);
        return;
    case 3:
        fn_80184038(self);
        return;
    case 4:
        fn_801840B4(self);
        return;
    case 5:
        fn_80184138(self);
        return;
    case 7:
        fn_80184280(self);
        return;
    }
}

extern "C" void fn_80184434(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_80183FB8(self);
        return;
    case 6:
        fn_801841B4(self);
        return;
    }
}

extern "C" void fn_80184458(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_80183FB8(self);
        return;
    case 3:
        fn_80184038(self);
        return;
    case 8:
        fn_801842FC(self);
        return;
    }
}

/* Per-`team` dispatch into the second step family (tail calls). */
extern "C" void fn_80184488(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_801843D4(self);
        return;
    case 0x11:
        fn_80184434(self);
        return;
    case 0x15:
        fn_80184458(self);
        return;
    }
}

/* The armed-motion step that hands a float to `em_approach_start` and waits on `em_approach_step`. */
extern "C" void fn_801844B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_approach_start(self, lbl_80797E88, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same step, but the argument picks one of two action tables. */
extern "C" void fn_8018454C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (u8)arg == 1 ? lbl_8056FF90 : lbl_8056FF50, 0, 0, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, (u8)arg == 1 ? lbl_8056FF90 : lbl_8056FF50) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step whose phase 1 gates `em_turn_to_target` on an argument and a frame
 * check. */
extern "C" void fn_8018460C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 6, 0);
        break;
    }
    case 1:
        if ((u8)arg == 1 && em_frame_check(self, 3, lbl_80797ED0, lbl_80797ED4) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step whose argument picks the motion pair and whether the target
 * distance is clamped. */
extern "C" void fn_801846BC(_ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        u32 motion = (u8)arg1 == 1 ? 9 : 2;
        em_mot_set(self, (u16)motion, 0xA, 0);
        em_approach_start(self, lbl_80797E88, 0);
        if ((u8)arg2 == 1 && self->value_0x378 > lbl_80797ED8) {
            self->value_0x378 = lbl_80797ED8;
        }
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step shared by two sub-states (`em_busy_set`/`em_busy_timer_reset` first). */
extern "C" void fn_8018479C(_ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);

    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xA, 0);
        em_approach_start(self, lbl_80797E88, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_state_set(self, 5, 5);
        }
        break;
    }
}

/* The armed-motion step whose argument picks the motion pair, the blend float and the
 * `em_approach_step` mask. */
extern "C" void fn_8018484C(_ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 0xA, 0);
        em_hit_window_set(self, 0, 0xD, 2);

        f32 blend;
        switch ((u8)arg2) {
        default:
            blend = lbl_80797E88;
            break;
        case 1:
            blend = lbl_80797EDC;
            break;
        case 2:
            blend = lbl_80797EE0;
            break;
        }
        em_approach_start(self, blend, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, (u16)((u8)arg1 == 1 ? 0xC0 : 0x40)) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The armed-motion step with the 0x96/0x5A countdown derived from `bits_0x1EC`. */
extern "C" void fn_8018493C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 4, 0);

        u32 ticks = self->bits_0x1EC & 0x1F;
        self->timer_0x020 = ticks + 0x96;

        f32 blend;
        switch ((u8)arg) {
        default:
            blend = lbl_80797E88;
            break;
        case 1:
            blend = lbl_80797EE4;
            break;
        case 2:
            blend = lbl_80797E88;
            break;
        case 3:
            blend = lbl_80797EE4;
            self->timer_0x020 = ticks + 0x5A;
            break;
        }
        em_approach_start(self, blend, 0);
        break;
    }
    case 1: {
        u32 done = 0;
        if ((u8)arg != 2) {
            s32 left = self->timer_0x020 - 1;
            self->timer_0x020 = left;
            if (left <= 0) {
                done = 1;
            }
        }
        if (em_approach_step(self, 0, 0x40) == 1 || done == 1) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* The first of the third family's three per-`state_sub` dispatchers (tail calls; the dense ones are jump
 * tables). */
extern "C" void fn_80184A5C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801844B8(self);
        return;
    case 1:
        fn_8018454C(self, 0);
        return;
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 4:
        fn_801846BC(self, 1, 0);
        return;
    case 5:
        fn_8018479C(self);
        return;
    case 6:
        fn_8018454C(self, 0);
        return;
    case 7:
        fn_8018484C(self, 0, 1);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 9:
        fn_8018484C(self, 1, 0);
        return;
    case 10:
        fn_801846BC(self, 0, 1);
        return;
    case 11:
        fn_801846BC(self, 1, 1);
        return;
    case 13:
        fn_8018484C(self, 0, 0);
        return;
    case 16:
        fn_8018484C(self, 0, 2);
        return;
    }
}

extern "C" void fn_80184B0C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 12:
        fn_8018493C(self, 1);
        return;
    case 14:
        fn_8018493C(self, 0);
        return;
    case 15:
        fn_8018493C(self, 2);
        return;
    case 17:
        fn_8018454C(self, 1);
        return;
    case 18:
        fn_8018493C(self, 3);
        return;
    }
}

extern "C" void fn_80184B78(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 12:
        fn_8018493C(self, 1);
        return;
    case 14:
        fn_8018493C(self, 0);
        return;
    case 15:
        fn_8018493C(self, 2);
        return;
    case 17:
        fn_8018454C(self, 1);
        return;
    }
}

/* The armed-motion step with the `field_0x482`-selected approach float. */
extern "C" void fn_80184CE0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570010, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, lbl_80570010) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797EE8);
        } else {
            fn_80136D4C(self, lbl_80797EEC);
        }
        break;
    }
}

/* Per-`team` dispatch into the third step family (tail calls). */
extern "C" void fn_80184BF8(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_80184A5C(self);
        return;
    case 0x11:
        fn_80184B0C(self);
        return;
    case 0x15:
        fn_80184B78(self);
        return;
    }
}

/* `fn_80184CE0`'s sibling with the other action table. */
extern "C" void fn_80184C28(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_8056FFD0, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, lbl_8056FFD0) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797EE8);
        } else {
            fn_80136D4C(self, lbl_80797EEC);
        }
        break;
    }
}

/* The first of the fifth family's five armed-motion steps: arm the motion, then finish on `em_mot_end_ck`. */
extern "C" void fn_801856B8(_ENEMY_WORK* self) {
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

extern "C" void fn_80185738(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x35, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801857B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80185838(_ENEMY_WORK* self) {
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
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801858B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x28, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* One more step of the same family. */
extern "C" void fn_80185BEC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80185FA0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* The team-selected motion step (`em_busy_set` runs first). */
extern "C" void fn_80185D60(_ENEMY_WORK* self) {
    em_busy_set(self);

    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)(self->team == 0x10 ? 0x3D : 0x3B), 0, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The motion step that stores the target angle byte: the difference between the body angle and `field_0x1C0`,
 * quantised into `state_0x007` (0 / 0x80 / the >>8 byte). */
extern "C" void fn_80185B0C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x2D, 0xA, 0);

        u32 diff = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (diff > 0x8000) {
            if (diff > 0xC000) {
                self->state_0x007 = 0;
            } else {
                self->state_0x007 = 0x80;
            }
        } else {
            self->state_0x007 = (u8)((s32)diff >> 8);
        }
        break;
    }
    case 1:
        em_turn_in_window(self, lbl_80797EB4, lbl_80797ECC, (s32)(self->state_0x007 << 8));
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* The motion step that runs the angle-driven approach: two `em_frame_check` gates and a clamped blend of
 * `fn_8012F8EC`'s value. */
extern "C" void fn_801850F8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x39, 0xA, 0, 1);
        break;
    }
    case 1:
        fn_80136D4C(self, lbl_80797EEC);

        if (em_frame_check(self, 1, lbl_80797F00, lbl_80797E88) == 1) {
            f32 blend = lbl_80797EEC * (fn_8012F8EC(self) - lbl_80797EB4);
            if (blend > lbl_80797EA4) {
                blend = lbl_80797EA4;
            }
            fn_8012FE3C(self, blend + fn_8013026C(self));
        }

        if (em_frame_check(self, 1, lbl_80797F04, lbl_80797E88) == 1) {
            em_turn_to_target(self, 0x30);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------ */

/* 0x8018B3B8 - `state_sub` 8 of the multi-motion dispatch: a tail call into the armed-motion step. */
void fn_8018B3B8(_ENEMY_WORK* self) {
    fn_8018479C(self);
}

/* 0x8018B3BC - `state_sub` 9: the motion-table step with id 8, no sub. */
void fn_8018B3BC(_ENEMY_WORK* self) {
    fn_80184D98(self, 8, 0);
}

/* 0x8018B3C8 - the multi-motion dispatcher keyed on the work's +0x1E6 class (0..9). */
void fn_8018B3C8(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8018AB94(self);
        return;
    case 1:
        fn_8018AC64(self);
        return;
    case 2:
        fn_8018ACEC(self);
        return;
    case 3:
        fn_8018AD68(self);
        return;
    case 4:
        fn_8018ADE8(self);
        return;
    case 5:
        fn_8018AE7C(self);
        return;
    case 6:
        fn_8018B1C4(self);
        return;
    case 7:
        fn_8018B258(self);
        return;
    case 8:
        fn_8018B3B8(self);
        return;
    case 9:
        fn_8018B3BC(self);
        return;
    default:
        return;
    }
}

/* 0x8018B418 - the run's opening step machine (states 0..10) on the work's +0x5 byte. */
void fn_8018B418(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x28, 0, 0);
        em_demo_reset(self, 0);
        em_demo_pos_set(self, lbl_80798010, lbl_80798014, lbl_80798018);
        em_demo_rot_set(self, lbl_80797E88, lbl_8079801C, lbl_80797E88);
        return;
    case 1:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x37, 0, 0);
            em_demo_enable(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F78);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        if (em_demo_time_ck(0x1DE) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x2C, 0x18, 0, 1);
            em_demo_pos_set(self, lbl_80798020, lbl_80798014, lbl_80798024);
        }
        break;
    case 3:
        if (em_frame_check(self, 3, lbl_80797F6C, lbl_80797F08) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, lbl_80797E9C);
        }
        if (em_demo_time_ck(0x1FA) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x2C, 0, 0x1E);
            em_demo_pos_set(self, lbl_80798028, lbl_8079802C, lbl_80798030);
            em_demo_rot_set(self, lbl_80797E88, lbl_8079801C, lbl_80797E88);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805ABDB8, NULL, 5, 0);
        }
        break;
    case 4:
        if (em_frame_check(self, 3, lbl_80797F80, lbl_80798034) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, lbl_80797EB8);
        }
        if (em_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        self->pos.y = lbl_8079802C;
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805ABDB8, NULL, 5, 0);
        if (em_demo_time_ck(0x2B2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x29, 0xE, 0, 1);
        }
        break;
    case 5:
        if (em_frame_check(self, 3, lbl_80797F6C, lbl_80797F08) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, lbl_80797E9C);
        }
        if (em_frame_check(self, 3, lbl_80797F80, lbl_80798034) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, lbl_80797EB8);
        }
        if (em_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ABF30, 0);
        if (em_demo_time_ck(0x382) == 1U) {
            self->state = (u8)(self->state + 1);
            em_fall_start_view1(self, lbl_80797E88);
            em_mot_set_blend(self, 0x3C, 0xE, 0x6C, 1);
            em_demo_pos_set(self, lbl_8079803C, lbl_80798040, lbl_80798044);
        }
        break;
    case 6:
        if (em_frame_check(self, 0, lbl_80798048, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797EA4);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, lbl_80797E9C);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ABF30, 0);
        if (em_demo_time_ck(0x3B0) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x40, 0, 0);
            em_mot_speed_set(self, lbl_8079804C);
            em_demo_pos_set(self, lbl_80798050, lbl_80798054, lbl_80798058);
            em_demo_rot_set(self, lbl_80797E88, lbl_8079805C, lbl_80797E88);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC1F0, NULL, 3, 0);
            eft009_spawn_at_joint(self, 3U, 0x11U, 0, lbl_80797EB8);
        }
        break;
    case 7:
        if (em_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797F10, lbl_80797E88);
            eft_em_spawn(self, 0x32, 3, &sp8, lbl_8079804C);
        }
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC1F0, NULL, 3, 0);
        if (em_demo_time_ck(0x420) == 1U) {
            self->state = (u8)(self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3D, 0, 0);
            em_demo_pos_set(self, lbl_80798060, lbl_80798064, lbl_80798068);
            em_demo_rot_set(self, lbl_80797E88, lbl_8079806C, lbl_80797E88);
        }
        break;
    case 8:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 0x15U, 0x24U, 0, lbl_80797EC0);
        }
        if (em_demo_time_ck(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_pos_set(self, lbl_80798070, lbl_80798074, lbl_80798078);
        }
        break;
    case 9:
        if (em_demo_time_ck(0x4A6) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x1A, 0xA, 0, 1);
        }
        break;
    case 10:
        if (em_demo_time_ck(0x582) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 1, 0x1E, 0, 1);
        }
        break;
    default:
        break;
    }
}

/* 0x8018BBD8 - the first arm/step pair: a two-state opener for the motion-table step. */
void fn_8018BBD8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_80798070, lbl_80798074, lbl_80798078);
        em_demo_rot_set(self, lbl_80797E88, lbl_8079806C, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018BC7C - the run's second step machine (states 0..7). */
void fn_8018BC7C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_8079807C, lbl_80798080, lbl_80798084);
        em_demo_rot_set(self, lbl_80797E88, lbl_80797F44, lbl_80797E88);
        return;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 4, 0, 0);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 5, 0, 0x2A);
            em_demo_pos_set(self, lbl_80798088, lbl_80798080, lbl_8079808C);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F9C, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 8U, 0x92U, 0, lbl_80798090);
        }
        if (em_demo_time_ck(0x134) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_pos_set(self, lbl_80798094, lbl_80798098, lbl_8079809C);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980A0, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, lbl_807980A4, lbl_807980A8, lbl_807980AC);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980B0, lbl_80797E88);
            return;
        }
        break;
    case 6:
        if (em_demo_time_ck(0x4AE) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 2, 0x1E, 0, 1);
            return;
        }
        break;
    case 7:
        if (em_frame_check(self, 0, lbl_80797FA8, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 8U, 0x92U, 0, lbl_80798090);
        }
        if (em_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 0xDU, 0x92U, 0, lbl_80798090);
        }
        break;
    default:
        break;
    }
}

/* 0x8018BF4C - the second arm/step pair opener. */
void fn_8018BF4C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_pos_set(self, lbl_807980B4, lbl_807980A8, lbl_807980B8);
        em_demo_rot_set(self, lbl_80797E88, lbl_807980B0, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018BFF0 - the run's third step machine (states 0..8). */
void fn_8018BFF0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_807980BC, lbl_807980C0, lbl_807980C4);
        em_demo_rot_set(self, lbl_80797E88, lbl_807980C8, lbl_80797E88);
        return;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x134) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, lbl_807980BC, lbl_807980C0, lbl_807980C4);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980CC, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x420) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980D8, lbl_80797E88);
            return;
        }
        break;
    case 6:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x18, 0, 0);
            em_demo_pos_set(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980D8, lbl_80797E88);
            return;
        }
        break;
    case 7:
        em_turn_in_window(self, lbl_807980DC, lbl_807980E0, 0x4000);
        if (em_demo_time_ck(0x49A) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 1, 0x1E, 0, 1);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980E4, lbl_80797E88);
            return;
        }
        break;
    case 8:
        if (em_demo_time_ck(0x51E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x14, 0x28, 0, 1);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C2CC - the third arm/step pair opener. */
void fn_8018C2CC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 0, 0);
        em_demo_pos_set(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
        em_demo_rot_set(self, lbl_80797E88, lbl_807980E8, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018C370 - the run's fourth step machine (states 0..3). */
void fn_8018C370(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_807980EC, lbl_807980F0, lbl_807980F4);
        em_demo_rot_set(self, lbl_80797E88, lbl_807980F8, lbl_80797E88);
        return;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 2, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797ED0, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_807980FC, lbl_80797F34);
            fn_801049D0(self, 8, 0x16, 0, &sp8, lbl_80798100);
        }
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, lbl_80798104, lbl_80798108, lbl_8079810C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798110, lbl_80797E88);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C528 - the fourth arm/step pair opener. */
void fn_8018C528(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_80798104, lbl_80798108, lbl_8079810C);
        em_demo_rot_set(self, lbl_80797E88, lbl_80798110, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018C5CC - the run's fifth step machine (states 0..6). */
void fn_8018C5CC(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x168) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_enable(self);
            em_fall_start_view1(self, lbl_80797E88);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805AC3C8, 0);
            em_demo_rot_set(self, lbl_80797E88, lbl_80797FF8, lbl_80797E88);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 0x13U, 0xDU, 0x8000, lbl_80798090);
            eft009_spawn_at_joint(self, 0x13U, 0xFU, 0, lbl_80798090);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805AC3C8, 0);
        if (em_demo_time_ck(0x19E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set_blend(self, 0x3B, 6, 6, 1);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805AC3C8, 0);
        if (em_demo_time_ck(0x242) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 2, 0xE, 0x6A);
            return;
        }
        break;
    case 4:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805AC3C8, 0);
        if (em_demo_time_ck(0x26C) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_pos_set(self, lbl_80798114, lbl_80798118, lbl_8079811C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80797FF8, lbl_80797E88);
        }
        /* fallthrough */
    case 5:
        if (em_demo_time_ck(0x57A) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x18, 0, 0);
            em_mot_speed_set(self, lbl_80798120);
            em_demo_pos_set(self, lbl_80798124, lbl_80798128, lbl_8079812C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798130, lbl_80797E88);
            return;
        }
        break;
    case 6:
        self->field_0x1C0 = fn_80133DB0(0x49F5, (u16)self->field_0x1C0, 0x160);
        if (em_demo_time_ck(0x652) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x14, 0xA, 0);
            em_demo_pos_set(self, lbl_80798124, lbl_80798128, lbl_8079812C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798134, lbl_80797E88);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C998 - the fifth arm/step pair opener. */
void fn_8018C998(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, lbl_80798124, lbl_80798128, lbl_8079812C);
        em_demo_rot_set(self, lbl_80797E88, lbl_80798134, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018CA3C - the run's sixth step machine (states 0..7). */
void fn_8018CA3C(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x1C2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_enable(self);
            em_fall_start_view1(self, lbl_80797E88);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_rot_set(self, lbl_80797F14, lbl_80798138, lbl_80797E88);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC698, lbl_805AC9A8, 7, 3);
            fn_80133C3C(self);
            return;
        }
        break;
    case 2:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC698, lbl_805AC9A8, 7, 3);
        fn_80133C3C(self);
        if (em_demo_time_ck(0x1F8) == 1U) {
            self->state = (u8)(self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3B, 0xC, 6);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC698, lbl_805AC9A8, 7, 3);
        fn_80133C3C(self);
        if (em_demo_time_ck(0x26C) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_pos_set(self, lbl_8079813C, lbl_80797ED0, lbl_80798140);
            em_demo_rot_set(self, lbl_80797E88, lbl_80797F70, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x29C) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 2, 0x14, 0, 1);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x566) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x19, 0, 0x14);
            em_demo_pos_set(self, lbl_80798144, lbl_80798148, lbl_8079814C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798150, lbl_80797E88);
            return;
        }
        break;
    case 6:
        em_turn_in_window(self, lbl_80797ED0, lbl_80798154, -0x4000);
        if (em_demo_time_ck(0x5B8) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 1, 0xA, 0, 1);
            em_demo_pos_set(self, lbl_80798144, lbl_80798148, lbl_8079814C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798158, lbl_80797E88);
            return;
        }
        break;
    case 7:
        if (em_demo_time_ck(0x6F2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x14, 0xE, 0);
        }
        break;
    default:
        break;
    }
}

/* 0x8018CDE0 - the sixth arm/step pair opener. */
void fn_8018CDE0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, lbl_80798144, lbl_80798148, lbl_8079814C);
        em_demo_rot_set(self, lbl_80797E88, lbl_80798158, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018CE84 - the run's seventh step machine (states 0..5). */
void fn_8018CE84(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x4A8) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_enable(self);
            em_fall_start_view1(self, lbl_80797E88);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_rot_set(self, lbl_80797E88, lbl_80797E88, lbl_80797E88);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ACC00, 0);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 0x13U, 0x10U, 0, lbl_8079804C);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ACC00, 0);
        if (em_demo_time_ck(0x4DE) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x3B, 8, 6, 1);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        if (em_demo_time_ck(0x4E6) == 0) {
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ACC00, 0);
        } else {
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805ACD80, NULL, 5, 0);
        }
        if (em_demo_time_ck(0x584) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 2, 0, 0);
            em_demo_pos_set(self, lbl_8079815C, lbl_80798160, lbl_80798164);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x662) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 1, 0xA, 0, 1);
            em_demo_pos_set(self, lbl_80798168, lbl_8079816C, lbl_80798170);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x768) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* 0x8018D1AC - the seventh arm/step pair opener. */
void fn_8018D1AC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_80798168, lbl_8079816C, lbl_80798170);
        em_demo_rot_set(self, lbl_80797E88, lbl_80797E88, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018D250 - the `state_sub` class dispatcher (0..13) into this unit's twelve step machines. */
void fn_8018D250(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8018B418(self);
        return;
    case 1:
        fn_8018BBD8(self);
        return;
    case 2:
        fn_8018BC7C(self);
        return;
    case 3:
        fn_8018BF4C(self);
        return;
    case 4:
        fn_8018BFF0(self);
        return;
    case 5:
        fn_8018C2CC(self);
        return;
    case 6:
        fn_8018C370(self);
        return;
    case 7:
        fn_8018C528(self);
        return;
    case 8:
        fn_8018C5CC(self);
        return;
    case 9:
        fn_8018C998(self);
        return;
    case 10:
        fn_8018CA3C(self);
        return;
    case 11:
        fn_8018CDE0(self);
        return;
    case 12:
        fn_8018CE84(self);
        return;
    case 13:
        fn_8018D1AC(self);
        return;
    default:
        return;
    }
}

/* 0x8018D2B0 - the action dispatcher (0..13) plus the two shared post-checks. */
void fn_8018D2B0(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_80183DCC(self);
        break;
    case 1:
        fn_80184488(self);
        break;
    case 2:
        fn_80184BF8(self);
        break;
    case 5:
        fn_801861E4(self);
        break;
    case 6:
        fn_80186960(self);
        break;
    case 7:
        fn_80189C7C(self);
        break;
    case 10:
        fn_8018A974(self);
        break;
    case 11:
        fn_8018AB64(self);
        break;
    case 12:
        fn_8018B3C8(self);
        break;
    case 13:
        fn_8018D250(self);
        break;
    }
    if (self->team != 0x15 && self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* 0x8018D370 - the per-frame effect spawner: measures the joint point against the water/height
 * gates and picks one of the four ground/air effect ids. */
void fn_8018D370(_ENEMY_WORK* self) {
    VEC3 sp8;
    MTX34 sp18;
    u16 ang;
    u16 mot;

    VEC3_ctor(&sp8);
    MTX34_ctor(&sp18);
    if (em_alt_mode_ck_c1(self) != 0) {
        ang = calcVecAngX(&self->vec_0x76C);
        if ((u16)(ang + 0x8000) > 0x671B) {
            mot = em_get_mot_no(self);
            if (mot != 0x4A && mot != 0x57 && mot != 0x5B && mot != 0x63 && mot != 0xC8) {
                setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F78);
                get_joint_wmat_em(self, 0x16, &sp18);
                mulVecMat(&sp8, &sp18);
                sp8.x += sp18.m[0][3];
                sp8.y += sp18.m[1][3];
                sp8.z += sp18.m[2][3];
                if (em_water_check_view1(self) == 1U && sp8.y < self->field_0x210) {
                    if ((system_w.field_0x0c & 0x1F) == 0) {
                        if ((u16)(ang + 0x8000) > 0x6E38U) {
                            eft_spawn_type10(self, 3, 0x16, &sp8, lbl_8079800C);
                            return;
                        }
                        eft_spawn_type10(self, 5, 0x16, &sp8, lbl_8079800C);
                    }
                } else if ((system_w.field_0x0c & 0x1F) == 0) {
                    if ((u16)(ang + 0x8000) > 0x6E38U) {
                        eft_spawn_type10(self, 0x1D, 0x16, &sp8, lbl_80797E9C);
                        return;
                    }
                    eft_spawn_type10(self, 0x1C, 0x16, &sp8, lbl_80797E9C);
                }
            }
        }
    }
}

/* 0x8018D558 - the effect-spawn router: `arg1` picks the position (+0x210 height, a joint, +0x20C height),
 * `arg3` the joint (0xFF: the work), then `eft009_set_pos` or `eft009_spawn_at_joint`. */
void fn_8018D558(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5) {
    VEC3 vec;
    u8 var;

    var = arg2;
    VEC3_ctor(&vec);
    switch (arg1) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (arg2) {
            case 2:
                var = 0xC;
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xE, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, arg3, 0xE, arg4, arg5);
                }
                break;
            case 4:
            case 6:
                var = 0x13;
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xF, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, arg3, 0xF, arg4, arg5);
                }
                break;
            default:
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                }
                break;
            }
        } else {
            if ((u32)(arg2 - 12) <= 13) {
                return;
            }
            if (arg2 == 0x26) {
                return;
            }
            if (arg3 == 0xFF) {
                vec.x = self->pos.x;
                vec.y = lbl_80797EE8 + self->field_0x20C;
                vec.z = self->pos.z;
            }
        }
        if (arg3 == 0xFF) {
            eft009_set_pos(var, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
            return;
        }
        eft009_spawn_at_joint(self, arg3, var, arg4, arg5);
        return;
    case 1:
        if ((self->field_0x228 & 6) != 0) {
            if (arg2 == 0 || arg2 == 3) {
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0x11, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                    return;
                }
                eft009_spawn_at_joint(self, arg3, 0x11, arg4, arg5);
                return;
            }
            return;
        }
        if (arg3 == 0xFF) {
            copyVec3(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &vec);
        }
        vec.y = self->field_0x20C;
        eft_spawn_pos_in_area(&vec, self->area_no, arg2, arg4, arg5 * get_em_chg_scale(self));
        return;
    case 2:
        if (arg3 == 0xFF) {
            copyVec3(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &vec);
        }
        vec.y = self->field_0x20C;
        {
            f32 scale = arg5 * get_em_chg_scale(self);
            if (self->pos.y <= lbl_80798174 + self->field_0x20C) {
                eft_spawn_type11_view1(self, &vec, arg2, scale);
            }
        }
        return;
    default:
        return;
    }
}

/* 0x80191038 - the team-0x10 TEV tint stepper: fades the two +0x350/+0x352 highlight words and the
 * +0x354..+0x358 fade triple toward the team's key colour. */
void fn_80191038(_ENEMY_WORK* self) {
    _GXColor color;
    s16 v;
    u8 flag;

    if (self->team == 0x10) {
        flag = (self->field_0x1E2 - 2) == 0;
        if ((fn_8012EC3C_c1(self) - 1) == 0) {
            v = self->tev_0x350 - 2;
            self->tev_0x350 = v;
            if (v < 0xAA) {
                self->tev_0x350 = 0xAA;
            }
            v = self->tev_0x352 - 2;
            self->tev_0x352 = v;
            if (v < 0xB4) {
                self->tev_0x352 = 0xB4;
            }
        } else {
            v = self->tev_0x350 + 2;
            self->tev_0x350 = v;
            if (v > 0xFF) {
                self->tev_0x350 = 0xFF;
            }
            v = self->tev_0x352 + 2;
            self->tev_0x352 = v;
            if (v > 0xFF) {
                self->tev_0x352 = 0xFF;
            }
        }
        v = self->tev_0x350;
        color.r = (u8)v;
        color.g = (u8)v;
        color.b = (u8)v;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR3, &color);
        v = self->tev_0x352;
        color.r = (u8)v;
        color.g = (u8)v;
        color.b = (u8)v;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, &color);
        if (flag == 1) {
            v = self->tev_0x354 + 7;
            self->tev_0x354 = v;
            if (v > 0xFF) {
                self->tev_0x354 = 0xFF;
            }
            v = self->tev_0x356 + 6;
            self->tev_0x356 = v;
            if (v > 0xFF) {
                self->tev_0x356 = 0xFF;
            }
            v = self->tev_0x358 + 6;
            self->tev_0x358 = v;
            if (v > 0xFF) {
                self->tev_0x358 = 0xFF;
            }
        } else {
            v = self->tev_0x354 - 7;
            self->tev_0x354 = v;
            if (v < 0x28) {
                self->tev_0x354 = 0x28;
            }
            v = self->tev_0x356 - 6;
            self->tev_0x356 = v;
            if (v < 0x3C) {
                self->tev_0x356 = 0x3C;
            }
            v = self->tev_0x358 - 6;
            self->tev_0x358 = v;
            if (v < 0x3C) {
                self->tev_0x358 = 0x3C;
            }
        }
        color.r = (u8)self->tev_0x354;
        color.g = (u8)self->tev_0x356;
        color.b = (u8)self->tev_0x358;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR1, &color);
        if (em_flags836_ck(self, 1) == 1U) {
            if (self->tev_0x35E == 1) {
                color.r = 0xFF;
                color.g = 0xFF;
                color.b = 0xFF;
                color.a = 0;
            } else {
                color.r = 0x28;
                color.g = 0x3C;
                color.b = 0x3C;
                color.a = 0;
            }
            ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR1, &color);
        } else {
            ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR1, &color);
            self->tev_0x35E = flag;
        }
        ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
        if (fn_8012EC3C_c1(self) == 1U) {
            color.a = 0;
        } else if (em_alt_mode_ck_c1(self) == 1U) {
            color.a = (s32)(lbl_80797EB4 *
                            (lbl_807981BC * (lbl_80797E9C + fn_8005024C((u16)(system_w.field_0x0c * 0x2000))))) +
                      0xE1;
        } else {
            color.a = (s32)(lbl_80797EB4 *
                            (lbl_807981BC * (lbl_80797E9C + fn_8005024C((u16)(system_w.field_0x0c * 0x2000))))) +
                      0x87;
        }
        ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    }
}

/* 0x801913FC - per-(team,kind) "the effect may fire" predicate: 0 = no, 1 = the main window,
 * 2 = the secondary window. */
s32 fn_801913FC(_ENEMY_WORK* self, u8 arg1) {
    f32 d;
    f32 scale;

    switch (self->team) {
    case 16:
        switch (arg1) {
        case 0:
            d = self->vec_0x36C.y - self->pos.y;
            if (d >= lbl_80797EA0) {
                return 1;
            }
            if (d <= lbl_80797EE4) {
                return 2;
            }
            return 0;
        case 1:
            if (self->tev_0x35A <= 0) {
                return 1;
            }
            break;
        case 2:
            if (self->field_0x1E2 == 2) {
                scale = get_em_chg_scale(self);
                if (self->pos.y >= self->field_0x210 - (lbl_80797ED0 + fn_8013026C(self)) * scale) {
                    return 1;
                }
            }
            break;
        case 5:
            if (self->tev_0x35C > 0) {
                return 1;
            }
            break;
        }
        break;
    case 17:
        if (arg1 == 4 && self->field_0x011 != 0) {
            return 1;
        }
        break;
    case 21:
        switch (arg1) {
        case 3:
            if (fn_801321DC(self) == 1U) {
                return 1;
            }
            break;
        case 4:
            if (self->field_0x011 != 0) {
                return 1;
            }
            break;
        }
        break;
    }
    return 0;
}

/* 0x8018D8C8 - the per-(team, motion) frame-window driver: each `em_after_frame_check` window that fires
 * spawns through `fn_8018D558` or an effect call (`field_0x228 & 6` picks the large/air variant). */
void fn_8018D8C8(_ENEMY_WORK* self) {
    VEC3 sp8;
    VEC3 sp14;
    VEC3 sp20;
    VEC3 sp2C;
    f32 temp_f1;
    u16 temp_r3;
    u16 temp_r3_2;
    u16 temp_r3_3;
    u8 temp_r0;

    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    VEC3_ctor(&sp14);
    temp_r0 = self->team;
    switch ((s32) temp_r0) {
    case 16:
        fn_8018D370(self);
        temp_r3 = em_get_mot_no(self);
        switch (temp_r3) {
        case 0x2:
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_807980E0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            break;
        case 0x9:
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079800C);
            }
            break;
        case 0x15:
            if ((em_after_frame_check(self, 0, lbl_80797EEC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798188, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797FC8, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            break;
        case 0x16:
            if (em_after_frame_check(self, 0, lbl_807980DC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x18:
            if ((em_after_frame_check(self, 0, lbl_80797F7C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x19:
            if ((em_after_frame_check(self, 0, lbl_80797F7C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_8079819C, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            break;
        case 0x1B:
            if ((em_after_frame_check(self, 0, lbl_80797F70, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            break;
        case 0x2D:
            if (em_after_frame_check(self, 0, lbl_807981A4, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797E88);
                eft_em_spawn(self, 0x46, 0x15, &sp14, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_807981A8, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079804C);
            }
            break;
        case 0x33:
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                em_hit_window_set_default(self, 0, 9);
            }
            if (em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797F38, lbl_80797E88, lbl_80797E88);
                eft_em_spawn_joint(self, 8, 0xF, &sp14, 0x10, lbl_80797EC0);
                setVector3(&sp14, lbl_807981B0, lbl_80797E88, lbl_80797E88);
                eft_em_spawn_joint(self, 8, 0x11, &sp14, 0x3C, lbl_80797EB8);
            }
            break;
        case 0x34:
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                em_hit_window_set_default(self, 0, 0xA);
            }
            if (em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_807981B4, lbl_80797E88, lbl_80797E88);
                eft_em_spawn_joint(self, 0xD, 0xF, &sp14, 0x10, lbl_80797EC0);
                setVector3(&sp14, lbl_807981B0, lbl_80797E88, lbl_80797E88);
                eft_em_spawn_joint(self, 0xD, 0x11, &sp14, 0x3C, lbl_80797EB8);
            }
            break;
        case 0x39:
            if (em_after_frame_check(self, 0, lbl_807980DC, lbl_80797E88) == 1U) {
                fn_8011D448(self, 0, 0x26, 8, lbl_8079800C);
                sp14.x = lbl_80797E88;
                sp14.y = lbl_807981B8;
                sp14.z = lbl_80797F5C;
                shell_set_func_ptr->method_0x2C(self, 3, 1, &sp14, lbl_807981BC, 0xFFFF, shell_set_func_ptr);
            }
            break;
        case 0x3B:
            if (em_after_frame_check(self, 0, lbl_807981C0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, self->field_0x1C0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0xFU, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x3D:
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797E9C);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x3E:
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 0x1DU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x1DU, self->field_0x1C0, lbl_8079804C);
                }
            }
            break;
        case 0x43:
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x10, &sp14, 0x1E, lbl_8079800C);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x46:
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x47:
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x49:
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x4D:
            if ((s32) (self->flags_0x836 & 1) == 0) {
                if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                    fn_8011D690(self, 8, 0x28, lbl_80797E9C);
                }
                if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                    fn_8011D690(self, 9, 0x29, lbl_80797E9C);
                }
                if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                    setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_807981C8 * get_em_chg_scale(self));
                    fn_80117E58_c1(self, 1, &sp14, 3, lbl_8079800C * get_em_chg_scale(self));
                    setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797F58 * get_em_chg_scale(self));
                    rotVecY(&sp14, self->field_0x1C0);
                    addVec3(&sp8, &self->pos, &sp14);
                    copyVec3(&sp2C, copyVec3(&sp20, &sp8));
                    temp_f1 = lbl_807981CC * get_em_chg_scale(self);
                    sp2C.y -= temp_f1;
                    sp20.y += lbl_807981CC * get_em_chg_scale(self);
                    shell_set_func_ptr->method_0x30(self, 4, &sp2C, &sp20, lbl_807981BC, 0xFFFF, shell_set_func_ptr);
                }
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                em_camera_req(self, 0x28, 7);
            }
            break;
        case 0x4E:
            if ((em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0xF, &sp14, 0x10, lbl_8079800C);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x4F:
            if ((em_after_frame_check(self, 0, lbl_80797F08, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U)) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0xF, &sp14, 0x10, lbl_8079800C);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x50:
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_807981D4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            break;
        case 0x51:
            if (em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797EC4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0xE000, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0xE000, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981D8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797E9C);
            }
            break;
        case 0x52:
            if ((em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981DC, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            if ((em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981E0, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x55:
        case 0x56:
            if (em_after_frame_check(self, 0, lbl_807981E4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981EC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x57:
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981A8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x58:
            if (em_after_frame_check(self, 0, lbl_80797F90, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xEU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0xEU, 0, lbl_8079800C);
                }
                em_camera_req(self, 0xE, 1);
            }
            break;
        case 0x59:
            if ((em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F90, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 8U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 8U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x5B:
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981F0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
            }
            break;
        case 0x5C:
            if (em_after_frame_check(self, 0, lbl_80798154, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x26U, 0, lbl_80797EF8);
                    fn_8018D558(self, 0U, 0xFU, 0x26U, 0, lbl_80797EF8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x26U, 0, lbl_80797EF8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0x11U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x11U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_80797FE0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797FCC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0x5E:
            if ((em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981F4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798034, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797EB8);
            }
            break;
        case 0x5F:
            if ((em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981F4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798034, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            break;
        case 0x60:
            if (em_after_frame_check(self, 0, lbl_80797F98, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 6U, 0xDU, 0x2AAB, lbl_80797F0C);
            }
            break;
        case 0x61:
            if (em_after_frame_check(self, 0, lbl_80797F98, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 6U, 8U, 0xD555, lbl_80797F0C);
            }
            break;
        case 0x62:
            if ((em_after_frame_check(self, 0, lbl_80797EC8, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x28U, 0, lbl_80798090);
                    fn_8018D558(self, 0U, 0xFU, 0x28U, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x28U, 0, lbl_80798090);
                }
            }
            break;
        case 0x63:
            if (em_after_frame_check(self, 0, lbl_807981E4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981EC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x6F:
            if (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_807981F8, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_807981FC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797E88);
                eft_em_spawn(self, 0x46, 3, &sp14, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80798200, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x79:
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0x7A:
            if (em_after_frame_check(self, 0, lbl_80797FF4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 3U, self->field_0x1C0 + 0x4000, lbl_80797F0C);
            }
            break;
        case 0x7B:
            if (em_after_frame_check(self, 0, lbl_80797FA8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xFU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x7C:
            if (em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            if (em_after_frame_check(self, 0, lbl_80798204, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x7D:
            if (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 3U, self->field_0x1C0 + 0xC000, lbl_80797F0C);
            }
            break;
        case 0x7E:
            if (em_after_frame_check(self, 0, lbl_80797FA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80798134, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xFU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x7F:
            if (em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797EB8);
            }
            if (em_after_frame_check(self, 0, lbl_80798204, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            break;
        case 0x81:
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x11U, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x11U, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x82:
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80798090);
            }
            break;
        case 0x83:
            if (em_after_frame_check(self, 0, lbl_80798208, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x10U, 0xFU, 0, lbl_807981E8);
                } else {
                    fn_8018D558(self, 1U, 1U, 0xFU, self->field_0x1C0, lbl_80797E9C);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x84:
            if (em_after_frame_check(self, 0, lbl_80797F40, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x88:
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 2U, 3U, self->field_0x1C0, lbl_80797E9C);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x89:
            if (em_after_frame_check(self, 0, lbl_80797FF4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x8B:
            if (em_after_frame_check(self, 0, lbl_8079820C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 0xFU, self->field_0x1C0, lbl_80797E9C);
            }
            break;
        case 0xC8:
            if ((em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_8079820C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981E0, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80798188, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798210, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                em_camera_req(self, 8, 0);
            }
            if ((em_after_frame_check(self, 0, lbl_807981D4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80798214, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798218, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                em_camera_req(self, 0xD, 0);
            }
            break;
        case 0xC9:
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 4U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F70, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0xCA:
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, self->field_0x1C0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0xCB:
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xEU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0xCC:
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xEU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        }
        break;
    case 17:
        temp_r3_2 = em_get_mot_no(self);
        switch ((s32) temp_r3_2) {
        case 0x3B:
            if (em_after_frame_check(self, 0, lbl_807981C0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0x10U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x10U, self->field_0x1C0, lbl_80797EC0);
                }
            }
            break;
        case 0x3C:
            if ((em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U) && ((s32) (self->field_0x228 & 6) != 0)) {
                fn_8018D558(self, 0U, 0x11U, 0x10U, 0, lbl_8079804C);
            }
            break;
        case 0x3E:
            if (em_after_frame_check(self, 0, lbl_80798154, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x17U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x17U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x3F:
            if ((em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) && ((s32) (self->field_0x228 & 6) != 0)) {
                fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                fn_8018D558(self, 0x27U, 0x30U, 3U, 0, lbl_80797EB8);
            }
            break;
        case 0x5C:
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x17U, 0x8000, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0x17U, 0x8000, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x17U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 0x10U, 0, lbl_8079800C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x10U, 0x8000, lbl_80797E9C);
                }
            }
            break;
        case 0x81:
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x10U, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x82:
            if (em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x83:
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, 0, lbl_80797F0C);
                }
            }
            break;
        case 0x85:
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0xC9:
            if ((em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xCU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xCU, 0, lbl_8079804C);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 7U, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 0U, 7U, 0, lbl_8079804C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_8079819C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EC4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_8079821C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_80797F0C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797ECC, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_807981E8);
                }
            }
            break;
        }
        break;
    case 21:
        temp_r3_3 = em_get_mot_no(self);
        switch ((s32) temp_r3_3) {
        case 0x70:
            if (em_after_frame_check(self, 0, lbl_80797F40, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, lbl_80797EB8 * get_em_scale(self));
            }
            em_shake_req_set(self);
            break;
        case 0x73:
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_8079818C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, self->field_0x1C0, lbl_80798220);
                }
            }
            break;
        case 0x81:
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x10U, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x82:
            if (em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x83:
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, 0, lbl_80797F0C);
                }
            }
            break;
        case 0x85:
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0xCD:
        case 0xC9:
            if (em_after_frame_check(self, 0, lbl_80798224, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, lbl_807981BC * get_em_scale(self));
            }
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x68U, 0, lbl_807981BC * get_em_scale(self));
            }
            if ((em_after_frame_check(self, 3, lbl_80797F08, lbl_80798194) == 1U) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                eft009_spawn_at_joint(self, 0x13U, 0x69U, 0, lbl_807981BC * get_em_scale(self));
            }
            break;
        case 0xD0:
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x66U, 0, get_em_scale(self));
            }
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, lbl_80797EB8 * get_em_scale(self));
            }
            break;
        case 0xD2:
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 3U, 0, lbl_8079800C);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80798228);
                }
            }
            break;
        }
        break;
    }
    if ((u8) self->team == 0x10) {
        fn_80191EF8_view1(self);
        fn_80192080_view1(self);
    }
}

#pragma peephole off

extern "C" {
/* ------------------------------------------------------------------------------------------------ */
/* bodies, in address order                                                                          */
/* ------------------------------------------------------------------------------------------------ */

/* Picks the caller's class/state pair from the work's team, map key and action id. */
void fn_80191598(EmActWork* self, u8* out_class, u8* out_state) {
    switch (self->team) {
    case 16:
        em_move_mode_set_view1(self, 0);
        *out_class = 12;
        *out_state = 2;
        switch (stage_map_kind_get_view3(self->field_0x1E0)) {
        case 1:
            switch (self->act_id) {
            case 6:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 8;
                fn_80126324_view1(self, 14, 15, lbl_80797E88);
                break;
            case 7:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 13, 14, lbl_80797E88);
                break;
            }
            break;
        case 3:
            switch (self->act_id) {
            case 2:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 6, 4, lbl_80797E88);
                break;
            case 3:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 15, 16, lbl_80797E88);
                break;
            }
            break;
        case 9:
        case 11:
            if (self->act_id == 1) {
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 0, 1, lbl_80797E88);
            }
            break;
        }
        break;
    case 17:
        fn_80182D5C_view1();
        switch (stage_map_kind_get_view3(self->field_0x1E0)) {
        case 1:
            switch (self->act_id) {
            case 5:
            case 6:
            case 8:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 5;
                break;
            case 7:
            case 12:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 0;
                break;
            default:
                *out_class = 12;
                if (self->field_0x00A == 1) {
                    em_move_mode_set_view1(self, 2);
                    *out_state = 3;
                } else {
                    em_move_mode_set_view1(self, 0);
                    *out_state = 2;
                }
                break;
            }
            break;
        case 3:
            switch (self->act_id) {
            case 1:
            case 5:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 5;
                break;
            case 7:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 6;
                break;
            case 2:
            case 3:
            case 4:
            case 6:
            case 8:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 0;
                break;
            case 10:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 7;
                break;
            default:
                *out_class = 12;
                if (self->field_0x00A == 1) {
                    em_move_mode_set_view1(self, 2);
                    *out_state = 3;
                } else {
                    em_move_mode_set_view1(self, 0);
                    *out_state = 2;
                }
                break;
            }
            break;
        default:
            *out_class = 12;
            if (self->field_0x00A == 1) {
                em_move_mode_set_view1(self, 2);
                *out_state = 3;
            } else {
                em_move_mode_set_view1(self, 0);
                *out_state = 2;
            }
            break;
        }
        break;
    case 21:
        em_move_mode_set_view1(self, 4);
        *out_class = 12;
        *out_state = 1;
        break;
    }
}

/* Re-arms the action the work's map key and action id select. */
void fn_80191990(EmActWork* self) {
    u32 state = 0;

    switch (stage_map_kind_get_view3(self->field_0x1E0)) {
    case 3:
        switch (self->act_id) {
        case 1:
            state = 1;
            break;
        case 2:
            state = 2;
            break;
        case 3:
            switch (self->state_0x9F6) {
            case 2:
                state = 1;
                break;
            }
            break;
        }
        break;
    }
    if (state == 1) {
        em_move_mode_set_view1(self, 0);
        em_mot_set_view1(self, 1, 0, 0);
    } else if (state == 2) {
        em_move_mode_set_view1(self, 2);
        em_mot_set_view1(self, 0x28, 0, 0);
    }
}

/* Reports whether the work is in one of the two team-specific ready states. */
u32 fn_80191A6C(EmActWork* self) {
    if (self->team == 21) {
        if (self->state_0x1E2 == 4 && fn_801321DC_view1(self) == 1) {
            return 1;
        }
    } else if (self->state_0x1E2 == 2 && em_alt_mode_ck_view1(self) == 0) {
        return 1;
    }
    return 0;
}

/* Reports whether the work's state byte is clear. */
u32 fn_80191AD8(EmActWork* self) {
    return self->state_0x1E2 == 0;
}

/* Re-seats the work at its own position while the damage part admits the action. */
void fn_80191AE8(EmActWork* self, u32 kind) {
    switch ((u8)kind) {
    case 1:
        if (em_parts_damage_level_get((struct _ENEMY_WORK*)self, 1) == 2 && self->state_0x1E2 != 1) {
            fn_803B9BA0(self, &self->pos_0x188, 100);
        }
        break;
    }
}

/* Reports whether the work may run its current action: the team/state gate, the entry probe and the
 * two restart checks. */
s32 fn_80191B4C(EmActWork* self, u32 arg) {
    u8 kind = stage_map_kind_get_view3(self->field_0x1E0);
    s32 hit;
    u32 probe;

    if (self->team != 16) {
        return 0;
    }
    switch (kind) {
    case 1:
    case 3:
        break;
    default:
        return 0;
    }
    if (fn_80129D3C_c1(self) == 1) {
        return 1;
    }
    hit = 0;
    switch (kind) {
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
        switch (fn_80129DB8_view1(self)) {
        case 1:
            hit = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (hit == 0) {
        switch (kind) {
        case 1:
            probe = 12;
            break;
        case 3:
            probe = 4;
            break;
        default:
            probe = 0xFF;
            break;
        }
        if (fn_8012A014_view1(self, 0, probe, (u16)arg, 0, lbl_805AAA74) == 1) {
            return 1;
        }
    }
    if (fn_80129A70_view1(self, (u16)arg) == 1) {
        return 1;
    }
    return (fn_8012A204_view1(self) - 1) == 0;
}

/* Re-enters the user-data accessor's third flag state. */
void fn_80191CDC(EmUserData* self) {
    fn_8013A654_view1(self, 3);
}

/* Applies one user-data item's aim transform to the caller's matrix holder. */
void fn_80191CE4(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item) {
    MTX34 mtx;
    EmMtx33 dst;
    EmRotVec rot;
    EmActWork* work;

    work = (EmActWork*)self->work_0x04;
    MTX34_ctor(&mtx);
    fn_800516F0(&dst);
    switch (item->type_0x04) {
    case 0xFF:
        switch (kind) {
        case 16:
        case 18:
        case 20:
            fn_8008E8D0(holder, &work->aim_0x328.vec_0x1C);
            break;
        case 24:
            rot.x = work->aim_0x328.angle_0x00;
            rot.y = 0;
            rot.z = 0;
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        case 25:
            eft_rot_vec_copy(&rot, &work->aim_0x328.rot_0x04);
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        case 26:
            eft_rot_vec_copy(&rot, &work->aim_0x328.rot_0x10);
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        }
        break;
    }
}

/* Places the aimed cluster the item's index selects. */
void fn_80191E30(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item) {
    MTX34 mtx;
    u32 index;
    EmActWork* work;

    work = (EmActWork*)self->work_0x04;
    MTX34_ctor(&mtx);
    switch (item->type_0x04) {
    case 0xFF:
        switch (kind) {
        case 16:
            index = 0;
            break;
        case 18:
            index = 1;
            break;
        case 20:
            index = 2;
            break;
        default:
            return;
        }
        if (work->clusters_0x590[index].live_0x03 >= 1) {
            joint_mtx_load(holder, &mtx);
            fn_800FBB90(&mtx, &work->clusters_0x590[index].pos_0x24);
            joint_mtx_store(holder, &mtx);
        }
        break;
    }
}

/* Steps the five aim angles by their per-frame increments and clamps them. */
void fn_80191EF8(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    u32 v;

    if (em_alt_mode_ck_view1(self) == 1) {
        v = rec->angle_0x00 + 546;
        rec->angle_0x00 = v;
        if ((s16)v > 0) {
            rec->angle_0x00 = 0;
        }
        v = rec->rot_0x04.x + 455;
        rec->rot_0x04.x = v;
        if ((s16)v > 0) {
            rec->rot_0x04.x = 0;
        }
        v = rec->rot_0x04.y + 455;
        rec->rot_0x04.y = v;
        if ((s16)v > 0) {
            rec->rot_0x04.y = 0;
        }
        v = rec->rot_0x10.x + 455;
        rec->rot_0x10.x = v;
        if ((s16)v > 0) {
            rec->rot_0x10.x = 0;
        }
        v = rec->rot_0x10.y - 455;
        rec->rot_0x10.y = v;
        if ((s16)v < 0) {
            rec->rot_0x10.y = 0;
        }
    } else {
        v = rec->angle_0x00 - 546;
        rec->angle_0x00 = v;
        if ((s16)v < -5460) {
            rec->angle_0x00 = (u16)-5460;
        }
        v = rec->rot_0x04.x - 455;
        rec->rot_0x04.x = v;
        if ((s16)v < -4550) {
            rec->rot_0x04.x = (u16)-4550;
        }
        v = rec->rot_0x04.y - 455;
        rec->rot_0x04.y = v;
        if ((s16)v < -4550) {
            rec->rot_0x04.y = (u16)-4550;
        }
        v = rec->rot_0x10.x - 455;
        rec->rot_0x10.x = v;
        if ((s16)v < -4550) {
            rec->rot_0x10.x = (u16)-4550;
        }
        v = rec->rot_0x10.y + 455;
        rec->rot_0x10.y = v;
        if ((s16)v > 4551) {
            rec->rot_0x10.y = 4551;
        }
    }
}

/* Runs the aim vector's fade in or out and mirrors it into its follower. */
void fn_80192080(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    f32 v;

    if (fn_8012EC3C_view1(self) == 0) {
        v = rec->vec_0x1C.x - lbl_8079822C;
        rec->vec_0x1C.x = v;
        if (v > lbl_80797E9C) {
            rec->vec_0x1C.x = lbl_80797E9C;
        }
        rec->vec_0x1C.y = rec->vec_0x1C.x;
    } else {
        v = rec->vec_0x1C.x + lbl_8079822C;
        rec->vec_0x1C.x = v;
        if (v < lbl_8079800C) {
            rec->vec_0x1C.x = lbl_8079800C;
        }
        rec->vec_0x1C.y = rec->vec_0x1C.x;
    }
}

/* Fills in one effect request (id 0x17) from the default vector. */
void fn_80192108(EmEffRequest* out, u8 a, s16 b, s16 c) {
    if (lbl_80794AA0 == 0) {
        setVec3(&vec_default_80191598, lbl_80797E88, lbl_80797E88, lbl_80797EB4);
        lbl_80794AA0 = 1;
    }
    out->id_0x00 = 0x17;
    copyVec3(&out->pos_0x04, &vec_default_80191598);
    out->field_0x10 = a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* Releases a heap block when the flag asks for it, then hands the pointer back. */
void* fn_801921A8(void* p, u32 flag) {
    if (p != 0) {
        fn_8013918C_view1(p, 0);
        if ((s16)flag > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* Builds the five static vectors the effect requests start from. */
void fn_80192204(void) {
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;
    VEC3 v4;
    VEC3 v5;
    VEC3 v6;
    VEC3 v7;
    VEC3 v8;

    assignVec3_view1(&vec_pair_80191598_0.vec_0x00,
                setVec3(&v1, lbl_80797E88, lbl_807981C0, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_0.vec_0x0C,
                setVec3(&v2, lbl_80797E88, lbl_80797E88, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_1.vec_0x00,
                setVec3(&v3, lbl_80797E88, lbl_80798230, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_1.vec_0x0C,
                setVec3(&v4, lbl_80797E88, lbl_80797E88, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_2.vec_0x00,
                setVec3(&v5, lbl_80797E88, lbl_80797F80, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_2.vec_0x0C,
                setVec3(&v6, lbl_80797E88, lbl_80798234, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_3.vec_0x00,
                setVec3(&v7, lbl_80797E88, lbl_80797E88, lbl_80797EB4));
    assignVec3_view1(&vec_pair_80191598_3.vec_0x0C,
                setVec3(&v8, lbl_80797E88, lbl_80797F24, lbl_80797E88));
}
}

/* The unit's `.bss`: the four vector pairs and the default vector `fn_80192204` builds (`fn_80192108` copies
 * the default).  Names are GUESSes. */
EmVecPair vec_pair_80191598_0;  /* +0x806A7A00 */
EmVecPair vec_pair_80191598_1;  /* +0x806A7A18 */
EmVecPair vec_pair_80191598_2;  /* +0x806A7A30 */
EmVecPair vec_pair_80191598_3;  /* +0x806A7A48 */
VEC3 vec_default_80191598;      /* +0x806A7A60 */
