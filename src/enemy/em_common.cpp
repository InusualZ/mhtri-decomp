/* enemy/em_common.cpp - the enemy common control set: the `_ENEMY_WORK` accessors and status bookkeeping
 *   (0x801251D0-0x8012BA00), the motion frame cost `fn_8012BA00`, the action/status checks (`em_act_ck`,
 *   `em_area_ck`, `em_die_ck`, `em_work_die_ck`, `ana_em_ck`, `shibire_em_ck`, 0x8012BDF4-0x8012E968), the area/group
 *   wait set (0x8012E968-0x8012EC74), the per-motion frame-window and motion/effect helpers (`em_sleep_ck`,
 *   `em_frame_check`, `shuffle1`..`shuffle6`, the joint helpers, 0x8012EC74-0x80137604) and one enemy's per-motion
 *   action/rotation accessors (0x80137604-0x8013791C).
 * RANGE. .text 0x801251D0-0x8013791C (514 functions); .ctors 0x8056F318-0x8056F31C (`fn_801378A0`, the light-table
 *   install), .rodata 0x8056F8D0-0x8056F8E0, .data 0x805A0FB0-0x805A1358 (the motion-set defaults and action tables
 *   from `lbl_805A0FB0`), .bss 0x806A4560-0x806A4590 (the light table), .sdata 0x807919D0-0x807919F0,
 *   .sdata2 0x80796C50-0x80796D40, extab, extabindex.
 * SEAM. Probably more than one TU: the `.sdata2` run jumps `lbl_80796CB4` -> `lbl_80796CB8` somewhere in the window
 *   0x8012E968-0x8012EC74 (`tudiscover` prefers 0x8012E968; the referrer sets are disjoint and ordered), the one
 *   strong observation in the band; the int-to-float magic at 0x80796C68 is loaded on both sides of it.
 * NAMES. Runtime-dump names: `get_enemy_data`, `em_act_ck`, `em_area_ck`, `em_die_ck`, `em_work_die_ck`,
 *   `ana_em_ck`(`_sub`), `shibire_em_ck`(`_sub`), `em_sleep_ck`, `em_get_mot_no`, `em_frame_check`,
 *   `em_after_frame_check`, `em_water_check`, `em_magma_check`, `UpdateValue`, `shuffle1`..`shuffle6`, `CancelFade`,
 *   `get_joint_wmat_em`, `get_joint_wpos_em`, `get_em_scale`, `get_em_chg_scale`.  The dump's
 *   `J3DColorBlockLightOff::*`, `GoalOverlay::SceneCreated`, `JASDsp::TChannel::forceStop`, `FormationPos::__ct`,
 *   `nw4hbm::...::GetCharSpace`, `cGameSFX::Init` (0x8012554C), `homebutton::MotorCallback` (0x80126844),
 *   `JASSeqCtrl::setIntrMask` (0x8012CE8C), `DBClose` (0x8012B604), `CBGetBytesAvailableForRead` (0x8012C204) and
 *   `GXGetTexObjMipMap` (0x8013023C) hits are signature matches on 4- to 36-byte bodies, not evidence; every other
 *   non-`fn_` name is a GUESS from its body.
 *   GUESS (from each body and its callers): em_record_hit_ck, em_mini_hit_ck, em_captured_ck
 *   GUESS: em_mini_kill_kind_get, em_quest_element_set, em_quest_element_set_large
 * RESIDUALS. 379 rows unwritten in 23 runs, the largest 0x8012F3F4-0x80137604 (270 functions);
 *   `sweepcomments.py --unit enemy/em_common` lists them.
 *  - `fn_8012E040`, `fn_8012E5D4`: dense-case dispatch trees; retail shares one tail per constant return and lowers
 *    the quartet 0x24..0x27 with `subi`/`cmplwi`, every shape tried emits inline returns or a two-compare range test
 *    (docs/enemy.md);
 *  - `ana_em_ck`, `shibire_em_ck`: retail re-materialises the two `(u8)` argument masks at each call in the loop,
 *    ours hoists them;
 *  - `fn_8012BDF4`: retail's frame is 0x70 against our 0x50 (an f31 `psq_st` spill and an `f64` temporary), and ours
 *    forms `ainpc_w`'s address, which retail does not reference;
 *  - `fn_8012C3C8`, `fn_8012C4E8`: the distance test (`fn_80050EAC`, `fmuls`) is laid out with the opposite branch
 *    polarity;
 *  - `fn_8012C300`, `fn_8012C600`, `fn_8012C6F4`, `fn_8012C870`, `fn_8012CDF4`, `fn_8012CF04`, `fn_8012D3E0`: the
 *    `clrlwi` narrowing sits at a different point (at entry in one, at the use or store in the other);
 *  - `em_team_damage_under_ck`, `em_team_damage_over_ck`: retail keeps a dead loop counter (`li r4,0` +
 *    `addi r4,r4,1`) and loads `lbl_80796C68`/`lbl_80796C80` where ours names its own `.sdata2` magic;
 *    `em_motion_window_ck`: the same magic, and ours materialises the final test branch-free (`mfcr` + `extrwi`);
 *  - `em_sleep_ck`: retail shares one `return 0` through both `bne` arms, ours a `li r3,0` per arm;
 *    `fn_8012F39C`: the constant is loaded before the field (f0/f1 against retail's f2/f0);
 *  - `fn_8012D0B4`, `fn_8012E2A8`: ours compares unsigned where retail uses `cmpwi` (`fn_8012D0B4` also an extra
 *    `b`); `fn_8012C0EC`: the branch polarity and one reload differ; `fn_8012CF2C`: the +0x796 store is scheduled two
 *    instructions later; `fn_8012D188`: the two bytes load in the other order; `shibire_em_ck_sub`: ours moves r6
 *    earlier;
 *  - `fn_8012B86C`, `fn_8012C220`, `fn_8012E664`, `fn_8012E968`: register allocation only.
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.rodata`/`.sdata` claimed, not emitted; `.sdata2`/`.text`/extab/extabindex
 *   short of the claim.
 * SHAPES. `#pragma peephole off` from the first body to the end (retail keeps `clrlwi`/`rlwinm` + `cmpwi` and `clrlwi`
 *   + `slwi` unfused) and `#pragma fp_contract off` around `fn_8012BA00` (retail keeps `fmuls` + `fadds`); the
 *   `em_act_ck__FP11_ENEMY_WORKUcUc`-style map names are declared `extern "C"` under the map's spelling, so the
 *   parameter widths come from the codegen (retail masks them; a C++ declaration loses the `clrlwi`);
 *   `fn_80128590`/`fn_801285A0` take `u8`/`u16` so their stores stay unmasked; `fn_8012BA00`'s branch order, `u32`
 *   parameters, empty `else if (enemy->field_0x1E2 == 2)` arm and local order are load-bearing (docs/enemy.md);
 *   `fn_8012E968` reuses its `mode` argument as the table index; the callees the folded sources declared differently
 *   are called through cast macros (`<name>_viewN`) and the disagreeing header declarations are renamed away around
 *   their `#include`.
 */

#include "enemy/em_area_entry_tbl.h" /* em_area_entry_tbl (rule 2: the owner's header) */
#include "types.h"
#include "hud/em_net_send.h" /* the owner's leaf header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "Pl/plw.h"
#include "enemy/EmAreaEntry.h" /* EmAreaEntry (rule 1: one definition) */
#include "ef.h"
#include "enemy/EnemyData.h"
#include "enemy/fn_801251D0.h"
#define em_alt_mode_ck em_alt_mode_ck_hidden_fn_8012E968_h
#define fn_8012EC3C fn_8012EC3C_hidden_enemy_h
#include "unsplit/enemy.h"
#undef fn_8012EC3C
#undef em_alt_mode_ck
#include "unsplit/enemy_pool.h" /* the enemy band's .data pool labels, declared in the band header */
#include "Pl/Pl_master_ck.h"
#define em_act_ck__FP11_ENEMY_WORKUcUc em_act_ck__FP11_ENEMY_WORKUcUc_hidden_fn_8012BDF4_h
#define em_area_ck em_area_ck_hidden_fn_8012BDF4_h
#define fn_8012D0B4 fn_8012D0B4_hidden_fn_8012BDF4_h
#define fn_8012D188 fn_8012D188_hidden_fn_8012BDF4_h
#define fn_8012D1A8 fn_8012D1A8_hidden_fn_8012BDF4_h
#define fn_8012D23C fn_8012D23C_hidden_fn_8012BDF4_h
#define fn_8012D3E0 fn_8012D3E0_hidden_fn_8012BDF4_h
#define fn_8012D7FC fn_8012D7FC_hidden_fn_8012BDF4_h
#define fn_8012D8D0 fn_8012D8D0_hidden_fn_8012BDF4_h
#define fn_8012DB3C fn_8012DB3C_hidden_fn_8012BDF4_h
#define fn_8012E21C fn_8012E21C_hidden_fn_8012BDF4_h
#define fn_8012E5A8 fn_8012E5A8_hidden_fn_8012BDF4_h
#include "enemy/fn_8012BDF4.h"
#undef fn_8012E5A8
#undef fn_8012E21C
#undef fn_8012DB3C
#undef fn_8012D8D0
#undef fn_8012D7FC
#undef fn_8012D3E0
#undef fn_8012D23C
#undef fn_8012D1A8
#undef fn_8012D188
#undef fn_8012D0B4
#undef em_area_ck
#undef em_act_ck__FP11_ENEMY_WORKUcUc
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Pl/pl_act.h"
#include "Pl/pl_skill.h"
#define fn_8012BA00 fn_8012BA00_hidden_fn_8012BA00_h
#include "enemy/fn_8012BA00.h"
#undef fn_8012BA00
#include "ai/ainpc.h"   /* `_AINPC_W` (rule 1) */
#include "ai/ainpc_w.h" /* `ainpc_w`, owned by ai/ai_npc.cpp (rule 2) */
#include "enemy/fn_80138074.h"
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "enemy/em_pop.h" /* quest_element_state_find (the owner's header, rule 2) */
#include "ef/pRoot.h"
/* Callees the folded sources declared with different parameters: each call keeps its own view through a cast
 * (the same direct call). */
