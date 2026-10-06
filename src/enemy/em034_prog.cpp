/* enemy/em034_prog.cpp - the em034 enemy's program: the seat/effect-action band (seat records, the per-tick seat
 *   update and the seat actions) and its static constructor.
 * RANGE. .text 0x801B4348-0x801B7020 (50 functions); .ctors 0x8056F348-0x8056F34C, .rodata 0x80570260-0x805702A0,
 *   .data 0x805B17A8-0x805B2118, .bss 0x806A7AA0-0x806A7AB8, .sdata 0x80791A90-0x80791AB0,
 *   .sdata2 0x80798C68-0x80798CF8, extab, extabindex.
 * NAMES. `em034_prog` is a GUESS from `em034_prog_tbl` (0x805B17A8), the first object of the unit's `.data`; the
 *   `.bss` record name is a GUESS (a pair of model-space points).
 * RESIDUALS. Every row is written.
 *  - `fn_801B4458`, `fn_801B47A4`, `fn_801B4C54`, `fn_801B5200`, `fn_801B529C`, `fn_801B5338`, `fn_801B563C`,
 *    `fn_801B5774`, `fn_801B5D24`, `fn_801B63E8`, `fn_801B65B8`, `fn_801B670C`: retail keeps `clrlwi`/`rlwinm` +
 *    `cmpwi`, ours emits the record form;
 *  - `fn_801B4398`, `fn_801B4694`, `fn_801B47A4`, `fn_801B5774`, `fn_801B5D24`: retail keeps `clrlwi` + `slwi`,
 *    ours fuses them into `clrlslwi`;
 *  - `fn_801B5200`, `fn_801B529C`, `fn_801B563C`, `fn_801B5774`, `fn_801B5908`, `fn_801B5BEC`, `fn_801B5D24`: ours
 *    loads `state_sub` (+0x5) at a different point of the body;
 *  - `fn_801B5030`, `fn_801B5ED4`, `fn_801B6010`, `fn_801B63E8`, `fn_801B563C`, `fn_801B5774`, `fn_801B5D24`: ours
 *    emits an extra `b` to the shared tail ahead of a case body;
 *  - `fn_801B4D14`, `fn_801B6EF4`, `fn_801B6C38`, `fn_801B5A08`: retail narrows the `u8` argument with `clrlwi`
 *    before the compare, ours compares the register;
 *  - `fn_801B47A4`: retail's frame is 0x50 against our 0x40 and its loops count with `mtctr`/`bdnz`;
 *    `fn_801B6FB0`: frame 0x30 against 0x20, the two `setVec3` calls colour their float registers differently;
 *    `fn_801B4458`, `fn_801B563C`, `fn_801B5774`, `fn_801B65B8`: retail spills f31 with `psq_st`;
 *  - `fn_801B670C`: ours lacks one of retail's `em_after_frame_check` calls (with its `lbl_80798CA0`/`lbl_80798C74`
 *    window);
 *  - `fn_801B45B0`: ours materialises the seat-code compare with `cntlzw`/`srwi` where retail branches;
 *    `fn_801B6B94`: ours loads the +0x1D4 field before retail's `lbl_80798CD0`; `fn_801B6C84`: the stack-vector
 *    setup is ordered differently; `fn_801B610C`, `fn_801B6308`: register allocation only.
 *   flipcheck: `.ctors`/`.rodata`/`.sdata`/`.sdata2` claimed, not emitted; `.data`/`.text` short of the claim.
 * SHAPES. The 10 callees the folded sources declared with different signatures are called through cast macros
 *   (`<name>_cN`, `<name>_viewN`: the same direct call), and the header declarations that disagree are renamed away
 *   around their `#include` (`#define <name> <name>_hidden_<header>`).  No peephole pragma (docs/enemy.md).
 */

#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2: the owner's header) */
#include "enemy/fn_8013072C.h" /* fn_8013072C (rule 2: the owner's header) */
#include "enemy/em_se_tbl_play.h" /* em_se_tbl_play (rule 2: the owner's header) */
#include "enemy/em_se_tbl_play_alt.h" /* em_se_tbl_play_alt (rule 2: the owner's header) */
#include "enemy/em_ground_rec_clear.h" /* fn_80125F54 (rule 2: the owner's header) */
#include "enemy/fn_80126324.h" /* fn_80126324 (rule 2: the owner's header) */
#include "enemy/em_hit_window_set_default.h" /* em_hit_window_set_default (rule 2: the owner's header) */
#include "enemy/fn_8012D0B4.h" /* fn_8012D0B4 (rule 2: the owner's header) */
#include "enemy/fn_8012D1A8.h" /* fn_8012D1A8 (rule 2: the owner's header) */
#include "enemy/fn_8013581C.h" /* fn_8013581C (rule 2: the owner's header) */
#include "enemy/em_wave_amp.h" /* em_wave_amp (rule 2: the owner's header) */
#include "enemy/em_turn_in_window.h" /* em_turn_in_window (rule 2: the owner's header) */
#include "enemy/em_approach_start.h" /* em_approach_start (rule 2: the owner's header) */
#include "enemy/em_approach_step.h" /* em_approach_step (rule 2: the owner's header) */
#include "enemy/em_turn_seq_start.h" /* em_turn_seq_start (rule 2: the owner's header) */
#include "enemy/em_turn_seq_step.h" /* em_turn_seq_step (rule 2: the owner's header) */
#include "enemy/em_move_mode_set.h" /* em_move_mode_set (rule 2: the owner's header) */
#include "enemy/fn_80131DF4.h" /* fn_80131DF4 (rule 2: the owner's header) */
#include "enemy/em_mot_set.h" /* em_mot_set (rule 2: the owner's header) */
#include "enemy/em_mot_set_ck.h" /* em_mot_set_ck (rule 2: the owner's header) */
#include "enemy/em_mot_speed_set.h" /* em_mot_speed_set (rule 2: the owner's header) */
#include "enemy/em_mot_end_ck.h" /* em_mot_end_ck (rule 2: the owner's header) */
#include "enemy/em_action_finish.h" /* em_action_finish (rule 2: the owner's header) */
#include "enemy/fn_8012CEB4.h" /* fn_8012CEB4 (rule 2: the owner's header) */
#include "enemy/fn_80132184.h" /* fn_80132184 (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801B0010.h"
#include "enemy/em005_act.h"
#include "enemy/em007_act.h"
#define fn_8012D0B4 fn_8012D0B4_hidden_fn_8012BDF4_h
#define fn_8012D1A8 fn_8012D1A8_hidden_fn_8012BDF4_h
#include "enemy/fn_8012BDF4.h"
#undef fn_8012D1A8
#undef fn_8012D0B4
#define em_hit_window_set_default em_hit_window_set_default_hidden_fn_801251D0_h
#define em_se_tbl_play em_se_tbl_play_hidden_fn_801251D0_h
#define em_se_tbl_play_alt em_se_tbl_play_alt_hidden_fn_801251D0_h
#define fn_80126324 fn_80126324_hidden_fn_801251D0_h
#include "enemy/fn_801251D0.h" /* EmGroundRec + fn_80125F54 (rule 1/2: their owner) */
#undef fn_80126324
#undef em_se_tbl_play_alt
#undef em_se_tbl_play
#undef em_hit_window_set_default
#include "ai/ainpc.h"   /* `_AINPC_W` (rule 1) */
#include "ai/ainpc_w.h" /* `ainpc_w`, owned by ai/ai_npc.cpp (rule 2) */
#define em_turn_seq_start em_turn_seq_start_hidden_fn_8012EC74_h
#include "enemy/fn_8012EC74.h"
#undef em_turn_seq_start
#include "enemy/enemy_control.h"
#include "enemy/fn_80137604.h"
#define ran_suu ran_suu_hidden_fn_800CDB2C_h
#include "ef/fn_800CDB2C.h"
#undef ran_suu
#define calcVecDistXZ calcVecDistXZ_hidden_fn_8004CAD8_h
#include "fn_8004CAD8.h"
#undef calcVecDistXZ
#define assignVec3 assignVec3_hidden_ef_h
#include "pl.h"
#undef assignVec3
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#define em_turn_seq_start em_turn_seq_start_hidden_enemy_h
#include "unsplit/enemy.h"
#undef em_turn_seq_start
#include "Pl/fn_80262940.h" /* pl_model_state_set (rule 2: its owner's header) */
#include "mh3_pad.h"
#include "quest/quest_item_slot.h" /* quest_item_work_merge (rule 2: its owner) */
#include "enemy/fn_801B4458.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_rot_vec_copy_c1 ((void (*)(void*, void*))eft_rot_vec_copy)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_se_tbl_play_c1 ((void (*)(void*, u32, u32))em_se_tbl_play)
#define em_se_tbl_play_alt_c1 ((void (*)(void*, u32, u32))em_se_tbl_play_alt)
#define em_hit_window_set_default_c1 ((u32 (*)(struct _ENEMY_WORK*, u32, u32))em_hit_window_set_default)
#define fn_8012D0B4_c1 ((u32 (*)(struct _ENEMY_WORK*, void*))fn_8012D0B4)
#define fn_8012D1A8_c1 ((u32 (*)(u8))fn_8012D1A8)
#define em_approach_start_c1 ((void (*)(struct _ENEMY_WORK*, f32, u32))em_approach_start)
#define em_turn_seq_start_c1 ((u32 (*)(struct _ENEMY_WORK*, void*, s32, s32, s32))em_turn_seq_start)
/* Callees the folded sources declared with different parameters: each call keeps its own view through a cast
 * (the same direct call). */