#define stage_map_kind_get_view1 ((u32 (*)(u32))stage_map_kind_get)
#define get_move_work_adrs_view5 ((void* (*)(u8))get_move_work_adrs)
#define get_move_work_adrs_view3 ((_ENEMY_WORK* (*)(u8))get_move_work_adrs)
#define get_move_work_adrs_view1 ((_ENEMY_WORK* (*)(u8))get_move_work_adrs)
#define em_mot_finished_ck_view5 ((u32 (*)(void))em_mot_finished_ck)
#define em_mot_finished_ck_view3 ((u32 (*)(void))em_mot_finished_ck)
#define em_mot_finished_ck_view1 ((u32 (*)(void))em_mot_finished_ck)
#define fn_8012EC3C_view7 ((u32 (*)(struct _ENEMY_WORK*))fn_8012EC3C)
#define fn_8012EC3C_view4 ((u32 (*)(struct _ENEMY_WORK*))fn_8012EC3C)
#define fn_8012EC3C_view1 ((u32 (*)(struct _ENEMY_WORK*))fn_8012EC3C)
#define fn_8012E5A8_view5 ((u32 (*)(struct _ENEMY_WORK*))fn_8012E5A8)
#define fn_8012E5A8_view3 ((u32 (*)(struct _ENEMY_WORK*))fn_8012E5A8)
#define fn_8012E5A8_view1 ((u32 (*)(struct _ENEMY_WORK*))fn_8012E5A8)
#define fn_8012E21C_view5 ((u32 (*)(u32, u32))fn_8012E21C)
#define fn_8012E21C_view3 ((u32 (*)(u32, u32))fn_8012E21C)
#define fn_8012E21C_view1 ((u32 (*)(u32, u32))fn_8012E21C)
#define fn_8012DB3C_view5 ((u32 (*)(struct _ENEMY_WORK*))fn_8012DB3C)
#define fn_8012DB3C_view3 ((u32 (*)(struct _ENEMY_WORK*))fn_8012DB3C)
#define fn_8012DB3C_view1 ((u32 (*)(struct _ENEMY_WORK*))fn_8012DB3C)
#define fn_8012D8D0_view5 ((u32 (*)(struct _ENEMY_WORK*))fn_8012D8D0)
#define fn_8012D8D0_view3 ((u32 (*)(struct _ENEMY_WORK*))fn_8012D8D0)
#define fn_8012D8D0_view1 ((u32 (*)(struct _ENEMY_WORK*))fn_8012D8D0)
#define fn_8012D7FC_view5 ((u32 (*)(struct _ENEMY_WORK*))fn_8012D7FC)
#define fn_8012D7FC_view3 ((u32 (*)(struct _ENEMY_WORK*))fn_8012D7FC)
#define fn_8012D7FC_view1 ((u32 (*)(struct _ENEMY_WORK*))fn_8012D7FC)
#define fn_8012D3E0_view4 ((u8 (*)())fn_8012D3E0)
#define fn_8012D3E0_view2 ((u8 (*)())fn_8012D3E0)
#define fn_8012D23C_view4 ((u32 (*)())fn_8012D23C)
#define fn_8012D23C_view2 ((u32 (*)())fn_8012D23C)
#define fn_8012D1A8_view5 ((s32 (*)(u8))fn_8012D1A8)
#define fn_8012D1A8_view3 ((s32 (*)(u8))fn_8012D1A8)
#define fn_8012D1A8_view1 ((s32 (*)(u8))fn_8012D1A8)
#define fn_8012D188_view5 ((s32 (*)(struct _ENEMY_WORK*, struct _ENEMY_WORK*))fn_8012D188)
#define fn_8012D188_view3 ((s32 (*)(struct _ENEMY_WORK*, struct _ENEMY_WORK*))fn_8012D188)
#define fn_8012D188_view1 ((s32 (*)(struct _ENEMY_WORK*, struct _ENEMY_WORK*))fn_8012D188)
#define fn_8012D0B4_view5 ((s32 (*)(struct _ENEMY_WORK*, void*))fn_8012D0B4)
#define fn_8012D0B4_view3 ((s32 (*)(struct _ENEMY_WORK*, void*))fn_8012D0B4)
#define fn_8012D0B4_view1 ((s32 (*)(struct _ENEMY_WORK*, void*))fn_8012D0B4)
#define fn_8012BA00_view1 ((s32 (*)(struct _ENEMY_WORK*, struct EnemyActionTable*, void*, s32, u16))fn_8012BA00)
#define fn_8012B9BC_view1 ((void (*)(_ENEMY_WORK*, u8, s32))fn_8012B9BC)
#define fn_8012B944_view1 ((s32 (*)(_ENEMY_WORK*, u8, s32))fn_8012B944)
#define fn_80050EF4_view1 ((f32 (*)(const VEC3*, const VEC3*))fn_80050EF4)
#define em_status_ck_view1 ((u32 (*)(_ENEMY_WORK*, u32))em_status_ck)
#define em_area_ck_view5 ((u32 (*)(struct _ENEMY_WORK*))em_area_ck)
#define em_area_ck_view3 ((u32 (*)(struct _ENEMY_WORK*))em_area_ck)
#define em_area_ck_view1 ((u32 (*)(struct _ENEMY_WORK*))em_area_ck)
#define em_alt_mode_ck_view5 ((u32 (*)(struct _ENEMY_WORK*))em_alt_mode_ck)
#define em_alt_mode_ck_view3 ((u32 (*)(struct _ENEMY_WORK*))em_alt_mode_ck)
#define em_alt_mode_ck_view1 ((u32 (*)(struct _ENEMY_WORK*))em_alt_mode_ck)
u32 em_sleep_ck(_ENEMY_WORK* enemy, u8 kind);

/* Callees outside this unit. */

/* `fn_80124C5C` (0x574 B), which `enemy/fn_8011D448.cpp` defines and whose header does not declare it. */
extern "C" void fn_80124C5C(u32 a, u32 b, u8 c);

/* The unit's own next symbols, defined in this TU. */
extern "C" void em_hit_window_set(struct _ENEMY_WORK* self, u8 a, u32 b, u32 c);


/* Pool literals (declared, never defined - playbook 29). */
extern "C" u8 lbl_807919D0;

/* A 4-byte table entry `fn_80127568` indexes: a value word with the payload at +0x2. */
struct _EM_REC4 {
    /* +0x0 */ u16 field_0x0;
    /* +0x2 */ u16 field_0x2;
}; /* size: 0x4 */

/* The status/motion accessors (0x80128A14..0x8012B9FC). */
extern "C" void fn_80128998(struct _ENEMY_WORK* self, u8 a, u8 b);
extern "C" void em_act_step_arm(struct _ENEMY_WORK* self, u8 a, u8 b, u32 c);

/* Types. */

/* A 3-float engine vector (`nw4r::math::VEC3`, the C spelling `VEC3`) comes from `nw4r/math.h`
 * (same layout: three `f32` at +0x00/+0x04/+0x08, size 0x0C). */

/* One motion set, the record `fn_8012BA00`'s second argument points at.  The three `.data` defaults
 * the callers fall back to fix the arrays' shapes: `lbl_805A0FF8` -> thresholds `lbl_805A0FB0`
 * (1000/2000/3000/5000), frames `lbl_805A0FC0` (5/3/2/1/0/-5/-30), extra `lbl_805A0FDC`
 * (0xC8/0/0/0/0/0/0), i.e. four band bounds and seven frame entries each.  size: 0x0C */
typedef struct MOTION_SET {
    /* +0x00 */ const f32* thresholds;   /* 4 entries: the upper bound of each distance band */
    /* +0x04 */ const s32* frames;       /* 7 entries: the frame delta of each band */
    /* +0x08 */ const s32* extra_frames; /* 7 entries: the deltas the per-type tests add */
} MOTION_SET;

extern "C" {
/* Callees under the map's spelling: a plain name as `symbols.txt` spells it, a mangled one written verbatim inside
 * `extern "C"`, so the identifier and the relocation pair with the map's symbol. */

 /* vector copy */
                    /* v = (0, 0, 0) */
extern s32 fn_802D2B78(_ENEMY_WORK* other, u16 mask); /* tests a bit of other's `+0x1EC` */

/* The pool constants the source reaches (0x80796C58..0x80796C90, this unit's `.sdata2`), declared and never
 * defined (playbook 29); the int-to-f64 magic at 0x80796C68 is the compiler's own entry. */
extern f32 lbl_80796C58; /* 0.0f */
extern f32 lbl_80796C70; /* 0.5f */
extern f32 lbl_80796C8C; /* 0.7f */
}

/* The player-side work record (`get_move_work_adrs(0)`/`(2)`, 0xB20 apart). */

/* `_PLW`, the player work record, comes from `Pl/plw.h` - one definition, in the owner's header (rule 1). */
/* The root object `get_move_work_adrs(0)` returns - the per-player status bytes that
 * `fn_8012D1A8` reads.  It is not the 0xB20 record array; the index is a byte offset into this one
 * object.
 * size: 0x22FD (lower bound; the per-player array bound is approximate) */
struct _PLAYER_ROOT {
    /* +0x00000 */ u8 unused_0x00000[0x22DD];
    /* +0x022DD */ u8 player_state[32];
};

/* The enemy's own action tables: `get_enemy_data(enemy)->field_0x0A0->field_0x024` points at three table pointers,
 * each replaced by a built-in default (`lbl_805A0FF8`, `lbl_805A1034`, `lbl_805A106C`) when the data has none. */

/* One action entry: two condition/value pairs, the value added to the attacker's damage when the
 * matching `Pl_*_condition_ck` passes.
 * size: 0x1C */
struct EnemyActionEntry {
    /* +0x000 */ u8 unused_0x000[0x00C];
    /* +0x00C */ s32 condition_0x00C;
    /* +0x010 */ s32 value_0x010;
    /* +0x014 */ s32 condition_0x014;
    /* +0x018 */ s32 value_0x018;
};

/* A whole action table; the entry the actor uses is at +0x08.
 * size: 0x0C */
struct EnemyActionTable {
    /* +0x000 */ u8 unused_0x000[0x008];
    /* +0x008 */ EnemyActionEntry* entry;
};

/* The three-table set the enemy carries.  It is written as an array so the built-in defaults
 * (`lbl_805A0FF8` and friends) share the type.
 * size: 0x0C */
struct EnemyActionSet {
    /* +0x000 */ EnemyActionTable* table_0x000;
    /* +0x004 */ EnemyActionTable* table_0x004;
    /* +0x008 */ EnemyActionTable* table_0x008;
};

/* The block at `get_enemy_data(enemy)->field_0x0A0`; only the action-set pointer is named.
 * size: 0x28 (lower bound) */
struct EnemyExtraData {
    /* +0x000 */ u8 unused_0x000[0x024];
    /* +0x024 */ EnemyActionSet* action_set;
};



/* Callees and pooled data. */
extern "C" f32 fn_80050EAC(void* ref, nw4r::math::VEC3* pos);
extern "C" u8 fn_80133BCC(void);
extern "C" void* quest_spawn_rec_find(u16 id);

extern "C" f32 fn_80050EF4(void* ref, nw4r::math::VEC3* pos);
extern "C" f32 calcVecDistXZ(void* ref, nw4r::math::VEC3* pos);
extern "C" s32 quest_time_limit_get(void);
extern "C" s32 quest_time_elapsed_get(void);