#define ran_suu_view1 ((u16 (*)(s32))ran_suu)
#define fn_8012D1A8_view1 ((s32 (*)(u8))fn_8012D1A8)
#define fn_8012D0B4_view1 ((s32 (*)(struct _ENEMY_WORK*, void*))fn_8012D0B4)
#define fn_80126324_view1 ((void (*)(struct _ENEMY_WORK*, u32, u32, f32))fn_80126324)
#define em_turn_seq_start_view1 ((void (*)(struct _ENEMY_WORK*, void*, s32, s32, s32))em_turn_seq_start)
#define em_se_tbl_play_alt_view1 ((void (*)(struct _ENEMY_WORK*, void*, u32, u32))em_se_tbl_play_alt)
#define em_se_tbl_play_view1 ((void (*)(struct _ENEMY_WORK*, void*, u32, u32))em_se_tbl_play)
#define em_hit_window_set_default_view1 ((void (*)(struct _ENEMY_WORK*, u32, u32))em_hit_window_set_default)
#define calcVecDistXZ_view1 ((f32 (*)(const void*, const void*))calcVecDistXZ)
#define assignVec3_view1 ((void (*)(Vec*, Vec*))assignVec3)

/* The 8-byte `lbl_805B1B08` lookup entry: a key byte, a value byte and the list pointer (more records of this shape,
 * keyed by the kind); `enemy/em004_act.cpp`'s `EmLookupEntry` is the different 0x805B3CD8 table.
 * size: 0x8 */
struct EmCodeListEntry {
    /* +0x0 */ u8 code;
    /* +0x1 */ u8 value_0x01;
    /* +0x2 */ u8 unused_0x02[2];
    /* +0x4 */ void* list_0x04;
};
extern "C" EmCodeListEntry lbl_805B1B08[];

extern "C" {
/* enemy/em_common.cpp (0x8012EC74..0x80137604) - the action/motion arming helpers. */

/* enemy/em_common.cpp (0x801251D0..0x8012BA00) - the program/entry helpers. */
u32 fn_801421E4(u32 id, void* out);
u8 stage_map_kind_get(u8 map);

/* enemy/em_common.cpp (the 0x8013xxxx motion setters). */

/* the base vector/effect helpers (owned elsewhere; declared, never defined - playbook 29). */
void assignVec3(void* out, void* in);
void addVec3(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void fn_8004FFC8(void* a, void* b, void* c, f32 d);
void fn_800AD9C0(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 scale);
f32 fn_80050EF4(void* a, void* b);
f32 calcVecDistXZ(void* a, void* b);
void fn_800FA378(void* out);
u32 move_work_state_ck(void);
void mhchar_mat_tev_set(void* self, s32 a, s32 b, u8 c, s32 d, s32 e, u8 f);
s32 fn_8028F558(void* a, void* b);
s32 fn_802907BC(void* a, void* b);
void eft_spawn_pos_in_area(void* pos, u8 area, u8 kind, s32 mode, f32 scale);
void eft009_spawn_at_joint(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint, f32 scale);
void fn_801049D0(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint, nw4r::math::VEC3* pos,
                 f32 scale);
s32 em_roster_record_slot_id_get(s32 handle);
void em_roster_record_release(s32 handle);
}

/* the callees whose map rows are manglings (rule 9). */
s32 ran_suu(s32 a);
void vec_to_mh_vec3(nw4r::math::VEC3* dst, Vec* src);

/* the mangled callees, at their real signatures (rule 9). */
s32 em_act_ck(struct _ENEMY_WORK* self, u8 a, u8 b);
s32 em_die_ck(struct _ENEMY_WORK* self);
u16 em_get_mot_no(struct _ENEMY_WORK* self);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
void* get_move_work_adrs(u8 index);
u16 get_move_work_max(u8 index);
f32 get_em_chg_scale(struct _ENEMY_WORK* self);
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);