extern "C" s32 fn_8012D3E0(_ENEMY_WORK* enemy, u32 kind);
extern "C" u32 fn_8012D23C(_ENEMY_WORK* enemy, u32 kind, u32 slot);
extern "C" void* fn_8012D498(u32 selector, _PLW* work, s8* out_flag);
extern "C" s32 fn_8012D7FC(void* arg);
extern "C" s32 fn_8012D468(u32 selector, _PLW* work, s8* out_flag);
extern "C" s32 fn_8012D8D0(_ENEMY_WORK* enemy);
extern "C" u32 ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(_ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" void* ana_em_ck__FUcPQ34nw4r4math4VEC3fUc(u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" u32 shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(_ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" void* shibire_em_ck__FUcPQ34nw4r4math4VEC3fUc(u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" s32 fn_8012DB3C(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012DD58(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012DE78(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012DED8(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012DF68(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E040(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E0D8(u32 state, u32 action);
extern "C" s32 fn_8012E158(u32 state, u32 action);
extern "C" s32 fn_8012E21C(u32 state, u32 action);
extern "C" s32 fn_8012E2A8(u32 state, u32 sub);
extern "C" s32 fn_8012E2D4(u32 state, u32 action);
extern "C" s32 fn_8012E35C(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E464(_ENEMY_WORK* enemy, u32 flag);
extern "C" s32 fn_8012E548(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E5A8(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E5D4(u32 actor, u32 flags);
extern "C" s32 em_record_hit_ck(_ENEMY_WORK* record);
extern "C" s32 em_mini_hit_ck(_ENEMY_WORK* record);
extern "C" void fn_8012E664(_ENEMY_WORK* enemy);
extern "C" void fn_8012E694(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E6A0(u32 kind, u16 id);
extern "C" u8 em_captured_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_mini_kill_kind_get(_ENEMY_WORK* enemy);
extern "C" void fn_8012E8DC(_ENEMY_WORK* record);
extern "C" s32 fn_8012E8F4(f32 seconds);

extern "C" u8 stage_map_kind_get(u32 map_no);
extern "C" u32 fn_802B0688(void* pos);
extern "C" u8 Pl_area_flag_get(u32 value);
extern "C" void fn_8012B988(_ENEMY_WORK* enemy, s32 value);
extern "C" void fn_8012CE8C(_ENEMY_WORK* enemy, u16 flag);
extern "C" void fn_8012CE9C(_ENEMY_WORK* enemy, u32 flag);
extern "C" void fn_8012CDF4(_ENEMY_WORK* enemy, u32 state, u32 sub);
extern "C" u32 fn_8012CF04(_ENEMY_WORK* enemy, u32 flag);

/* Map-mangled callees, kept as the map spells them (see the signature note in the file header). */
u8 get_now_areano(void);
EnemyData* get_enemy_data(_ENEMY_WORK* enemy);
_PLW* get_move_work_adrs(u8 kind);
u16 get_move_work_max(u8 kind);

extern "C" EnemyActionTable* lbl_805A0FF8[4];
extern "C" EnemyActionTable* lbl_805A1034[3];
extern "C" EnemyActionTable* lbl_805A106C[3];

extern "C" f32 lbl_80796C58;
extern "C" f32 lbl_80796C90;
extern "C" f32 lbl_80796C94;

extern "C" s32 fn_8012D0B4(_ENEMY_WORK* enemy, _PLW* work);
extern "C" s32 fn_8012D1A8(u32 index);
extern "C" s32 fn_8012D188(_ENEMY_WORK* enemy, _PLW* work);
extern "C" u32 em_busy_ck(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012CF2C(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012CF90(_ENEMY_WORK* enemy, u32 value);
extern "C" u32 fn_8012C6F4(_ENEMY_WORK* enemy, u32 mask, s32 arg2);
extern "C" u32 em_act_ck__FP11_ENEMY_WORKUcUc(_ENEMY_WORK* enemy, u32 state, u32 sub);
extern "C" u32 fn_8012C870(_ENEMY_WORK* enemy, u32 mask);

/* `Screen_w` (0x8065903C, `mh3_pad.cpp`'s `.bss`): only the frame scale at +0x14 that `main.cpp` writes
 * (`sw->f20 = 60.0f / vcount`); `main.cpp`'s `ScreenWork` documents the rest.
 * size: 0x54 */
typedef struct ScreenFrameScale {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ f32 frame_scale;
    /* +0x18 */ u8 pad_0x18[0x54 - 0x18];
} ScreenFrameScale; /* size: 0x54 */

extern "C" ScreenFrameScale Screen_w;

/* The `.sdata2` pool literals this range reads, in address order (values from `main.elf`). */
extern "C" f32 lbl_80796C9C;   /* 3.0f - mode 0 */
extern "C" f32 lbl_80796CA0;   /* 60.0f - the frame scale every mode is multiplied by */
extern "C" f32 lbl_80796CA4;   /* 5.0f - mode 1 */
extern "C" f32 lbl_80796CA8;   /* 10.0f - mode 2 */
extern "C" f32 lbl_80796CAC;   /* 15.0f - mode 3 */
extern "C" f32 lbl_80796CB0;   /* 25.0f - mode 4 */
extern "C" f32 lbl_80796CB4;   /* 35.0f - mode 5 */




extern "C" f32 lbl_80796C58;   /* 0.0f  - `fn_8012EFDC`'s `team == 0xf` threshold */
extern "C" f32 lbl_80796C5C;   /* 100.0f - the frame-window denominator */

extern "C" f32 lbl_80796CB8;   /* 0.3f  */
extern "C" f32 lbl_80796CBC;   /* 0.2f  */
extern "C" f32 lbl_80796CC0;   /* 0.18f */
extern "C" f32 lbl_80796CC4;   /* 0.15f */
extern "C" f32 lbl_80796CC8;   /* 0.1f  */
extern "C" f32 lbl_80796CCC;   /* 0.05f */
extern "C" f32 lbl_80796CD0;   /* 0.9f  */

/* In-range helpers defined further down or unwritten, and the forward declaration of `fn_8012EC74`. */
extern "C" f32 fn_8012EC74(_ENEMY_WORK* self);
extern "C" u32 em_get_rank(_ENEMY_WORK* self);
extern "C" u32 fn_80130134(_ENEMY_WORK* self, u32 flag);
extern "C" f32 fn_8013032C(_ENEMY_WORK* self);
extern "C" u32 fn_80132270(_ENEMY_WORK* self);

/* The other in-range mangled entry point, at C++ scope for its mangling. */
u32 em_magma_check(_ENEMY_WORK* self);

/* Other units' C-linkage callees. */
extern "C" s32 fn_8011E640(_ENEMY_WORK* self, u32 mask);

extern "C" u32 quest_entry_active_ck(void);

/* Pooled data owned by other units: declared, never defined (playbook 29), so the load operands pair with the
 * target's pool relocations. */
extern f32 lbl_80796C58;
extern f32 lbl_80796D34;
extern f32 lbl_80796D38;
extern f32 lbl_80796D3C;

/* The four 0x0C-byte colour records `setVec3` rebuilds; the table's bytes belong to the data pass. */
extern nw4r::math::VEC3 lbl_806A4560[];

/* `get_move_work_adrs__FUc` / `get_move_work_max__FUc` (owner `ef/system_core.cpp`). */
u16 get_move_work_max(u8 area);

extern "C" {
/* The plain `fn_` callees and this unit's own definitions are `extern "C"`, so they keep the map's names
 * (playbook 42); their owners' headers do not declare them. */

/* this unit's own entry points, used before their definitions below */
void fn_801376B4(_ENEMY_WORK* self);
void fn_801376BC(_ENEMY_WORK* self, u8 flag);
void fn_801376DC(_ENEMY_WORK* self, u8 flag);

s16* fn_80126494(_ENEMY_WORK* self);

/* `enemy/em_common.cpp` */
u32 em_busy_ck(_ENEMY_WORK* self);

/* `ef/system_core.cpp` */
u32 my_player_no(void);

/* the 0x803xxxxx helpers the band shares */
void lb_sub0e_send(u8 a, u16 b, u8 c, u16 d);
}

#pragma peephole off

/* The written bodies, in address order. */
extern "C" void em_se_tbl_play(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b)
{
    fn_80124C5C((u32)self, (u32)tbl, (u8)a);
}

extern "C" void em_se_tbl_play_alt(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b)
{
    fn_80124C5C((u32)self, (u32)tbl, (u8)a);
}

extern "C" void fn_801252C0(struct _ENEMY_WORK* self, u8 a)
{
    self->owner_slot_0x1F6 = (s8)a;
    self->field_0x1F5 = 0;
    em_net_send(self, 2, 0);
}

extern "C" void fn_8012554C(struct _ENEMY_WORK* self)
{
    self->state_0x9F9 = 0;
    self->state_0x9FA = 0;
}

extern "C" u8 fn_80125F88(u32 idx)
{
    return lbl_805A1ADC[(u8)idx];
}

extern "C" u32 enemy_kind_same_ck(u32 a, u32 b)
{
    return fn_80125F88((u8)a) == fn_80125F88((u8)b);
}

extern "C" u32 fn_80125FF0(u32 a, u32 b)
{
    return (u8)stage_map_kind_get_view1((u8)a) == (u8)b;
}

extern "C" u8* fn_80126044(struct _ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);
    u8* result = fn_8014260C(data->field_0x0E);
    if (result != NULL) {
        return result + self->field_0x7C8 * 0x1C;
    }
    return lbl_805A1078;
}

extern "C" s32 fn_80126098(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x14;
}

extern "C" s32 fn_801260BC(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x1C;
}

extern "C" s32 fn_801260E0(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x20;
}

extern "C" s32 fn_80126104(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x24;
}

EnemyData* get_enemy_data(struct _ENEMY_WORK* work)
{
    return fn_80140C00(work->team, work->field_0x00A);
}

extern "C" void* fn_80126704(struct _ENEMY_WORK* self)
{
    if (self->field_0x60 != NULL) {
        return self->field_0x60;
    }
    return &lbl_807919D0;
}

extern "C" void fn_80126898(struct _ENEMY_WORK* self)
{
    if (self->field_0x018 > 0) {
        self->field_0x018--;
    }
}

extern "C" u32 fn_801269E8(u32* table, u32 a, u32 b)
{
    if ((u8)a == (u8)b) {
        return 1;
    }
    if ((u8)a == 255 || (u8)b == 255) {
        return 0;
    }
    s8* row = (s8*)table[(u8)a];
    if (row == NULL) {
        return 0;
    }
    return row[(u8)b] != -1;
}

extern "C" u16 fn_80127568(void* unused, struct _EM_REC4* table, u32 idx)
{
    if (table == NULL) {
        return 3;
    }
    return table[(u8)idx].field_0x2;
}

extern "C" u32 fn_80127CB0(struct _ENEMY_WORK* self, u32 idx)
{
    return self->values_0x868[(u8)idx];
}

extern "C" void fn_80128590(struct _ENEMY_WORK* self, u8 a, u8 b, u16 c)
{
    self->field_0x1EE = a;
    self->field_0x1EF = b;
    self->field_0x1F0 = c;
}

extern "C" void fn_801285A0(struct _ENEMY_WORK* self, u8 a, u8 b, u16 c)
{
    self->state = 0;
    self->state_0x006 = 0;
    self->state_0x007 = 0;
    self->action = a;
    self->state_sub = b;
    self->bits_0x1EC = c;
}

extern "C" void fn_801281EC(struct _ENEMY_WORK* self)
{
    self->field_0x1F4 = 1;
}

extern "C" void fn_801281F8(struct _ENEMY_WORK* self)
{
    self->field_0x1F4 = 0;
}

extern "C" u32 fn_80128204(struct _ENEMY_WORK* self)
{
    return self->field_0x1F4;
}

extern "C" void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    em_hit_window_set(self, (u8)a, b, 0);
}

extern "C" void fn_80129864(struct _ENEMY_WORK* self)
{
    if (self->value_0x452 < 27000) {
        self->value_0x452++;
    }
}

extern "C" void fn_80129984(struct _ENEMY_WORK* self)
{
    self->field_0x1FC = 0;
    self->field_0x1FE = 0;
    self->field_0x1FF = 255;
}

extern "C" u32 fn_8012B5C4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c)
{
    if (self->field_0x380 == (u8)a) {
        if (self->state_0x381 == (u8)b) {
            if (self->field_0x382 == (u8)c) {
                return 1;
            }
        }
    }
    return 0;
}

extern "C" void fn_8012B604(void)
{
}

extern "C" void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    fn_80128998(self, (u8)a, (u8)b);
}

extern "C" void fn_80128A30(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    em_act_step_arm(self, (u8)a, (u8)b, 1);
    fn_801281EC(self);
}

extern "C" void fn_80128A70(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    fn_80128A30(self, (u8)a, (u8)b);
}

extern "C" void fn_80128A8C(struct _ENEMY_WORK* self, u8 a, u8 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_step_arm(self, a, b, 1);
}

extern "C" void fn_80128AAC(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 2);
}

extern "C" void fn_80128ACC(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 5);
}

extern "C" void fn_80128AEC(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 0);
}

extern "C" void fn_801299D8(struct _ENEMY_WORK* self)
{
    if (--self->field_0x200 <= 0) {
        self->field_0x200 = 0;
    }
    if (--self->field_0x202 <= 0) {
        self->field_0x202 = 0;
    }
}

extern "C" u32 fn_80129A1C(void* unused, u32 a, u32 b, s16* timer)
{
    if (*timer <= 0) {
        if ((u16)a % 100 < (u16)b) {
            return 1;
        }
        *timer = 900;
    }
    return 0;
}

extern "C" u32 fn_8012A204(struct _ENEMY_WORK* self)
{
    if (self->value_0x44C <= 0
        || (self->field_0x43D != 1 && self->value_0x452 > self->value_0x44E)) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 2;
        self->field_0x1FF = 255;
        return 1;
    }
    return 0;
}

extern "C" u32 fn_8012B86C(struct _ENEMY_WORK* self)
{
    u8 mask = 0;
    for (u32 i = 0; i < 4; i++) {
        if (self->values_0x3A4[33 + i] >= 20000) {
            mask |= 1 << i;
        }
    }
    return mask;
}

extern "C" void fn_8012B8F8(struct _ENEMY_WORK* self)
{
    for (u32 i = 0; i < 4; i++) {
        if (self->values_0x3A4[33 + i] >= 20000) {
            self->values_0x3A4[33 + i] = 0;
        }
    }
}

extern "C" void fn_8012B944(struct _ENEMY_WORK* self, u32 a, s32 b)
{
    s32 v;
    self->values_0x390[(u8)a] += b;
    v = self->values_0x390[(u8)a];
    if (v > 20000) {
        self->values_0x390[(u8)a] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x390[(u8)a] = 0;
    }
}

extern "C" void fn_8012B988(struct _ENEMY_WORK* self, s32 b)
{
    s32 v = self->values_0x390[4] + b;
    self->values_0x390[4] = v;
    if (v > 20000) {
        self->values_0x390[4] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x390[4] = 0;
    }
}

extern "C" void fn_8012B9BC(struct _ENEMY_WORK* self, u32 a, s32 b)
{
    s32 v;
    self->values_0x3A4[(u8)a] += b;
    v = self->values_0x3A4[(u8)a];
    if (v > 20000) {
        self->values_0x3A4[(u8)a] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x3A4[(u8)a] = 0;
    }
}

#pragma fp_contract off

extern "C" {
/* The frame cost of motion `type` for slot `part`: the per-type extra cost, then the distance band
 * the distance to `other` falls in, plus a fraction of the last entry on the two flagged paths. */
s32 fn_8012BA00(_ENEMY_WORK* enemy, const MOTION_SET* motion, _ENEMY_WORK* other, u32 type, u32 part)
{
    s32 result = 0;
    const f32* thresholds;
    const s32* frames;
    u32 flag; /* "add the extra cost", set by each type's test; unset for any other type */
    u32 add;
    VEC3 vec;
    const s32* extra;
    f32 dist;

    VEC3_ctor(&vec);
    add = 0;

    thresholds = motion->thresholds;
    frames = motion->frames;

    switch ((u8)type) {
    case 1:
        flag = (fn_8012D0B4_view1(enemy, other) == 1);
        extra = motion->extra_frames;
        if (extra != 0) {
            if (fn_8026FE98(other, 0x200) != 0) {
                result = extra[0];
            }
            if ((other->flags_0x5C4 & 0xF) != 0) {
                result += extra[1];
            }
            if (flag == 1 && fn_8012D7FC_view1(other) == 1) {
                result += extra[2];
            }
        }
        if (other->field_0x009 == 3) {
            add = 1;
        }
        break;

    case 2:
        flag = (fn_8012D188_view1(enemy, other) == 1);
        copyVec3(&vec, &other->pos_0x178);
        extra = motion->extra_frames;
        if (extra != 0) {
            if (fn_802D2B78(other, 1) != 0) {
                result = extra[0];
            }
            if (other->flag_0x420 == 1 && other->count_0x422 > 0) {
                result += extra[1];
            }
        }
        if (other->state_0x170 == 2) {
            add = 1;
        }
        break;

    case 3:
        flag = (enemy->area_no == other->area_no);
        copyVec3(&vec, &other->pos);
        if (other->field_0x1E2 == 2) {
            add = 1;
        }
        break;
    }

    if (enemy->field_0x43D != 1) {
        result += frames[4];
    } else if (flag == 0 || em_sleep_ck(enemy, 0) == 1) {
        result += frames[5];
    } else {
        if ((u8)type == 1) {
            dist = enemy->frames_0x454[(u16)part];
        } else {
            dist = fn_80050EF4_view1(&enemy->pos, &vec);
        }

        if (dist < lbl_80796C58) {
            result += frames[5];
        } else if (dist <= thresholds[0]) {
            result += frames[0];
        } else if (dist <= thresholds[1]) {
            result += frames[1];
        } else if (dist <= thresholds[2]) {
            result += frames[2];
        } else if (dist <= thresholds[3]) {
            result += frames[3];
        } else {
            result += frames[4];
        }

        if (enemy->action == 7) {
            if (enemy->field_0x380 == (u8)type && enemy->field_0x382 == (u16)part) {
                if (enemy->field_0x1E2 == 2) {
                    result = (s32)((f32)result + lbl_80796C8C * (f32)frames[6]);
                } else {
                    result += frames[6];
                }
            }
        } else if (enemy->field_0x1E2 == 2 && add == 0) {
            result = (s32)((f32)result + lbl_80796C70 * (lbl_80796C8C * (f32)frames[6]));
        } else if (enemy->field_0x1E2 == 2) {
            /* the state is 2 and the slot is not flagged: nothing to add */
        } else if (add == 1) {
            result = (s32)((f32)result + lbl_80796C70 * (f32)frames[6]);
        }
    }

    return result;
}
}

#pragma fp_contract on


/* Scores every player-side work record against its action table and hands the result to
 * `fn_8012B944`; then, when the global state allows it, picks the enemy's target for `fn_8012B9BC`. */
extern "C" void fn_8012BDF4(_ENEMY_WORK* enemy)
{
    EnemyActionSet* set;
    EnemyActionTable* table;
    _PLW* work;
    _ENEMY_WORK* target;
    u16 max;
    u16 i;
    u16 enemy_max;
    u16 j;
    s32 value;

    set = get_enemy_data(enemy)->extra->action_set;
    if (set == NULL || (table = set->table_0x000) == NULL) {
        table = lbl_805A0FF8[0];
    }
    max = get_move_work_max(2);
    work = get_move_work_adrs(2);
    for (i = 0; i < max; i++) {
        if (work->slot_active != 0 && fn_8012D1A8(work->chunk_ofs) == 0) {
            value = fn_8012BA00_view1(enemy, table, work, 1, i);
            if (value > 0) {
                if (Pl_Skill_ck(work, 0xC) == 1) {
                    value = (s32)((f32)value * lbl_80796C90);
                }
                if (Pl_Skill_ck(work, 0xB) == 1) {
                    value = (s32)((f32)value * lbl_80796C94);
                    if (value <= 0) {
                        value = 1;
                    }
                }
            }
            fn_8012B944_view1(enemy, (u8)i, value);
        }
    }
    if (ainpc_w.active != 0) {
        if (ainpc_w.field_0x171 != 5) {
            if (set == NULL || (table = set->table_0x004) == NULL) {
                table = lbl_805A1034[0];
            }
            value = fn_8012BA00_view1(enemy, table, &ainpc_w, 2, 0);
        } else {
            value = -5;
        }
        fn_8012B988(enemy, value);
    }
    if (set == NULL || (table = set->table_0x008) == NULL) {
        table = lbl_805A106C[0];
    }
    enemy_max = get_move_work_max(3);
    target = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (j = 0; j < enemy_max; j++, target++) {
        if (enemy->group == j || target->active == 0 || enemy->team == target->team ||
            fn_8012CF90(enemy, target->team) == 1 || (u8)(target->action - 0x0B) <= 1) {
            enemy->values_0x3A4[j] = 0;
        } else {
            fn_8012B9BC_view1(enemy, (u8)j, fn_8012BA00_view1(enemy, table, target, 3, j));
        }
    }
}


/* The attacker's damage against one player-side record: the stored value plus the two table
 * adjustments whose conditions the player passes. */
extern "C" s32 fn_8012C0EC(_ENEMY_WORK* enemy, u32 index)
{
    EnemyActionSet* set;
    EnemyActionTable* table;
    EnemyActionEntry* entry;
    _PLW* work;
    s32 value;

    work = get_move_work_adrs(2);
    set = get_enemy_data(enemy)->extra->action_set;
    value = enemy->values_0x390[(u8)index];
    if (set == NULL || (table = set->table_0x000) == NULL) {
        table = lbl_805A0FF8[0];
    }
    work += (u8)index;
    if (work->slot_active != 0 && fn_8012D1A8(work->chunk_ofs) == 0 && fn_8012D0B4(enemy, work) == 1) {
        entry = table->entry;
        if (entry->condition_0x00C != 0 && Pl_condition_ck(work, entry->condition_0x00C) == 1) {
            value += entry->value_0x010;
        }
        if (entry->condition_0x014 != 0 && Pl_dm_condition_ck(work, entry->condition_0x014) == 1) {
            value += entry->value_0x018;
        }
    }
    if (value < 0) {
        value = 0;
    }
    return value;
}


/* The enemy's fourth per-attacker value. */
extern "C" s32 fn_8012C204(_ENEMY_WORK* enemy)
{
    return enemy->values_0x390[4];
}

/* The per-attacker value at `values_0x390[index + 5]`. */
extern "C" s32 fn_8012C20C(_ENEMY_WORK* enemy, u32 index)
{
    return enemy->values_0x3A4[(u8)index];
}


/* Claims the first enemy-side record of `team` that no other record's `state_sub` owns, marking it
 * with `flags_0xA04` bit 0 and returning 1; 0 when none is free. */
extern "C" s32 fn_8012C220(u32 team, u32 state_sub)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->action != 0x0B && work->action != 0x0C && work->team == (u8)team &&
            work->area_no != (u8)state_sub) {
            if ((work->flags_0xA04 & 1) == 0) {
                work->flags_0xA04 |= 1;
                work->field_0xA05 = state_sub;
                work->field_0xA06 = 0;
            }
            return 1;
        }
    }
    return 0;
}


/* Sets `flags_0xA04` bit 1 on every enemy-side record of `team` whose area matches `state_sub` and
 * returns 1 when at least one was marked. */
extern "C" s32 fn_8012C300(u32 team, u32 state_sub)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    u32 found;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    found = 0;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->action != 0x0B && work->action != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            work->flags_0xA04 |= 2;
            found = 1;
        }
    }
    return found;
}


/* Sets `flags_0xA04` bit 2 on every enemy-side record of `team` whose area matches `state_sub` and
 * whose position lies within `radius` of `ref`. */
extern "C" s32 fn_8012C3C8(u32 team, u32 state_sub, void* ref, f32 radius)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    u32 found;
    f32 zero;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    found = 0;
    zero = lbl_80796C58;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->action != 0x0B && work->action != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            if (ref == NULL || radius < zero ||
                fn_80050EAC(ref, &work->pos) <= radius * radius) {
                work->flags_0xA04 |= 4;
                found = 1;
            }
        }
    }
    return found;
}


/* Sets `flags_0xA04` bit 2 on every enemy-side record of `team` whose area matches `state_sub` and
 * whose position lies within `radius` of `ref`, and hands each one to `fn_80130858`. */
extern "C" void fn_8012C4E8(u32 team, u32 state_sub, s16 arg3, void* ref, f32 radius)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    f32 zero;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    zero = lbl_80796C58;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->action != 0x0B && work->action != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            if (ref == NULL || radius < zero ||
                fn_80050EAC(ref, &work->pos) <= radius * radius) {
                work->flags_0xA04 |= 4;
                fn_80130858(work, arg3);
            }
        }
    }
}


/* Runs the pending bits of `flags_0xA04` down: the sleep flag (bit 1) is cleared, and the stun flag
 * (bit 0) counts one frame before it is released. */
extern "C" void fn_8012C600(_ENEMY_WORK* enemy)
{
    if (enemy->flags_0xA04 != 0) {
        if ((enemy->flags_0xA04 & 2) != 0) {
            em_status_set(enemy, 1);
            enemy->flags_0xA04 &= 0xFD;
        }
        if ((enemy->flags_0xA04 & 1) != 0) {
            if (enemy->field_0xA06 == 0) {
                enemy->field_0xA06 = enemy->field_0xA06 + 1;
                fn_8012CE8C(enemy, 8);
            } else if (enemy->area_no == enemy->field_0xA05) {
                enemy->flags_0xA04 &= 0xFE;
                enemy->field_0xA05 = 0xFF;
                enemy->field_0xA06 = 0;
                fn_8012CE9C(enemy, 8);
            }
        }
        if ((enemy->flags_0xA04 & 4) != 0) {
            enemy->field_0xA07 = 1;
            enemy->flags_0xA04 &= 0xFB;
        }
    }
}


/* Latches the "hit by" byte unless the enemy is already in the state that consumes it. */
extern "C" void fn_8012C6DC(_ENEMY_WORK* enemy)
{
    if (enemy->field_0x43D != 1) {
        enemy->field_0x43A = 1;
    }
}


/* Marks the enemy as being hit for the given attacker: clears `field_0x798` and, when `mask` names
 * any live attacker, picks the last one whose bit is set, then runs the hit state machine. */
extern "C" u32 fn_8012C6F4(_ENEMY_WORK* enemy, u32 mask, s32 arg2)
{
    _PLW* work;
    u16 max;
    u8 i;

    if ((enemy->field_0x1C8 & 0x80) != 0) {
        return 0;
    }
    if (enemy->field_0x95C == 5) {
        return 0;
    }
    em_status_set(enemy, 1);
    if (fn_8013A884(enemy, 5) == 1) {
        max = get_move_work_max(2);
        work = get_move_work_adrs(2);
        enemy->field_0x798 = 0xFF;
        if ((u8)mask != 0xFF) {
            for (i = 0; i < max; i++, work++) {
                if (work->slot_active != 0 && fn_8012D1A8(work->chunk_ofs) == 0 &&
                    ((u8)mask & (1 << i)) != 0 && fn_8012D0B4(enemy, work) != 0) {
                    enemy->field_0x798 = i;
                    break;
                }
            }
        }
        if (arg2 == 0) {
            fn_8012CDF4(enemy, 5, 0);
        } else if (fn_8013A8B4(enemy, 5, 1) == 1) {
            fn_8012CDF4(enemy, 5, 1);
        } else {
            fn_8012CDF4(enemy, 5, 0);
        }
        return 1;
    }
    return 0;
}