/* The `.sdata2` float pool and the two data tables the band reads. */
extern f32 lbl_80798C68; /* 1000000.0f */
extern f32 lbl_80798C6C; /* 2.0f */
extern f32 lbl_80798C70; /* 1.0f */
extern f32 lbl_80798C74; /* 0.0f */
extern f32 lbl_80798C78; /* 1000.0f */
extern f32 lbl_80798C7C; /* 400.0f */
extern f32 lbl_80798C80; /* 0.85f */
extern f32 lbl_80798C84; /* 1.3f */
extern f32 lbl_80798C88; /* 1.1f */
extern f32 lbl_80798C8C; /* -200.0f */
extern f32 lbl_80798C90; /* -500.0f */
extern f32 lbl_80798C94; /* 30.0f */
extern f32 lbl_80798C98; /* 46.0f */
extern f32 lbl_80798C9C; /* 0.8f */
extern f32 lbl_80798CA0; /* 24.0f */
extern f32 lbl_80798CA4; /* 1.8f */
extern f32 lbl_80798CA8; /* 0.5f */
extern f32 lbl_80798CAC; /* 10.0f */
extern f32 lbl_80798CB0; /* 16.0f */
extern f32 lbl_80798CB4; /* 14.0f */
extern f32 lbl_80798CB8; /* 0.6f */
extern f32 lbl_80798CBC; /* 34.0f */
extern f32 lbl_80798CC0; /* -15.0f */
extern f32 lbl_80798CC4; /* -50.0f */
extern f32 lbl_80798CC8; /* 40.0f */
extern f32 lbl_80798CCC; /* 1.2f */
extern f32 lbl_80798CD0; /* 128.0f */
extern f32 lbl_80798CD4; /* 255.0f */
extern f32 lbl_80798CD8; /* 200.0f */
extern f32 lbl_80798CDC; /* 500.0f */
extern f32 lbl_80798CE0; /* 300.0f */
extern f32 lbl_80798CE4; /* 140.0f */
extern f32 lbl_80798CE8; /* 17.0f */
extern f32 lbl_80798CEC; /* -20.0f */
extern f32 lbl_80798CF0; /* -2.38f */
extern f32 lbl_80798CF4; /* 0.11f */

extern u8 lbl_80570260[];
extern VEC3 vec_pair_801B4458_0[2];

/* the 0x805B1Bxx enemy-control program tables `em_se_tbl_play`/`em_se_tbl_play_alt` walk. */
extern u8 lbl_805B1BB0[];
extern u8 lbl_805B1BD8[];
extern u8 lbl_805B1C30[];
extern u8 lbl_805B1C78[];
extern u8 lbl_805B1CA0[];
extern u8 lbl_805B1CC8[];
extern u8 lbl_805B1CF0[];
extern u8 lbl_805B1D30[];
extern u8 lbl_805B1DA0[];
extern u8 lbl_805B1DC8[];
extern u8 lbl_805B1E08[];
extern u8 lbl_805B1E50[];
extern u8 lbl_805B1E90[];
extern u8 lbl_805B1F18[];
extern u8 lbl_805B1FF0[];
extern u8 lbl_805B20E8[];

/* 0x801B4348 (0x50).  The em030 ground-position hook: the ground record `em_ground_rec_clear` builds for the
 * work's +0x1A id is copied onto the work's +0x1B0 when the enemy-control lookup finds it. */
extern "C" void fn_801B4348(_ENEMY_WORK* work) {
    EmGroundRec rec;

    em_ground_rec_clear(&rec);
    if (fn_801421E4(work->field_0x01A, &rec) == 1) {
        copyVec3(&work->aim, &rec.pos_0x08);
    }
}

/* 0x801B4398 (0xC0).  The em030 program-table lookup: the `lbl_805B1B08` entry whose +0x1E0 key
 * matches the work record's, then the 8-byte entry list under it, keyed by the kind. */
extern "C" u32 fn_801B4398(_ENEMY_WORK* work, u32 kind, u32* out) {
    u8 i;

    for (i = 0; lbl_805B1B08[i].code != 0xFF; i++) {
        EmCodeListEntry* entry;

        if (fn_80125FF0(work->field_0x1E0, 0) != 1) {
            continue;
        }
        entry = (EmCodeListEntry*)lbl_805B1B08[i].list_0x04;
        if (entry == NULL) {
            break;
        }
        for (; entry->code != 0xFF; entry++) {
            if (entry->code == (u8)kind) {
                *out = (u32)entry->list_0x04;
                return entry->value_0x01;
            }
        }
        break;
    }
    return 0;
}

/* The 0x20-byte ground record `em_ground_rec_clear` prepares and `fn_801421E4` fills is `EmGroundRec`
 * (`enemy/ENEMY_WORK.h`). */

/* The range's functions, in address order. */

/* 0x801B4458 (0x158).  Picks the seat record of the `kind` set `fn_801B4398` hands back whose point
 * (or two-point segment midpoint) is nearest the work record's `pos`; 0xFF when the set is empty. */
extern "C" u8 fn_801B4458(_ENEMY_WORK* self, u8 kind) {
    nw4r::math::VEC3 mid;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 delta;
    nw4r::math::VEC3 point;
    EmSeatRec* list;
    u8 best;
    u8 count;
    u8 i;
    f32 bestDist;
    f32 dist;

    list = NULL;
    best = 0xFF;
    bestDist = lbl_80798C68;
    VEC3_ctor(&mid);
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    count = (u8)fn_801B4398(self, kind, (u32*)&list);
    for (i = 0; i < count; i++) {
        if (list->code == 1) {
            vec_to_mh_vec3(&a, (Vec*)list->vec_0x04);
            vec_to_mh_vec3(&b, (Vec*)list->vec_0x10);
            addVec3(&delta, &a, &b);
            fn_800AD9C0(&point, &delta, lbl_80798C6C);
            copyVec3(&mid, &point);
        } else {
            vec_to_mh_vec3(&a, (Vec*)list->vec_0x04);
            copyVec3(&mid, &a);
        }
        dist = fn_80050EF4(&self->pos, &mid);
        if (i == 0 || bestDist > dist) {
            bestDist = dist;
            best = i;
        }
        list++;
    }
    return best;
}

/* 0x801B45B0 (0xE4): whether the seat record is within `radius` of `point`; a two-point seat (code 1) tests the
 * box between its points, a single-point seat the straight distance. */
extern "C" s32 fn_801B45B0(void* point, void* seat, f32 radius) {
    EmSeatRec* rec;
    u8 box[0x34];
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;

    rec = (EmSeatRec*)seat;
    fn_800FA378(box);
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    if (rec->code == 1) {
        vec_to_mh_vec3(&a, (Vec*)rec->vec_0x04);
        vec_to_mh_vec3(&b, (Vec*)rec->vec_0x10);
        fn_8004FFC8(&a, &b, box, radius);
        fn_8028F558(box, box);
        if (fn_802907BC(point, box) == 1) {
            return 1;
        }
        return 0;
    }
    vec_to_mh_vec3(&a, (Vec*)rec->vec_0x04);
    if (fn_80050EF4(point, &a) < radius) {
        return 1;
    }
    return 0;
}

/* 0x801B4694 (0x110): moves the aim to seat `index` - a segment (`fn_8013581C`) for a two-point seat, the point
 * itself (`em_wave_amp`) otherwise, each with the seat's own float and a random u16 phase. */
extern "C" void fn_801B4694(_ENEMY_WORK* self, u8 index) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    EmSeatRec* list;
    u8 count;
    u8 i;
    u16 phase;

    list = NULL;
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    count = (u8)fn_801B4398(self, self->area_no, (u32*)&list);
    if ((u8)index < count) {
        phase = ran_suu(0);
        i = (u8)index;
        if (list[i].code == 1) {
            vec_to_mh_vec3(&a, (Vec*)list[i].vec_0x04);
            vec_to_mh_vec3(&b, (Vec*)list[i].vec_0x10);
            fn_8013581C(&self->aim, &a, &b, 0, phase, list[i].value_0x1C);
        } else {
            vec_to_mh_vec3(&a, (Vec*)list[i].vec_0x04);
            em_wave_amp(&self->aim, &a, list[i].value_0x1C, 0, phase);
        }
    }
}

/* 0x801B47A4 (0x4B0): the per-tick seat update; marks which of the up-to-10 seats the kind-3 work and kind-2 area
 * records occupy, re-picks the current seat (`fn_801B4458`) and arms the seat action (`fn_8013072C`). */
extern "C" void fn_801B47A4(_ENEMY_WORK* self) {
    u8 flags[0x0A];
    EmSeatRec* list;
    _ENEMY_WORK* work;
    EmAreaWork* area;
    u8 count;
    u8 i;
    u8 j;
    u8 k;
    u8 n;
    u8 seated;
    u8 any;
    u16 max;

    any = 0;
    seated = 0;
    list = NULL;
    if (self->field_0x43B == 1) {
        if (self->field_0x440 > 0x1C2) {
            fn_8013072C(self, 0, 0);
            return;
        }
    } else if ((u8)(self->field_0x43B + 0xFE) <= 1) {
        count = fn_801B4398(self, self->area_no, (u32*)&list);
        if (count > 0x0A) {
            count = 0x0A;
        }
        memset(flags, 0, 0x0A);
        max = get_move_work_max(3);
        work = (_ENEMY_WORK*)get_move_work_adrs(3);
        for (i = 0; i < max; i++) {
            if (work->active != 0 && (work->field_0x1C8 & 1) != 0 && self->area_no == work->area_no) {
                any = 1;
                if ((self->field_0x43B == 3 || self->field_0x440 > 0x1C2) && count != 0) {
                    for (j = 0; j < count; j++) {
                        if (j >= 0x0A) {
                            break;
                        }
                        if (fn_801B45B0(&work->pos, &list[j], lbl_80798C78) == 1) {
                            flags[j] = 1;
                            if (j == self->slot_0x328) {
                                seated = 1;
                            }
                        }
                    }
                }
            }
            work++;
        }
        if ((self->field_0x43B == 3 || self->field_0x440 > 0x1C2) && count != 0) {
            max = get_move_work_max(2);
            area = (EmAreaWork*)get_move_work_adrs(2);
            for (i = 0; i < max; i++) {
                if (area->active != 0 && fn_8012D1A8_c1(area->field_0x008) == 0 &&
                    fn_8012D0B4_c1(self, area) != 0) {
                    for (j = 0; j < count; j++) {
                        if (j >= 0x0A) {
                            break;
                        }
                        if (fn_801B45B0(&area->vec_0x3C, &list[j], lbl_80798C78) == 1) {
                            flags[j] = 1;
                            if (j == self->slot_0x328) {
                                seated = 1;
                            }
                        }
                    }
                }
                area++;
            }
        }
        if (seated || (self->flag_0x329 == 1 && self->field_0x833 != 0)) {
            fn_8013072C(self, 2, 0);
            self->flag_0x329 = 0;
            fn_8012CEB4(self, (s16)(ran_suu(0) & 0x1F), 0);
        }
        if (any == 0) {
            fn_8013072C(self, 0, 0);
            fn_801B4348(self);
        } else if (self->field_0x43B == 2 && self->field_0x440 > 0x1C2) {
            k = 0;
            if (self->slot_0x328 != 0xFF) {
                k = self->slot_0x328;
                n = count;
                if (count != 0) {
                    do {
                        k = (u8)(k + 1);
                        if (k >= count) {
                            k = 0;
                        }
                        if (flags[k] == 0) {
                            break;
                        }
                        n--;
                    } while (n != 0);
                }
                self->slot_0x328 = k;
            }
            fn_8013072C(self, 3, k);
            fn_801B4694(self, k);
        }
        if (self->field_0x43B == 3 && self->flag_0x329 == 0 &&
            calcVecDistXZ(&self->pos, &self->aim) < lbl_80798C7C) {
            self->flag_0x329 = 1;
        }
    } else {
        if (self->field_0x43B == 0 && (self->field_0x833 == 1 || (self->field_0x00A & 2) != 0)) {
            fn_8013072C(self, 4, 0);
        }
        max = get_move_work_max(3);
        work = (_ENEMY_WORK*)get_move_work_adrs(3);
        for (i = 0; i < max; i++) {
            if (work->active != 0 && (work->field_0x1C8 & 1) != 0 &&
                self->area_no == work->area_no) {
                u8 seat = fn_801B4458(self, self->area_no);
                self->slot_0x328 = seat;
                if (seat == 0xFF) {
                    fn_8013072C(self, 3, 0);
                    return;
                }
                fn_8013072C(self, 3, seat);
                fn_801B4694(self, self->slot_0x328);
                return;
            }
            work++;
        }
    }
}

/* 0x801B4C54 (0xC0).  The enemy-control seat class: 0 when the control lookup fails, else 2 or 1
 * from the +0x0B flag and a random percent (2 on a 2-bit flag below 60, else on a low roll). */
extern "C" s32 fn_801B4C54(u16 id) {
    EmGroundRec rec;
    s32 pct;

    em_ground_rec_clear(&rec);
    if (move_work_state_ck() == 0) {
        return 0;
    }
    if (fn_801421E4((u16)id, &rec) == 0) {
        return 0;
    }
    pct = (u16)ran_suu(0) % 100;
    if ((rec.field_0x03 & 2) != 0) {
        if (pct < 0x3C) {
            return 2;
        }
    } else if (pct < 0x0A) {
        return 2;
    }
    return 1;
}

/* 0x801B4D14 (0x124).  The per-tick seat preference: refresh the +0x0A mode from the control class,
 * scale the +0x1CC weight by the mode, then reset the seat pair. */
extern "C" void fn_801B4D14(_ENEMY_WORK* self, u8 kind) {
    s8 mode;

    if (kind == 1 || kind == 4) {
        mode = fn_801B4C54(self->field_0x01A);
        if (mode == 1) {
            self->field_0x00A = (u8)(self->field_0x00A & 0xFD);
        } else if (mode == 2) {
            self->field_0x00A = (u8)(self->field_0x00A | 2);
        }
    } else if (self->field_0x00F == 0) {
        mode = fn_801B4C54(self->field_0x01A);
        if (mode == 1) {
            self->field_0x00A = (u8)(self->field_0x00A & 0xFD);
        } else if (mode == 2) {
            self->field_0x00A = (u8)(self->field_0x00A | 2);
        }
    }
    switch (self->field_0x00A) {
    case 1:
        self->field_0x1CC = self->field_0x1CC * lbl_80798C80;
        break;
    case 2:
        self->field_0x1CC = self->field_0x1CC * lbl_80798C84;
        break;
    case 3:
        self->field_0x1CC = self->field_0x1CC * lbl_80798C88;
        break;
    }
    self->slot_0x328 = 0xFF;
    self->flag_0x329 = 0;
}