/* The knock-down variant of `fn_8012C6F4`: gated on the shell timer, it picks the last live attacker
 * out of `mask` and enters the knock-down state. */
extern "C" u32 fn_8012C870(_ENEMY_WORK* enemy, u32 mask)
{
    _PLW* work;
    u16 max;
    u8 i;

    if (enemy->field_0x95C == 4) {
        return 0;
    }
    if (enemy->field_0x916 > 0) {
        return 0;
    }
    if ((enemy->field_0x38C & 1) == 0) {
        return 0;
    }
    em_status_set(enemy, 2);
    if (fn_8013A884(enemy, 4) == 1) {
        max = get_move_work_max(2);
        work = get_move_work_adrs(2);
        enemy->field_0x798 = 0xFF;
        for (i = 0; i < max; i++, work++) {
            if (work->slot_active != 0 && fn_8012D1A8(work->chunk_ofs) == 0 &&
                ((u8)mask & (1 << i)) != 0 && fn_8012D0B4(enemy, work) != 0) {
                enemy->field_0x798 = i;
                break;
            }
        }
        fn_8012CDF4(enemy, 4, 0);
        return 1;
    }
    return 0;
}


/* Switches the enemy to the given state pair, saving the previous state and its label first. */
extern "C" void fn_8012CDF4(_ENEMY_WORK* enemy, u32 state, u32 sub)
{
    enemy->field_0x9DC = enemy->field_0x95C;
    enemy->field_0x9DD = enemy->field_0x95D;
    enemy->field_0x9E0 = enemy->field_0x958;
    enemy->field_0x9E4 = fn_80133BCC();
    enemy->field_0x95C = state;
    enemy->field_0x95D = sub;
    enemy->field_0x958 = enemy->field_0x954[(u8)state][(u8)sub];
    enemy->field_0x79A = 1;
    fn_80133BB4(enemy);
}


/* Sets the given bits in the per-attacker flag word. */
extern "C" void fn_8012CE8C(_ENEMY_WORK* enemy, u16 flag)
{
    enemy->field_0x794 |= flag;
}

/* Clears the given bits in the per-attacker flag word. */
extern "C" void fn_8012CE9C(_ENEMY_WORK* enemy, u32 flag)
{
    enemy->field_0x794 &= ~flag;
}

/* Arms the "hit by" timer and remembers which attack it was, then marks the state word. */
extern "C" void fn_8012CEB4(_ENEMY_WORK* enemy, s16 timer, u8 index)
{
    fn_8012CE8C(enemy, 2);
    enemy->field_0x796 = timer;
    enemy->field_0x799 = index;
}

/* Whether any of the given bits is set in the per-attacker flag word. */
extern "C" u32 fn_8012CF04(_ENEMY_WORK* enemy, u32 flag)
{
    return (enemy->field_0x794 & flag) != 0;
}

/* Marks the enemy busy for this frame. */
extern "C" void em_busy_set(_ENEMY_WORK* enemy)
{
    enemy->field_0x79B = 1;
}


/* Runs the "hit by" timer down and reports 1 on the frame it expires. */
extern "C" s32 fn_8012CF2C(_ENEMY_WORK* enemy)
{
    s16 timer;

    if (fn_8012CF04(enemy, 2) == 1) {
        timer = enemy->field_0x796 - 1;
        enemy->field_0x796 = timer;
        if (timer <= 0) {
            enemy->field_0x796 = 0;
            return 1;
        }
    }
    return 0;
}


/* Whether the enemy's own value list contains `value` (0x28 is always a hit). */
extern "C" s32 fn_8012CF90(_ENEMY_WORK* enemy, u32 value)
{
    u8* list = get_enemy_data(enemy)->values;

    if ((u8)value == 0x28) {
        return 1;
    }
    if (list != NULL) {
        while (*list != 0) {
            if (*list == (u8)value) {
                return 1;
            }
            list++;
        }
    }
    return 0;
}


/* Whether the enemy record is still alive: an inactive record, or one whose death flag has run past
 * 2, counts as dead. */
s32 em_work_die_ck(_ENEMY_WORK* enemy)
{
    if (enemy->active != 0 && enemy->field_0x004 < 2) {
        return 0;
    }
    return 1;
}

/* Whether the enemy is in its death state. */
s32 em_die_ck(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0B || em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xC, 0xFF) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the area the game is currently in. */
s32 em_area_ck(_ENEMY_WORK* enemy)
{
    return get_now_areano() == enemy->area_no;
}

/* Whether the enemy and the given work record share an area. */
extern "C" s32 fn_8012D188(_ENEMY_WORK* enemy, _PLW* work)
{
    return work->effect_key_0x1A4 == enemy->area_no;
}

/* The enemy's activity byte. */
extern "C" u32 em_busy_ck(_ENEMY_WORK* enemy)
{
    return enemy->field_0x1F5;
}

/* Whether the given player slot is in the "out of action" state; only player 0 is looked at. */
extern "C" s32 fn_8012D1A8(u32 index)
{
    _PLAYER_ROOT* root;

    if (isServerSelectState() == 1) {
        root = (_PLAYER_ROOT*)get_move_work_adrs(0);
        if (root != NULL && root->player_state[(u8)index] == 4) {
            return 1;
        }
    }
    return 0;
}

/* Whether the enemy is in the given state pair. */
extern "C" u32 em_act_ck__FP11_ENEMY_WORKUcUc(_ENEMY_WORK* enemy, u32 state, u32 sub)
{
    if (enemy->action == (u8)state && enemy->state_sub == (u8)sub) {
        return 1;
    }
    return 0;
}


/* Whether the enemy may attack the given work record: same area, not already out, and past the
 * map-specific position gate. */
extern "C" s32 fn_8012D0B4(_ENEMY_WORK* enemy, _PLW* work)
{
    if (fn_8012D1A8(work->chunk_ofs) == 1) {
        return 0;
    }
    if (work->field_0x00A == 8) {
        return 0;
    }
    if (enemy->area_no == work->area_0x16) {
        if (stage_map_kind_get(enemy->field_0x1E0) == 9) {
            if (Pl_area_flag_get(work->chunk_ofs) == 0) {
                if (fn_802B0688(&enemy->pos) == 1) {
                    return 1;
                }
            } else if (fn_802B0688(&enemy->pos) == 0) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x8012D23C is not written yet (420 bytes, a two-level switch over the state byte and the state_sub timer).  Its
 * per-slot predicate `fn_8012D3E0` and the search helpers below are. */

/* The first attacker slot whose per-kind predicate `fn_8012D23C` accepts the enemy. */
extern "C" s32 fn_8012D3E0(_ENEMY_WORK* enemy, u32 kind)
{
    u16 max;
    s32 i;

    max = get_move_work_max(2);
    for (i = 0; i < max; i++) {
        if (fn_8012D23C(enemy, kind, (u8)i) == 1) {
            return (u8)i;
        }
    }
    return 0xFF;
}

/* Whether the search helper finds any record. */
extern "C" s32 fn_8012D468(u32 selector, _PLW* work, s8* out_flag)
{
    return fn_8012D498((u8)selector, work, out_flag) != NULL;
}


/* Whether the actor has the "charge" skill, or the pair is on its last frame. */
extern "C" s32 fn_8012D7FC(void* arg)
{
    if (Pl_Skill_ck((_PLW*)arg, 0xCD) == 1) {
        return 1;
    }
    return (fn_8027AC18(arg) - 1) == 0;
}

/* Whether the enemy is in the low part of its action state. */
extern "C" s32 fn_8012D850(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0A && enemy->state_sub <= 0x12) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the mid-range part of its action state. */
extern "C" s32 fn_8012D878(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0A && (u32)(enemy->state_sub - 0x58) <= 0x1F) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the high part of its action state. */
extern "C" s32 fn_8012D8A4(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0A && (u32)(enemy->state_sub - 0xC3) <= 0x0F) {
        return 1;
    }
    return 0;
}

/* The "analysable" action-state list `ana_em_ck` accepts. */
extern "C" s32 fn_8012D8D0(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB7) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBB) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBC) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB9) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBA) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x0F) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1C) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x24) == 1) {
        return 1;
    }
    return 0;
}

/* The "paralysed" action-state list. */
extern "C" s32 fn_8012DB3C(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xC0) == 1) {
        return 1;
    }
    return 0;
}

/* The "analysable enemy at position" and "paralysed enemy at position" searches. */

/* Whether one record is an analysable enemy of the given area within `radius` of `pos`. */
extern "C" u32 ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(
    _ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    if (enemy->active == 0) {
        return 0;
    }
    if (enemy->area_no != (u8)area) {
        return 0;
    }
    if ((u8)flag == 0 && enemy->action == 0x0B) {
        return 0;
    }
    if (fn_8012D8D0(enemy) == 0) {
        return 0;
    }
    return calcVecDistXZ(pos, &enemy->pos) <= radius;
}

/* The first analysable enemy of the given area within `radius` of `pos`. */
extern "C" void* ana_em_ck__FUcPQ34nw4r4math4VEC3fUc(
    u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    _ENEMY_WORK* work;
    s16 i;
    u16 max;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(work, (u8)area, pos, (u8)flag,
                                                                radius) == 1) {
            return work;
        }
    }
    return NULL;
}

/* Whether one record is a paralysed enemy of the given area within `radius` of `pos`. */
extern "C" u32 shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(
    _ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    if (enemy->active == 0) {
        return 0;
    }
    if (enemy->area_no != (u8)area) {
        return 0;
    }
    if (fn_8012DB3C(enemy) == 0 &&
        ((u8)flag == 0 || (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1A) == 0 &&
                           em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1B) == 0))) {
        return 0;
    }
    if (enemy->field_0x1E2 == 2) {
        if (fn_80050EF4(pos, &enemy->pos) <= radius) {
            return 1;
        }
    } else if (calcVecDistXZ(pos, &enemy->pos) <= radius) {
        return 1;
    }
    return 0;
}