/* 0x801B4E38 (0x4).  The empty slot the neighbour's function table keeps. */
extern "C" void fn_801B4E38(void) {}

/* 0x801B4E3C (0x6C).  Releases the latched effect handle +0x888 when neither the 0xA/0x7D nor the
 * 0xB/0x25 action is live. */
extern "C" void fn_801B4E3C(_ENEMY_WORK* self) {
    if (em_act_ck(self, 0x0A, 0x7D) == 0 && em_act_ck(self, 0x0B, 0x25) == 0 &&
        self->field_0x888 != -1) {
        em_roster_record_release(self->field_0x888);
        self->field_0x888 = -1;
    }
}

/* 0x801B4EA8 (0x60).  The seat update's per-tick entry: when the work is not dying, clear the
 * "already seated" byte and run the seat update. */
extern "C" void fn_801B4EA8(_ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        if (self->field_0x834 == 1) {
            fn_8013072C(self, 1, 0);
            self->field_0x834 = 0;
        }
        fn_801B47A4(self);
    }
}

/* 0x801B4F08 (0x7C).  Seat sub-state 0: set motion 1/4, then wait. */
extern "C" void fn_801B4F08(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B4F84 (0x7C).  Seat sub-state 0: set motion 0x11/4, then wait. */
extern "C" void fn_801B4F84(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x11, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B5000 (0x30).  Dispatches the 0x801B4F08/0x801B4F84 pair on the +0x1E6 sub-state. */
extern "C" void fn_801B5000(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801B4F08(self);
        return;
    case 1:
        fn_801B4F84(self);
        return;
    case 2:
        fn_801B4F08(self);
        return;
    }
}

/* 0x801B5030 (0xD8).  Seat sub-state 2: arm motion 2/2 and latch the +0x1EC bit, then run the
 * latch down before finishing. */
extern "C" void fn_801B5030(_ENEMY_WORK* self) {
    u8 state = self->state;
    u8 latch;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 2, 0);
        self->state_0x006 = (u8)(self->bits_0x1EC & 1);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            latch = self->state_0x006;
            if (latch == 0) {
                self->state = (u8)(self->state + 1);
                em_mot_set(self, 7, 4, 0);
                return;
            }
            self->state_0x006 = (u8)(latch - 1);
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

/* 0x801B5108 (0x7C).  Seat sub-state 3: arm motion 3/2, then wait. */
extern "C" void fn_801B5108(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B5184 (0x7C).  Seat sub-state 4: arm motion 4/4, then wait. */
extern "C" void fn_801B5184(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B5200 (0x9C).  Seat sub-state 5: motion 5 (0x23 with the +0x0A bit), then wait. */
extern "C" void fn_801B5200(_ENEMY_WORK* self) {
    u8 state = self->state;
    u32 motion = 5;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        if ((self->field_0x00A & 1) != 0) {
            motion = 0x23;
        }
        em_mot_set(self, motion, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B529C (0x9C).  Seat sub-state 6: motion 6 (0x24 with the +0x0A bit), then wait. */
extern "C" void fn_801B529C(_ENEMY_WORK* self) {
    u8 state = self->state;
    u32 motion = 6;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        if ((self->field_0x00A & 1) != 0) {
            motion = 0x24;
        }
        em_mot_set(self, motion, 0, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B5338 (0xB0).  Seat sub-state 7: motion 0x26 with the +0x0A bit, else 0x15 plus the +0x1CC
 * weight reset. */
extern "C" void fn_801B5338(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        if ((self->field_0x00A & 1) != 0) {
            em_mot_set(self, 0x26, 4, 0);
            return;
        }
        em_mot_set(self, 0x15, 4, 0);
        em_mot_speed_set(self, lbl_80798C84);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B53E8 (0x7C).  Seat sub-state 8: motion 1/4, then wait. */
extern "C" void fn_801B53E8(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B5464 (0x7C).  Seat sub-state 9: motion 0x11/4, then wait. */
extern "C" void fn_801B5464(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B54E0 (0x7C).  Seat sub-state 10: motion 0x26/4, then wait. */
extern "C" void fn_801B54E0(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B555C (0x4C).  Dispatches the 0x801B5030..0x801B54E0 set on the +0x1E6 sub-state. */
extern "C" void fn_801B555C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801B5030(self);
        return;
    case 1:
        fn_801B5108(self);
        return;
    case 2:
        fn_801B5184(self);
        return;
    case 3:
        fn_801B5200(self);
        return;
    case 4:
        fn_801B529C(self);
        return;
    case 5:
        fn_801B5338(self);
        return;
    case 6:
        fn_801B53E8(self);
        return;
    case 7:
        fn_801B5464(self);
        return;
    case 8:
        fn_801B54E0(self);
        return;
    default:
        return;
    }
}

/* 0x801B55A8 (0x94).  Seat sub-state 11: motion 8/4 plus the +0x34004 motion timer, then wait for
 * its 0xA0-frame gate. */
extern "C" void fn_801B55A8(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        em_approach_start_c1(self, lbl_80798C8C, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B563C (0x138).  Seat sub-state 12 (two variants): motion 9/4 with the kind-selected timer,
 * then either finish or continue with 0x25/4. */
extern "C" void fn_801B563C(_ENEMY_WORK* self, u8 kind) {
    u8 state = self->state;
    f32 timer = lbl_80798C8C;
    u32 motion = 0x0D;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 4, 0);
        if (kind == 1) {
            timer = lbl_80798C90;
        }
        em_approach_start_c1(self, timer, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            if (kind == 0) {
                em_action_finish(self);
                return;
            }
            self->state = (u8)(self->state + 1);
            if ((self->field_0x00A & 1) != 0) {
                motion = 0x25;
            }
            em_mot_set(self, motion, 4, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x801B5774 (0x194).  Seat sub-state 13: motion 0xA/4 with the kind-selected timer and the +0x10
 * flag/timer, then either finish or continue with 0x25/4. */
extern "C" void fn_801B5774(_ENEMY_WORK* self, u8 kind, u8 flag) {
    u8 state = self->state;
    f32 timer = lbl_80798C8C;
    u32 motion = 0x0D;
    u32 done;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x0A, 4, 0);
        if (kind == 1) {
            timer = lbl_80798C90;
        }
        em_approach_start_c1(self, timer, 0x10);
        if (flag == 1) {
            self->state_0x006 = (u8)(((self->bits_0x1EC & 1) * 2) + 2);
        }
        return;
    case 1:
        done = em_approach_step(self, 0, 0x100);
        if (flag == 1 && em_mot_end_ck(self) == 1) {
            if (self->state_0x006 != 0) {
                self->state_0x006 = (u8)(self->state_0x006 - 1);
            } else {
                done = 1;
            }
        }
        if (done != 0) {
            if (kind == 0) {
                em_action_finish(self);
                return;
            }
            self->state = (u8)(self->state + 1);
            if ((self->field_0x00A & 1) != 0) {
                motion = 0x25;
            }
            em_mot_set(self, motion, 4, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x801B5908 (0x100).  Seat sub-state 14: motion 0xB/4 (0xC with kind) repeated `count` times,
 * then finish. */
extern "C" void fn_801B5908(_ENEMY_WORK* self, u8 kind, u8 count) {
    u8 state = self->state;
    u32 motion = 0x0B;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        self->state_0x006 = count;
        if (kind == 1) {
            motion = 0x0C;
        }
        em_mot_set(self, motion, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (self->state_0x006 <= 1) {
                em_action_finish(self);
                return;
            }
            self->state = 1;
            self->state_0x006 = (u8)(self->state_0x006 - 1);
            if (em_get_mot_no(self) == 0x0B) {
                motion = 0x0C;
            }
            em_mot_set(self, motion, 4, 0);
        }
        return;
    }
}

/* 0x801B5A08 (0xDC).  Seat sub-state 15: motion 0xE/4 (0xF with flag), then the +0x33E3C angle set. */
extern "C" void fn_801B5A08(_ENEMY_WORK* self, u8 flag) {
    u8 state = self->state;
    u32 motion = 0x0E;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        if (flag == 1) {
            motion = 0x0F;
        }
        em_mot_set(self, motion, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        if (flag == 0) {
            em_turn_in_window(self, lbl_80798C74, lbl_80798C94, 0x7800);
            return;
        }
        em_turn_in_window(self, lbl_80798C74, lbl_80798C94, -0x7800);
        return;
    }
}

/* 0x801B5AE4 (0x7C).  Seat sub-state 16: motion 0x10/4, then wait. */
extern "C" void fn_801B5AE4(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x10, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B5B60 (0x8C).  Seat sub-state 17: the +0x80570260 motion set, then wait on its gate. */
extern "C" void fn_801B5B60(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_turn_seq_start_c1(self, lbl_80570260, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570260) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B5BEC (0x138).  Seat sub-state 18: motion 0xE/4 for a low +0x1EC mod-5, else 0xF/4, then the
 * +0x33E3C angle set selected by that mod. */
extern "C" void fn_801B5BEC(_ENEMY_WORK* self) {
    u8 state = self->state;
    u32 motion = 0x0F;
    s32 angle = 0;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        self->state_0x006 = (u8)(self->bits_0x1EC % 5);
        if (self->state_0x006 < 3) {
            motion = 0x0E;
        }
        em_mot_set(self, motion, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        switch (self->state_0x006) {
        case 0:
            angle = 0x8000;
            break;
        case 1:
            angle = 0x6000;
            break;
        case 2:
            angle = 0x4000;
            break;
        case 3:
            angle = -0x4000;
            break;
        case 4:
            angle = -0x6000;
            break;
        }
        em_turn_in_window(self, lbl_80798C74, lbl_80798C94, angle);
        return;
    }
}

/* 0x801B5D24 (0xFC).  Seat sub-state 19: motion 0xA/4 with a +0x1EC-derived countdown, then either
 * finish or continue with 0x25/4. */
extern "C" void fn_801B5D24(_ENEMY_WORK* self, u8 flag) {
    u8 state = self->state;
    u32 motion = 0x0D;
    s32 timer;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x0A, 4, 0);
        timer = self->bits_0x1EC & 3;
        self->timer_0x020 = (timer * 0x10) - timer + 0x28;
        return;
    case 1:
        timer = self->timer_0x020 - 1;
        self->timer_0x020 = timer;
        if (timer <= 0) {
            if (flag == 0) {
                em_action_finish(self);
                return;
            }
            self->state = (u8)(state + 1);
            if ((self->field_0x00A & 1) != 0) {
                motion = 0x25;
            }
            em_mot_set(self, motion, 4, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B5E20 (0xB4).  Dispatches the 0x801B55A8..0x801B5D24 set on the +0x1E6 sub-state. */
extern "C" void fn_801B5E20(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801B55A8(self);
        return;
    case 1:
        fn_801B563C(self, 0);
        return;
    case 2:
        fn_801B5774(self, 0, 0);
        return;
    case 3:
        fn_801B5908(self, 0, 4);
        return;
    case 4:
        fn_801B5A08(self, 0);
        return;
    case 5:
        fn_801B5AE4(self);
        return;
    case 6:
        fn_801B563C(self, 1);
        return;
    case 7:
        fn_801B5774(self, 1, 0);
        return;
    case 8:
        fn_801B5908(self, 1, 4);
        return;
    case 9:
        fn_801B5A08(self, 1);
        return;
    case 10:
        fn_801B5B60(self);
        return;
    case 11:
        fn_801B5774(self, 1, 1);
        return;
    case 12:
        fn_801B5908(self, 0, 1);
        return;
    case 13:
        fn_801B5908(self, 1, 1);
        return;
    case 14:
        fn_801B5BEC(self);
        return;
    case 15:
        fn_801B5D24(self, 1);
        return;
    default:
        return;
    }
}

/* 0x801B5ED4 (0x13C).  Seat sub-state 20: motion 0x12/4 plus the +0x29668 gate, then either the
 * frame window or the 0x13/0x14 pair. */
extern "C" void fn_801B5ED4(_ENEMY_WORK* self, u8 flag) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        em_hit_window_set_default_c1(self, 0, 1);
        return;
    case 1:
        if (flag == 0) {
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
                return;
            }
            return;
        }
        if (em_frame_check(self, 1, lbl_80798C98, lbl_80798C74) == 1) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x13, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x14, 0, 8);
            em_hit_window_set_default_c1(self, 0, 2);
            return;
        }
        return;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B6010 (0xC4).  The 0x801B5ED4 variant without the frame window: 0x13 then 0x14. */
extern "C" void fn_801B6010(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 0, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x14, 0, 8);
            em_hit_window_set_default_c1(self, 0, 2);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B60D4 (0x38).  Dispatches the 0x801B5ED4/0x801B6010 trio on the +0x1E6 sub-state. */
extern "C" void fn_801B60D4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801B5ED4(self, 0);
        return;
    case 1:
        fn_801B6010(self);
        return;
    case 2:
        fn_801B5ED4(self, 1);
        return;
    }
}

/* 0x801B610C (0x1FC).  The enemy-control program-set selector: map the +0x1E6 sub-state to the
 * `em_se_tbl_play` (table, flag, id) triple; the default finishes the action. */
extern "C" void fn_801B610C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x01:
        em_se_tbl_play_c1(lbl_805B1BB0, 0, 1);
        return;
    case 0x02:
        em_se_tbl_play_c1(lbl_805B1BD8, 0, 2);
        return;
    case 0x17:
        em_se_tbl_play_c1(lbl_805B1C30, 1, 0x17);
        return;
    case 0x05:
        em_se_tbl_play_c1(lbl_805B1C30, 1, 5);
        return;
    case 0x23:
        em_se_tbl_play_c1(lbl_805B1C30, 1, 0x23);
        return;
    case 0x7A:
        em_se_tbl_play_c1(lbl_805B1C78, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play_c1(lbl_805B1CA0, 0, 0x7B);
        return;
    case 0x7C:
        em_se_tbl_play_c1(lbl_805B1CC8, 0, 0x7C);
        return;
    case 0x8D:
        em_se_tbl_play_c1(lbl_805B1CF0, 0, 0x8D);
        return;
    case 0x7D:
        em_se_tbl_play_c1(lbl_805B1D30, 0, 0x7D);
        return;
    case 0x8E:
        em_se_tbl_play_c1(lbl_805B1DA0, 0, 0x8E);
        return;
    case 0x84:
        em_se_tbl_play_c1(lbl_805B1C30, 1, 0x84);
        return;
    case 0x9F:
        em_se_tbl_play_c1(lbl_805B1DC8, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play_c1(lbl_805B1DC8, 0, 0xA0);
        return;
    case 0xA8:
        em_se_tbl_play_c1(lbl_805B1E08, 0, 0xA8);
        return;
    case 0xA9:
        em_se_tbl_play_c1(lbl_805B1C30, 1, 0xA9);
        return;
    default:
        em_action_finish(self);
        return;
    }
}

/* 0x801B6308 (0xE0).  The `em_se_tbl_play_alt` sibling of 0x801B610C (the table is the second half of the
 * set; the default arms id 0 on the first table). */
extern "C" void fn_801B6308(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x00:
        em_se_tbl_play_alt_c1(lbl_805B1E50, 1, 0);
        return;
    case 0x05:
        em_se_tbl_play_alt_c1(lbl_805B1E50, 1, 5);
        return;
    case 0x1D:
        em_se_tbl_play_alt_c1(lbl_805B1E90, 0, 0x1D);
        return;
    case 0x1E:
        em_se_tbl_play_alt_c1(lbl_805B1E50, 1, 0x1E);
        return;
    case 0x38:
        em_se_tbl_play_alt_c1(lbl_805B1F18, 0, 0x38);
        return;
    case 0x25:
        em_se_tbl_play_alt_c1(lbl_805B1FF0, 0, 0x25);
        return;
    case 0x2A:
        em_se_tbl_play_alt_c1(lbl_805B20E8, 1, 0x2A);
        return;
    default:
        em_se_tbl_play_alt_c1(lbl_805B1E50, 0, 0);
        return;
    }
}

/* 0x801B63E8 (0x100).  Seat sub-state 21: motion 0xA/4 with the +0x10/-500 timer, then the ground
 * hook or 0x25/4 by the +0x43B kind. */
extern "C" void fn_801B63E8(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x0A, 4, 0);
        em_approach_start_c1(self, lbl_80798C90, 0x10);
        if (self->field_0x43B != 3 && self->field_0x43B != 2) {
            fn_801B4348(self);
            return;
        }
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            u32 motion = 0x0D;
            self->state = (u8)(self->state + 1);
            if ((self->field_0x00A & 1) != 0) {
                motion = 0x25;
            }
            em_mot_set(self, motion, 4, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B64E8 (0x14).  Seat sub-state 22: 0x801B63E8 only on its first sub-state. */
extern "C" void fn_801B64E8(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_801B63E8(self);
    }
}

/* 0x801B64FC (0xBC).  The seat-action dispatcher: switch the action id `action` over the six seat
 * sub-state groups, then run the shared 0xA/0x7D tail. */
extern "C" void fn_801B64FC(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_801B5000(self);
        break;
    case 1:
        fn_801B555C(self);
        break;
    case 2:
        fn_801B5E20(self);
        break;
    case 7:
        fn_801B60D4(self);
        break;
    case 10:
        fn_801B610C(self);
        break;
    case 11:
        fn_801B6308(self);
        break;
    case 12:
        fn_801B64E8(self);
        break;
    }
    if (em_act_ck(self, 0x0A, 0x7D) != 0) {
        fn_80131DF4(self);
    }
}

/* 0x801B65B8 (0x154).  Spawns the seat's effect at the motion's joint (kind 1) or directly (kind 0),
 * with the +0x228 gate selecting the "blocked" id/scale. */
extern "C" void fn_801B65B8(_ENEMY_WORK* self, u8 kind, u8 id, s32 joint, s32 arg4, f32 scale) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (kind) {
    case 0:
        switch (id) {
        case 9:
            if ((self->field_0x228 & 6) != 0) {
                id = 0x33;
                scale += lbl_80798C70;
            }
            break;
        case 37:
            if ((self->field_0x228 & 6) != 0) {
                id = 0x34;
                scale += lbl_80798C9C;
            }
            break;
        case 39:
            if ((self->field_0x228 & 6) != 0) {
                id = 0x16;
                scale += lbl_80798C88;
            } else {
                id = 9;
            }
            break;
        }
        eft009_spawn_at_joint(self, joint, id, arg4, scale);
        return;
    case 1:
        get_joint_wpos_em(self, joint, &pos);
        pos.y = self->field_0x20C;
        eft_spawn_pos_in_area(&pos, self->area_no, id, arg4, scale * get_em_chg_scale(self));
        return;
    default:
        return;
    }
}

/* 0x801B670C (0x488).  The per-motion effect selector: switch `em_get_mot_no` over the 15 motion
 * ids and spawn the motion's effect when its `em_after_frame_check` window opens. */
extern "C" void fn_801B670C(_ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (em_get_mot_no(self)) {
    case 0x9:
        if ((self->field_0x228 & 6) != 0 &&
            em_after_frame_check(self, 0, lbl_80798CA0, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 0x17, 0x1A, 0, lbl_80798CA4);
        }
        if (em_after_frame_check(self, 0, lbl_80798C6C, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 0x27, 0x1A, 0, lbl_80798CA8);
        }
        return;
    case 0xA:
        if ((self->field_0x228 & 6) != 0 &&
            em_after_frame_check(self, 0, lbl_80798CAC, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 0x17, 0x1A, 0, lbl_80798CA4);
        }
        if (em_after_frame_check(self, 0, lbl_80798CB0, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 0x27, 0x1A, 0, lbl_80798CA8);
        }
        return;
    case 0xB:
        if (em_after_frame_check(self, 0, lbl_80798CB4, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 9, 0x19, 0, lbl_80798CB8);
        }
        return;
    case 0xC:
        if (em_after_frame_check(self, 0, lbl_80798CB4, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 9, 0x15, 0, lbl_80798CB8);
        }
        return;
    case 0xD:
        if (em_after_frame_check(self, 0, lbl_80798CAC, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 0x25, 0xB, 0, lbl_80798C70);
        }
        return;
    case 0xE:
        if (em_after_frame_check(self, 0, lbl_80798CBC, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 9, 0x19, 0, lbl_80798CB8);
        }
        return;
    case 0xF:
        if (em_after_frame_check(self, 0, lbl_80798CBC, lbl_80798C74) == 1) {
            fn_801B65B8(self, 0, 9, 0x15, 0, lbl_80798CB8);
        }
        return;
    case 0x10:
        if (em_after_frame_check(self, 0, lbl_80798CA0, lbl_80798C74) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 0x1A, 0x33, 0, lbl_80798CA4);
                return;
            }
            setVector3(&pos, lbl_80798C74, lbl_80798C74, lbl_80798CC0);
            fn_801049D0(self, 0x1A, 9, 0, &pos, lbl_80798CB8);
        }
        return;
    case 0x12:
        if (em_after_frame_check(self, 0, lbl_80798CA0, lbl_80798C74) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&pos, lbl_80798C74, lbl_80798C74, lbl_80798CC4);
                fn_801049D0(self, 0x1A, 0x33, 0, &pos, lbl_80798CA4);
                return;
            }
            setVector3(&pos, lbl_80798C74, lbl_80798C74, lbl_80798CC4);
            fn_801049D0(self, 0x1A, 9, 0, &pos, lbl_80798CB8);
        }
        return;
    case 0x67:
        if (em_after_frame_check(self, 0, lbl_80798CC8, lbl_80798C74) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                setVector3(&pos, lbl_80798CAC, lbl_80798C74, lbl_80798C74);
                fn_801049D0(self, 3, 0x35, 0, &pos, lbl_80798CCC);
                return;
            }
            setVector3(&pos, lbl_80798CAC, lbl_80798C74, lbl_80798C74);
            fn_801049D0(self, 3, 0x24, 0, &pos, lbl_80798CB8);
        }
        return;
    }
}

/* 0x801B6B94 (0xA4).  Seeds the two alpha/colour passes of the embedded MHchar base from the +0x1D4
 * ratio. */
extern "C" void fn_801B6B94(_ENEMY_WORK* self) {
    f32 ratio;
    s32 a;
    s32 c;

    ratio = self->field_0x1D4;
    a = (s32)(lbl_80798CD0 * ratio);
    c = (s32)(lbl_80798CD4 * ratio);
    mhchar_mat_tev_set(self->char_0x024, 0, 6, (u8)a, 0, 3, (u8)c);
    mhchar_mat_tev_set(self->char_0x024, 1, 6, (u8)a, 0, 3, (u8)c);
}

/* 0x801B6C38 (0x4C).  Whether the latch kind is clear and the +0x888 handle can be re-armed (the
 * Haskell side reports none live). */
extern "C" s32 fn_801B6C38(_ENEMY_WORK* self, u8 flag) {
    if (flag == 0 && self->field_0x888 != -1 && em_roster_record_slot_id_get(self->field_0x888) <= 0) {
        return 1;
    }
    return 0;
}

/* 0x801B6C84 (0x270): maps the map kind (`stage_map_kind_get`) and area to a motion pair through `fn_80126324`;
 * with no match it reports the unmatched state and copies the control record's position/rotation onto the work. */
extern "C" void fn_801B6C84(_ENEMY_WORK* self, s8* out_state, s8* out_flag) {
    EmGroundRec rec;
    u32 unmatched = 0;

    em_ground_rec_clear(&rec);
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 1:
            fn_80126324(self, 3, 4, lbl_80798CD8);
            break;
        case 2:
            fn_80126324(self, 7, 8, lbl_80798CDC);
            break;
        case 3:
            fn_80126324(self, 7, 8, lbl_80798CD8);
            break;
        case 4:
            fn_80126324(self, 4, 5, lbl_80798CD8);
            break;
        default:
            unmatched = 1;
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 2:
            fn_80126324(self, 4, 5, lbl_80798CE0);
            break;
        case 6:
            fn_80126324(self, 8, 9, lbl_80798CE0);
            break;
        case 9:
            fn_80126324(self, 4, 5, lbl_80798CE0);
            break;
        default:
            unmatched = 1;
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 7:
            fn_80126324(self, 3, 4, lbl_80798CE0);
            break;
        case 9:
            fn_80126324(self, 3, 4, lbl_80798CE4);
            break;
        default:
            unmatched = 1;
            break;
        }
        break;
    case 5:
        if (self->area_no == 1) {
            fn_80126324(self, 2, 3, lbl_80798CE0);
        } else {
            unmatched = 1;
        }
        break;
    default:
        unmatched = 1;
        break;
    }
    if (unmatched == 1) {
        em_move_mode_set(self, 0);
        *out_state = 0;
        *out_flag = 0;
        if (fn_801421E4(self->field_0x01A, &rec) == 1) {
            copyVec3(&self->pos, &rec.pos_0x08);
            eft_rot_vec_copy_c1(&self->field_0x1BC, &rec.field_0x14);
        }
    } else {
        em_move_mode_set(self, 0);
        *out_state = 0x0C;
        *out_flag = 0;
    }
}

/* 0x801B6EF4 (0xBC).  The `fn_801B4D14` variant on the second control record (+0x016 id, +0x003
 * mode byte). */
extern "C" void fn_801B6EF4(_ENEMY_WORK* self, u8 kind) {
    s8 mode;

    if ((u8)kind == 1 || (u8)kind == 4) {
        mode = fn_801B4C54(*(u16*)&self->field_0x016);
        if (mode == 1) {
            self->team = (u8)(self->team & 0xFD);
        } else if (mode == 2) {
            self->team = (u8)(self->team | 2);
        }
    } else if (self->field_0x010 == 0) {
        mode = fn_801B4C54(*(u16*)&self->field_0x016);
        if (mode == 1) {
            self->team = (u8)(self->team & 0xFD);
        } else if (mode == 2) {
            self->team = (u8)(self->team | 2);
        }
    }
}

/* 0x801B6FB0 (0x6C).  The `.ctors` static initializer: seed the two global control vectors. */
extern "C" void fn_801B6FB0(void) {
    nw4r::math::VEC3 tmp;

    setVec3(&tmp, lbl_80798C74, lbl_80798CE8, lbl_80798CEC);
    assignVec3(vec_pair_801B4458_0, &tmp);
    setVec3(&tmp, lbl_80798C74, lbl_80798CF0, lbl_80798CF4);
    assignVec3(&vec_pair_801B4458_0[1], &tmp);
}

/* 0x801B701C (0x4).  The tail-call slot the neighbouring unit's table keeps. */
extern "C" u32 fn_801B701C(_ENEMY_WORK* self) {
    return fn_80132184();
}

/* The unit's `.bss` (0x806A7AA0-0x806A7AB8): the two-vector record its static constructor `fn_801B6FB0` builds
 * and the `.data` tables point at. */
VEC3 vec_pair_801B4458_0[2];  /* +0x806A7AA0 */