/* The first paralysed enemy of the given area within `radius` of `pos`. */
extern "C" void* shibire_em_ck__FUcPQ34nw4r4math4VEC3fUc(
    u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    _ENEMY_WORK* work;
    s16 i;
    u16 max;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(work, (u8)area, pos,
                                                                    (u8)flag, radius) == 1) {
            return work;
        }
    }
    return NULL;
}

/* The action-state list predicates. */

/* The "sleeping" action-state list. */
extern "C" s32 fn_8012DD58(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE8) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE9) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE7) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEA) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEE) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEC) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xED) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in action state 0xF1 or 0xF2 (an `em_act_ck` list). */
extern "C" s32 fn_8012DE78(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF1) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF2) == 1) {
        return 1;
    }
    return 0;
}

/* The "waking up" action-state list. */
extern "C" s32 fn_8012DED8(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF3) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF5) == 1) {
        return 1;
    }
    return 0;
}

/* The "stunned" action-state list. */
extern "C" s32 fn_8012DF68(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xAF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB0) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB1) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB2) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB3) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB5) == 1) {
        return 1;
    }
    return 0;
}

/* Three-way action-state query: 0/1 for the two states it knows, 0xFF otherwise. */
extern "C" s32 fn_8012E040(_ENEMY_WORK* enemy)
{
    switch (enemy->action) {
    case 0x0A:
        if ((u32)(enemy->state_sub - 0xE6) <= 3) {
            return 0;
        }
        if ((u32)(enemy->state_sub - 0xEC) <= 3) {
            return 1;
        }
        if (enemy->state_sub == 0xE4) {
            return 0;
        }
        if (enemy->state_sub == 0xEA) {
            return 1;
        }
        return 0xFF;
    case 0x0B:
        if (enemy->state_sub == 0x34 || enemy->state_sub == 0x36) {
            return 0;
        }
        if (enemy->state_sub == 0x35 || enemy->state_sub == 0x37) {
            return 1;
        }
        return 0xFF;
    }
    return 0xFF;
}

/* The per-state action id tests. */

/* Whether the action id is one of the "frontal" attacks. */
extern "C" s32 fn_8012E0D8(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7C:
        case 0x82:
        case 0x8B:
        case 0x95:
        case 0x9C:
        case 0xBB:
        case 0xE8:
        case 0xEE:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "rear" attacks. */
extern "C" s32 fn_8012E158(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7A:
        case 0x7B:
        case 0x80:
        case 0x81:
        case 0x89:
        case 0x8A:
        case 0x93:
        case 0x94:
        case 0x9A:
        case 0x9B:
        case 0xB9:
        case 0xBA:
        case 0xE6:
        case 0xE7:
        case 0xEC:
        case 0xED:
        case 0xF8:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "side" attacks. */
extern "C" s32 fn_8012E21C(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7B:
        case 0x81:
        case 0x8A:
        case 0x94:
        case 0x9B:
        case 0xBA:
        case 0xE7:
        case 0xED:
        case 0xF8:
            return 1;
        }
    }
    return 0;
}

/* Whether the enemy is in the three-state part of the second action state. */
extern "C" s32 fn_8012E2A8(u32 state, u32 sub)
{
    if ((u8)state == 0xB && (u32)((u8)sub - 0x1A) <= 2) {
        return 1;
    }
    return 0;
}

/* Whether the action id is one of the "leap" attacks. */
extern "C" s32 fn_8012E2D4(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7D:
        case 0x7E:
        case 0x7F:
        case 0x83:
        case 0x8C:
        case 0x96:
        case 0x9D:
        case 0xBC:
        case 0xE9:
        case 0xEF:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "roar" attacks. */
extern "C" s32 fn_8012E35C(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x84) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x85) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x86) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x87) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x88) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x97) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x98) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x99) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x9E) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the action id is one of the "ball" attacks of the given flavour. */
extern "C" s32 fn_8012E464(_ENEMY_WORK* enemy, u32 flag)
{
    if ((u8)flag == 0) {
        if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD6) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD7) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD8) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD9) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDA) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDB) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDC) == 1) {
            return 1;
        }
    }
    return 0;
}

/* The two-state action list around the "poison" attacks. */
extern "C" s32 fn_8012E548(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBD) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBE) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the four-state part of the second action state. */
extern "C" s32 fn_8012E5A8(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0B && (u32)(enemy->state_sub - 0x10) <= 3) {
        return 1;
    }
    return 0;
}

/* Per-actor hit/guard flags: 0x22..0x27 want the "guard" bit, 0x1B wants exactly 3. */
extern "C" s32 fn_8012E5D4(u32 actor, u32 flags)
{
    if ((u32)((u8)actor - 0x24) <= 3) {
        if ((flags & 2) != 0) {
            return 1;
        }
        return 0;
    }
    switch ((u8)actor) {
    case 0x22:
        if ((flags & 2) != 0) {
            return 1;
        }
        return 0;
    case 0x1B:
        if (flags == 3) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* The hit test for the actor/mode pair the record carries. */
extern "C" s32 em_record_hit_ck(_ENEMY_WORK* record)
{
    return fn_8012E5D4(record->team, record->field_0x00A);
}

/* The hit test for the record's secondary actor/mode pair. */
extern "C" s32 em_mini_hit_ck(_ENEMY_WORK* record)
{
    return fn_8012E5D4(record->group, record->team);
}

/* Arms the attack timer and, when it was not already armed, tells the attack system about it. */
extern "C" void fn_8012E664(_ENEMY_WORK* enemy)
{
    if (enemy->field_0x011 == 0) {
        enemy->field_0x011 = 1;
        if ((enemy->field_0x1C8 & 8) == 0) {
            fn_80144584(3);
        }
    }
}

/* Marks the attack timer as expired. */
extern "C" void fn_8012E694(_ENEMY_WORK* enemy)
{
    enemy->field_0x011 = 2;
}

/* Whether the given actor id has a live hit record. */
extern "C" s32 fn_8012E6A0(u32 kind, u16 id)
{
    u8* entry;

    if (Pl_motion_input_ck(0) == 1) {
        return 0;
    }
    entry = (u8*)quest_spawn_rec_find(id);
    if (entry == NULL || (s16)((u16*)entry)[2] <= 0) {
        return 0;
    }
    return 1;
}


/* Whether the enemy is in the "charge" state pair. */
extern "C" u8 em_captured_ck(_ENEMY_WORK* enemy)
{
    return (u8)((fn_8012E2A8(enemy->action, enemy->state_sub) - 1) == 0);
}

/* The two-state mask the record's mode byte selects. */
extern "C" s32 em_mini_kill_kind_get(_ENEMY_WORK* enemy)
{
    return (enemy->field_0x014 == 2) ? 2 : 0;
}

/* Clears the "attack finished" byte once it has been consumed. */
extern "C" void fn_8012E8DC(_ENEMY_WORK* record)
{
    if (record->field_0x008 == 5) {
        record->field_0x008 = 0;
    }
}

/* Whether more than `seconds` have passed since the last frame stamp. */
extern "C" s32 fn_8012E8F4(f32 seconds)
{
    s32 now = quest_time_limit_get();

    return (f32)(now - quest_time_elapsed_get()) > seconds;
}

/* The mode-dispatched predicate (see the file header). */
extern "C" s32 fn_8012E968(_ENEMY_WORK* self, u8 mode)
{
    _ENEMY_WORK* work;
    EmAreaEntry* entry;
    u16 max;
    s32 idx;

    switch (mode) {
    case 0:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796C9C);
    case 1:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CA4);
    case 2:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CA8);
    case 3:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CAC);
    case 4:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CB0);
    case 5:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CB4);
    case 6:
        work = (_ENEMY_WORK*)get_move_work_adrs_view1(3);
        max = get_move_work_max(3);
        for (idx = 0; idx < max; idx++) {
            if (work->active != 0 && work->area_no == self->field_0x00A &&
                work->team == self->group) {
                return 0;
            }
            work++;
        }
        /* The table walk: 0x60 / 3 = the 32 groups the table holds, four records per group.  `mode`
         * is the target's own counter (see the file header, residual). */
        for (entry = &em_area_entry_tbl[0][0], mode = 0; mode < 0x60; mode += 3, entry += 4) {
            if (entry[0].active != 0 && entry[0].area_no == self->field_0x00A &&
                entry[0].group == self->group && entry[0].field_0x0C == 1) {
                return 0;
            }
            if (entry[1].active != 0 && entry[1].area_no == self->field_0x00A &&
                entry[1].group == self->group && entry[1].field_0x0C == 1) {
                return 0;
            }
            if (entry[2].active != 0 && entry[2].area_no == self->field_0x00A &&
                entry[2].group == self->group && entry[2].field_0x0C == 1) {
                return 0;
            }
            if (entry[3].active != 0 && entry[3].area_no == self->field_0x00A &&
                entry[3].group == self->group && entry[3].field_0x0C == 1) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

/* Whether `field_0x89F` is 2 or 3 (`enemy/em_kind.cpp`'s handlers gate on it), written as one unsigned range test:
 * retail's branch-free `subfic`/`orc`/`srwi`/`subf`/`srwi` run is how this compiler spells it. */
extern "C" s32 fn_8012EC3C(_ENEMY_WORK* self)
{
    return (u32)(self->field_0x89F - 2) <= 1;
}

/* Whether the record's latched mode is 1 (`em_alt_mode_set` latches it, 0/1). */
extern "C" s32 em_alt_mode_ck(_ENEMY_WORK* self)
{
    return self->mode_0x8AA == 1;
}

/* Whether the enemy's area/group state admits the record (see the file header). */
extern "C" s32 em_mot_finished_ck(_ENEMY_WORK* self)
{
    f32 rate;

    rate = fn_8012EC74(self);
    return (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4 <= rate;
}

/* The mode-selected base window: 0.3/0.2/0.18/0.15 s by the motion's sub-window count
 * `em_get_rank` returns.  The target re-reads the count in each arm, so the call is written per arm. */
extern "C" f32 fn_8012EC74(_ENEMY_WORK* self)
{
    if ((u8)em_get_rank(self) <= 1) return lbl_80796CB8;
    if ((u8)em_get_rank(self) <= 2) return lbl_80796CBC;
    if ((u8)em_get_rank(self) <= 3) return lbl_80796CC0;
    return lbl_80796CC4;
}

/* The first team-wide window scan: "(100 - kind) % of the motion has run on a live record of `team`".
 * `team`/`kind` are the two bytes the callers at 0x803B5990 mask out of their table row. */
extern "C" u32 em_team_damage_under_ck(u8 team, u8 kind)
{
    _ENEMY_WORK* work;
    u16 max;
    s32 i;
    f32 want;

    want = (f32)(100 - kind) / lbl_80796C5C;
    work = get_move_work_adrs_view3(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++) {
        if (work->active != 0 && work->team == team) {
            if ((f32)(s32)work->field_0x7A0 / (f32)(s32)work->field_0x7A4 <= want) {
                return 1;
            }
        }
        work++;
    }
    return 0;
}

/* The second team-wide window scan: "the taken part `(field_0x7AC - field_0x7A0) / field_0x7A4` is at
 * least `kind` %". */
extern "C" u32 em_team_damage_over_ck(u8 team, u8 kind)
{
    _ENEMY_WORK* work;
    u16 max;
    u32 i;
    f32 want;

    want = (f32)kind / lbl_80796C5C;
    work = get_move_work_adrs_view3(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++) {
        if (work->active != 0 && work->team == team) {
            if ((f32)(s32)(work->field_0x7AC - work->field_0x7A0) / (f32)(s32)work->field_0x7A4 >= want) {
                return 1;
            }
        }
        work++;
    }
    return 0;
}

/* "The record's team id is a live map id and the map's frame gate is the first frame". */
extern "C" s32 fn_8012EF98(_ENEMY_WORK* self)
{
    if (quest_element_state_find(self->team) != 0) {
        if (quest_entry_active_ck() == 1) return 1;
    }
    return 0;
}

/* The area/status gate that guards the per-motion block: the `field_0x1C8` bit-0 request, the area and
 * magma tests, then the team-specific height/map tests. */
extern "C" s32 fn_8012EFDC(_ENEMY_WORK* self)
{
    if (self->field_0x1C8 & 1) {
        if (em_area_ck_view5(self) == 0) return 0;
        if (fn_80130134(self, 1) == 1) return 0;
        if (em_magma_check(self) == 1) return 0;
        if (quest_element_state_find(self->team) == 0x404) return 0;
    }
    switch (self->team) {
    case 0xf:
        if (self->field_0x7B0 > lbl_80796C58) return 0;
        break;
    case 0x19:
        if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 2) return 1;
        return 0;
    case 0x14:
        if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3) return 1;
        return 0;
    }
    return 1;
}

/* The program-mode gate for one of the two window masks (0xa / 0xb); the "die" test is the only path
 * that keeps the record alive. */
extern "C" s32 fn_8012F110(_ENEMY_WORK* self, u16 mode)
{
    if (self->field_0x818 > 0) return 0;
    switch (mode) {
    case 0xa:
        if (fn_8011E640(self, 0x20) != 0) {
            if (self->field_0x1E2 == 0) {
                if ((u32)em_die_ck(self) != 1) break;
            }
        }
        return 0;
    case 0xb:
        if (fn_8011E640(self, 0x40) != 0) {
            if (self->field_0x1E2 == 0 || self->field_0x1E2 == 2) {
                if ((u32)em_die_ck(self) != 1) break;
            }
        }
        return 0;
    }
    return 1;
}

/* The window test with the sub-window padding `em_get_rank` selects (0.1 s or 0.05 s). */
extern "C" u32 em_motion_window_ck(_ENEMY_WORK* self)
{
    if (self->field_0x938 > 0) {
        f32 rate = fn_8012EC74(self);
        f32 limit;

        if ((u8)em_get_rank(self) <= 1) {
            limit = rate + lbl_80796CC8;
        } else {
            limit = rate + lbl_80796CCC;
        }
        return (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4 <= limit;
    }
    return 0;
}

/* The window test behind the two landed cross-unit gates: either of the two "in this motion" tests,
 * then the padded window test. */
extern "C" s32 fn_8012F2A4(_ENEMY_WORK* self)
{
    if (fn_8012D8D0_view5(self) == 1 || fn_8012DB3C_view5(self) == 1) {
        if (em_motion_window_ck(self) == 1) return 1;
    }
    return 0;
}

/* The sleep gate (`u32`): kind 0 is "can sleep this frame", kind 1 the action lookup. */
u32 em_sleep_ck(_ENEMY_WORK* self, u8 kind)
{
    switch (kind) {
    case 0:
        if (em_status_ck_view1(self, 1) == 1 || fn_80132270(self) == 1) return 1;
        return 0;
    case 1:
        if (fn_8012E21C_view5(self->action, self->state_sub) == 1 || fn_80132270(self) == 1) return 1;
        return 0;
    }
    return 0;
}

/* "The record's kind is 4 and its height is above 0.9 * the model scale `fn_8013032C` returns". */
extern "C" s32 fn_8012F39C(_ENEMY_WORK* self)
{
    if (self->field_0x1E2 == 4) {
        if (self->field_0x1AC > lbl_80796CD0 * fn_8013032C(self)) return 1;
    }
    return 0;
}

/* 0x80137604..0x801376B4 - the state accessors. */

/* Stores the record's byte +0x0D. */
extern "C" void em_quest_element_set(_ENEMY_WORK* self, u8 value)
{
    self->field_0x00D = value;
}

/* The second entry point that stores the same byte. */
extern "C" void em_quest_element_set_large(_ENEMY_WORK* self, u8 value)
{
    self->field_0x00D = value;
}

/* Whether the record is on action 11 with its +0x8D3 sub-state byte set. */
extern "C" u32 fn_80137614(_ENEMY_WORK* self)
{
    if (self->action == 11 && self->field_0x8D3 == 1) {
        return 1;
    }
    return 0;
}

/* Sets the field `fn_80137648` reads back. */
extern "C" void fn_8013763C(_ENEMY_WORK* self)
{
    self->field_0x43F = 1;
}

/* Whether the `fn_8013763C` field is set. */
extern "C" u32 fn_80137648(_ENEMY_WORK* self)
{
    return self->field_0x43F == 1;
}

/* Kicks `lb_sub0e_send` for the record when `em_busy_ck` says the action is armed. */
extern "C" void fn_8013765C(_ENEMY_WORK* self, u32 arg1)
{
    if (em_busy_ck(self) == 1) {
        lb_sub0e_send(my_player_no(), arg1, 1, self->field_0x01A);
    }
}

/* Sets the `flags_0x8B3` bit 1. */
extern "C" void fn_801376B4(_ENEMY_WORK* self)
{
    fn_801376DC(self, 1);
}

/* Adds `flag` to the record's `flags_0x8B3` bitmap. */
extern "C" void fn_801376BC(_ENEMY_WORK* self, u8 flag)
{
    if ((self->flags_0x8B3 & flag) == 0) {
        self->flags_0x8B3 |= flag;
    }
}

/* Removes `flag` from the record's `flags_0x8B3` bitmap. */
extern "C" void fn_801376DC(_ENEMY_WORK* self, u8 flag)
{
    if (self->flags_0x8B3 & flag) {
        self->flags_0x8B3 &= ~flag;
    }
}

/* Whether `flag` is set in the record's `flags_0x8B3` bitmap. */
extern "C" u32 fn_80137704(_ENEMY_WORK* self, u8 flag)
{
    return (self->flags_0x8B3 & flag) != 0;
}

/* 0x80137720..0x801378A0 - the motion-mode hook, the move-work picker and the light table. */

/* Latches the record's motion mode and, for mode 2, arms the timer `fn_80126494` returns. */
extern "C" void em_motion_mode_set(_ENEMY_WORK* self, u8 mode)
{
    if ((self->field_0x1C8 & 1) == 0 || (self->field_0x1C8 & 0x80000) != 0) {
        self->field_0x89F = 0;
    }
    self->field_0x89F = mode;
    switch (mode) {
    case 2: {
        s16* timer = fn_80126494(self);

        fn_801376BC(self, 1);
        if (timer != NULL) {
            self->field_0x8A4 = *timer;
        } else {
            self->field_0x8A4 = 3600;
        }
        break;
    }
    case 0:
        fn_801376B4(self);
        break;
    }
}

/* The move-work record `mode` picks: the slot holding the first bit of `mask` when `mode` is 10, the
 * slot `mask` names otherwise. */
extern "C" u8* fn_801377D0(u8 mode, u8 mask)
{
    u8* work = (u8*)get_move_work_adrs_view5(2);

    if (mask != 0xFF) {
        u16 max = get_move_work_max(2);
        u8 slot;

        if (mode == 10) {
            u8 i;

            slot = 0xFF;
            for (i = 0; i < max; i++) {
                if (mask & (1 << i)) {
                    slot = i;
                    break;
                }
            }
        } else {
            slot = mask;
        }
        if (slot < max) {
            work += slot * 2848;
        }
    }
    return work;
}

/* Rebuilds the record's four light records. */
extern "C" void fn_801378A0(void)
{
    setVec3(&lbl_806A4560[0], lbl_80796D34, lbl_80796D38, lbl_80796C58);
    setVec3(&lbl_806A4560[1], lbl_80796C58, lbl_80796D38, lbl_80796D3C);
    setVec3(&lbl_806A4560[2], lbl_80796D3C, lbl_80796D38, lbl_80796C58);
    setVec3(&lbl_806A4560[3], lbl_80796C58, lbl_80796D38, lbl_80796D34);
}
