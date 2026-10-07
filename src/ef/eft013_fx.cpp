/* ef/eft013_fx.cpp - the eft013, eft014 and eft015 effect families: their setters, state dispatchers, hooks and
 *   per-frame handlers.
 * RANGE. .text 0x80107250-0x8010BDE4 (58 functions); extab 0x8000C074-0x8000C1F4, extabindex 0x80025F98-0x800261D8,
 *   .data 0x8059EB50-0x8059F518, .sdata 0x807917A0-0x80791820, .sdata2 0x807967A8-0x807967E8.
 * FLAGS. `cflags_main`; `#pragma peephole off` for the whole file except `fn_80107640` (retail keeps the `u8`
 *   argument `clrlwi`s and the loop `cmpwi`s; peephole on costs the setters, and peephole off costs `fn_80107640`).
 * NAMES. `eft013_set`, `eft013_set_dmeft_pl`, `eft014_set_daihouden` and `eft015_set` are the runtime dump's own
 *   names; the plain C definitions are `extern "C"`.
 *   GUESS (from the bodies and their callers): `eft013_set_from_sel` (0x801073B8), `eft013_set_pl_model`
 *   GUESS: (0x80107BB4), `eft013_setup_effect` (0x80107E5C), `eft013_setup_uv_model` (0x801086C4),
 *   GUESS: `eft013_retire_step` (0x8010A1D4); the data `eft014_effect_ids`, `eft014_type_effect_ids`,
 *   GUESS: `eft014_joint_lists`, `eft014_joint_counts`; the foreign `mtx34_set_trans` (0x800FBB90, `ef/eft001.cpp`).
 * RESIDUALS. 10 rows unwritten: 0x80107E5C-0x801089C0 (`eft013_setup_effect`, `eft013_setup_uv_model`),
 *   0x80108A74-0x8010A1D4 (`fn_80108A74`, `fn_801093F4`, `fn_80109A84`, `fn_80109D00`), 0x8010A49C-0x8010A6A0
 *   (`fn_8010A49C`), 0x8010B198-0x8010BA48 (`fn_8010B198`, `fn_8010B504`, `fn_8010B71C`).
 *   4 partial rows:
 *  - `fn_80107DDC`, `fn_8010AD00`: the case-0 nested dispatch (docs/ef.md, "The case-0 nested dispatch");
 *  - `fn_80107640`: ours compares the type signed with a compare chain where retail uses `subi` + `cmplwi` range
 *    tests, narrows the type with `clrlwi`, and keeps the hook addresses in the stored register;
 *  - `fn_8010A7D4`: retail keeps explicit compares for the empty cases 2 and 5 (ours folds them into the default).
 * *   flipcheck: `.data`/`.sdata` claimed, not emitted; `.text`, extab and extabindex short of the claim; `.sdata2`
 *   0x8 of the claimed 0x40 (the constants are declared, the claim is the pool).
 * SHAPES. Loop counters are declared before the work pointers (retail's register colours); the type tests that
 *   retail compares `cmpwi` are `switch`es; the small-data tables are declared with their sizes (`@sda21`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "enemy.h"
#include "sound/fn_800D7F54.h"
#include "ef/effect.h"
#include "ef/eft004.h"
#include "ef/fn_80105314.h"
#include "ef/eft_res.h"
#include "ef/fn_8010BDE4.h"
#include "gx.h"
#include "pl.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "unsplit/ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/fn_80105314_types.h"
#include "Pl/Pl_master_ck.h" /* Pl_master_ck (rule 2) */
#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2) */
#include "ef/eft001.h" /* mtx34_set_trans (rule 2) */
#include "enemy/fn_8012BDF4.h" /* em_work_die_ck (rule 2) */
#include "menu/get_pop_dat_ptr.h" /* get_option_21 (rule 2) */
#include "fn_8004CAD8.h" /* subVec3 (rule 2) */
#include "Runtime.PPCEABI.H/memset.h" /* memset (rule 2) */
#include "g3d/mtx34_inverse.h" /* mtx34_inverse (rule 2) */
#include "enemy/fn_8012EC74.h" /* get_joint_wmat_em (rule 2) */

#pragma peephole off

/* size: 0x34 - lower bound, an approximation. */

/* The player record the `eft013` setters read (the `_PLW` view, only the offsets this unit uses). */
struct _EFT013_PL {
    /* +0x000 */ u8 unused_0x000;
    /* +0x001 */ u8 flag_0x01;
    /* +0x002 */ u8 unused_0x002[0x016 - 0x002];
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ u8 unused_0x017[0x054 - 0x017];
    /* +0x054 */ _CP_VECTOR rot_0x54;
    /* +0x060 */ u8 unused_0x060[0x171 - 0x060];
    /* +0x171 */ u8 field_0x171;
    /* +0x172 */ u16 field_0x172;
    /* +0x174 */ u8 unused_0x174[0x1A4 - 0x174];
    /* +0x1A4 */ u8 field_0x1A4;
};
/* size: 0x1A5 - lower bound, an approximation. */

/* Pool block of the `fn_80107804` family: the setter state at +0x28 and the two payload bytes. */
struct _EFT013_WORK_E {
    /* +0x00 */ u8 unused_0x00[0x28];
    /* +0x28 */ u8 state_0x28;
    /* +0x29 */ u8 unused_0x29[0x31 - 0x29];
    /* +0x31 */ u8 field_0x31;
    /* +0x32 */ u16 field_0x32;
};
/* size: 0x34 - lower bound, an approximation. */

/* Pool block of the `fn_80107518` family: the placement position at +0x30, a word at +0x40 and the
 * setter state at +0x44. */
struct _EFT013_WORK_F {
    /* +0x00 */ s32 count;
    /* +0x04 */ u8 unused_0x04[0x28 - 0x04];
    /* +0x28 */ f32 scale_0x28;
    /* +0x2C */ f32 scale_0x2C;
    /* +0x30 */ nw4r::math::VEC3 pos_0x30;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ u32 field_0x40;
    /* +0x44 */ u8 field_0x44;
    /* +0x45 */ u8 unused_0x45;
    /* +0x46 */ u8 option_0x46;
};
/* size: 0x48 - lower bound, an approximation. */

/* Work block of the `fn_80107A18` allocator (0x3C bytes): the model, the scale vector and two colours. */
struct _EFT013_WORK_M {
    /* +0x00 */ s32 count;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ u8* model;
    /* +0x0C */ u8 unused_0x0C[0x10 - 0x0C];
    /* +0x10 */ nw4r::math::VEC3 scale_0x10;
    /* +0x1C */ f32 scale_0x1C;
    /* +0x20 */ f32 scale_0x20;
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u8 state_0x28;
    /* +0x29 */ _GXColor color0;
    /* +0x2D */ _GXColor color1;
    /* +0x31 */ u8 field_0x31;
    /* +0x32 */ u16 field_0x32;
    /* +0x34 */ u8 handles[0x3C - 0x34];
};
/* size: 0x3C */

/* The selector record `fn_80107914` reads: a pointer at +0x2C and the variant at +0x30. */
struct _EFT013_SEL {
    /* +0x00 */ u8 unused_0x00[0x0F];
    /* +0x0F */ u8 area_0x0F;
    /* +0x10 */ u8 unused_0x10[0x28 - 0x10];
    /* +0x28 */ s16* joint_0x28;
    /* +0x2C */ void* field_0x2C;
    /* +0x30 */ u8 field_0x30;
};
/* size: 0x34 - lower bound, an approximation. */

/* The `fn_80107914` case-0/case-3 source (player or shell record): the id bytes and the area/key. */
struct _EFT013_PL2 {
    /* +0x000 */ u8 unused_0x000[0x0A];
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 unused_0x00B;
    /* +0x00C */ u16 field_0x00C;
    /* +0x00E */ u8 unused_0x00E[0x016 - 0x00E];
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ u8 unused_0x017[0x171 - 0x017];
    /* +0x171 */ u8 field_0x171;
    /* +0x172 */ u16 field_0x172;
    /* +0x174 */ u8 unused_0x174[0x1A4 - 0x174];
    /* +0x1A4 */ u8 field_0x1A4;
};
/* size: 0x1A5 - lower bound, an approximation. */

/* The `fn_80107914` case-1 enemy view: the action/state bytes and the word at +0x204. */
struct _EFT013_ENEMY_VIEW {
    /* +0x000 */ u8 unused_0x000[0x1E1];
    /* +0x1E1 */ u8 act_id;
    /* +0x1E2 */ u8 unused_0x1E2[0x1E5 - 0x1E2];
    /* +0x1E5 */ u8 action_0x1E5;
    /* +0x1E6 */ u8 state_sub;
    /* +0x1E7 */ u8 unused_0x1E7[0x204 - 0x1E7];
    /* +0x204 */ u32 field_0x204;
};
/* size: 0x208 - lower bound, an approximation. */

/* The enemy-data record `fn_80107914` scales by: a stat pointer at +0x9C. */
/* size: 0x08 - lower bound, an approximation. */
struct _EFT013_ENEMY_STAT {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ f32 scale_0x04;
};
struct _EFT013_ENEMY_DATA {
    /* +0x00 */ u8 unused_0x00[0x9C];
    /* +0x9C */ _EFT013_ENEMY_STAT* field_0x9C;
};
/* size: 0xA0 - lower bound, an approximation. */

/* Pool block of the `fn_80107D20` family: the effect array at +0x04 and the two g3d model handles. */
struct _EFT013_WORK_H {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[1];
    /* +0x08 */ u8 unused_0x08[0x34 - 0x08];
    /* +0x34 */ void* models[2];
};
/* size: 0x3C - lower bound, an approximation. */

/* The model-object record `fn_8010A6A0` walks: the field it stamps at +0xB4. */
struct _EFT014_OBJ {
    /* +0x00 */ u8 unused_0x00[0xB4];
    /* +0xB4 */ u32 field_0xB4;
};
/* size: 0xB8 - lower bound, an approximation. */

/* Pool block of the `fn_8010A394` family: the three keyframe-interpolated colour bytes. */
struct _EFT013_WORK_K {
    /* +0x00 */ u8 unused_0x00[0x29];
    /* +0x29 */ u8 field_0x29;
    /* +0x2A */ u8 field_0x2A;
    /* +0x2B */ u8 field_0x2B;
};
/* size: 0x2C - lower bound, an approximation. */

/* Pool blocks of the `fn_8010AB1C` / `fn_8010AB88` / `fn_8010BD3C` release loops. */
/* size: 0x0C - lower bound, an approximation. */
struct _EFT014_SLOT_A {
    /* +0x00 */ nw4r::ef::Effect* effect;
    /* +0x04 */ u8 pad_0x04[0x0C - 0x04];
};
/* size: 0xD4 - lower bound, an approximation. */
struct _EFT014_POOL_A {
    /* +0x00 */ s32 count;
    /* +0x04 */ _EFT014_SLOT_A slots[0x10];
    /* +0xC4 */ u8 unused_0xC4[0xCF - 0xC4];
    /* +0xCF */ u8 phase_0xCF;
    /* +0xD0 */ u8 count_0xD0;
};
/* size: 0x08 - lower bound, an approximation. */
struct _EFT014_POOL_B {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* slots[1];
};
/* One shell-trail slot of the eft015 pool: four state bytes, the model list, the velocity and the position. */
/* size: 0x48 */
struct _EFT014_SLOT_C {
    /* +0x00 */ u8 state[4];
    /* +0x04 */ u8* model;
    /* +0x08 */ nw4r::math::VEC3 vel;
    /* +0x14 */ u8 unused_0x14[0x20 - 0x14];
    /* +0x20 */ nw4r::math::VEC3 pos;
    /* +0x2C */ u8 unused_0x2C[0x48 - 0x2C];
};
struct _EFT014_POOL_C {
    /* +0x00 */ s32 count;
    /* +0x04 */ _EFT014_SLOT_C slots[2];
};
/* size: 0x94 - lower bound, an approximation (the pool slot is 0x9C bytes). */

/* One slot of the `fn_8010ABF4` / `fn_8010AF50` pool: the model list it releases and its start delay. */
struct _EFT014_SLOT_D {
    /* +0x00 */ void* models;
    /* +0x04 */ u8 unused_0x04[0x19 - 0x04];
    /* +0x19 */ u8 delay_0x19;
    /* +0x1A */ u8 unused_0x1A[0x1C - 0x1A];
};
/* size: 0x1C */

/* Pool block of the `fn_8010ABF4` / `fn_8010AF50` family: three slots and two g3d handles per slot. */
struct _EFT014_POOL_D {
    /* +0x00 */ s32 count;
    /* +0x04 */ _EFT014_SLOT_D slots[3];
    /* +0x58 */ u8 unused_0x58[0x5C - 0x58];
    /* +0x5C */ struct _g3d_work* handles[3][2];
};
/* size: 0x74 - lower bound, an approximation. */

struct _PLW;

/* mtx34_trans_get comes from its owner's header (rule 2). */
/* fn_800DC60C / fn_800DB964 come from their owner's header (rule 2). */
extern "C" _EFT013* fn_80107640(void* self, u32 a, f32 scale, u32 areano);
extern "C" void* fn_80107A18(void* self, u8 a, u32 b, f32 scale, u8 c);
extern "C" void eft013_set_from_sel(_EFT013_SEL* self, nw4r::math::VEC3* pos, u8 type);

extern "C" u8 lbl_8059EE1C[];
extern "C" u8 lbl_80791808[8];
extern "C" s32* eft014_joint_lists[5];  /* per slot: the joints a trail effect may sit on  .data 0x8059F3C0 */
extern "C" s32 eft014_joint_counts[5];  /* per slot: the length of its joint list         .data 0x8059F3D4 */
extern "C" u16 eft014_effect_ids[];      /* the type-1 effect ids, one per pool slot  .data 0x8059F3E8 */
extern "C" u16 eft014_type_effect_ids[4];/* the effect id of types 3..6               .sdata 0x80791810 */

extern "C" f32 lbl_807967A8;
extern "C" f32 lbl_807967AC;

extern "C" f32 lbl_8059EE68[];
extern "C" f32 lbl_8059EEE8[];
extern "C" f32 lbl_8059EF68[];
extern "C" f32 lbl_8059EFB8[];
extern "C" f32 lbl_8059F028[];

void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
/* `se_req_pos_ps` comes from the owner's header `sound/fn_800D7F54.h` (rule 2); this unit's local
 * `void` copy collided with the owner's `SeSlot*` once the header declared it. */
u8 get_now_areano();

f32 get_em_scale(_ENEMY_WORK* enemy);
void* get_enemy_data(_ENEMY_WORK* enemy);
/* target references push_g3d_wk__FP9_g3d_work: C++ linkage with the _g3d_work* parameter */
struct _g3d_work;
void push_g3d_wk(struct _g3d_work* work);
extern "C" u16 fn_800A51D0(void* obj);
extern "C" void* fn_800A51D8(void* obj, u16 index);
void getKeyData3(f32* keys, f32 frame, f32* out0, f32* out1, f32* out2);

extern "C" void fn_80107CA0(_EFT013* self);
extern "C" void fn_80107DDC(_EFT013* self);
extern "C" void eft013_setup_effect(_EFT013* self);
extern "C" void eft013_setup_uv_model(_EFT013* self);
extern "C" void eft013_retire_step(_EFT013* self);
extern "C" void fn_80107CE4(_EFT013* self);
extern "C" void fn_80107D20(_EFT013* self);
extern "C" void fn_80107DA0(_EFT013* self);
extern "C" void fn_80108A74(_EFT013* self);
extern "C" void fn_801093F4(_EFT013* self);
extern "C" void fn_80109A84(_EFT013* self);
extern "C" void fn_80109D00(_EFT013* self);
extern "C" void fn_80108A2C(_EFT013* self);
extern "C" void fn_8010AA24(_EFT013* e, _ENEMY_WORK* self, u8 a, nw4r::math::VEC3* v, u32 b, u32 c);
extern "C" void fn_8010AB1C(_EFT013* self);
extern "C" void fn_8010AB88(_EFT013* self);
extern "C" void fn_8010ABF4(_EFT013* self);
extern "C" void fn_8010AC94(_EFT013* self);
extern "C" void fn_8010AD90(_EFT013* self);
extern "C" void fn_8010AE0C(_EFT013* self);
extern "C" void fn_8010AF50(_EFT013* self);
extern "C" void fn_8010B008(_EFT013* self);
extern "C" void fn_8010B154(_EFT013* self);
extern "C" void fn_8010B198(_EFT013* self);
extern "C" void fn_8010B504(_EFT013* self);
extern "C" void fn_8010B71C(_EFT013* self);
extern "C" void fn_8010BA48(_EFT013* self);
s32 ran_suu(long kind);

#pragma peephole off

/* 0x8010A318 - destroy the effect object. */
extern "C" void fn_8010A318(_EFT013* self) {
    eft_res_slot_release(self);
}

/* 0x8010BBCC - the trivial state advance. */
extern "C" void fn_8010BBCC(_EFT013* self) {
    self->state_0x05++;
}

/* 0x8010BBDC - destroy the effect object. */
extern "C" void fn_8010BBDC(_EFT013* self) {
    eft_res_slot_release(self);
}

/* 0x8010BDA8 - state_0x05 dispatcher for the fn_8010BDE4 family. */
extern "C" void fn_8010BDA8(_EFT013* self) {
    switch (self->state_0x05) {
    case 0:
        fn_8010BDE4((_EFT*)self);
        return;
    case 1:
        fn_8010C0E0((_EFT*)self);
        return;
    case 2:
        fn_8010C454((_EFT*)self);
        return;
    case 3:
        fn_8010C464((_EFT*)self);
        return;
    default:
        return;
    }
}

#pragma peephole off

/* 0x8010AAD0 - type dispatch over the lbl_80791808 table. */
extern "C" void fn_8010AAD0(_EFT013* self) {
    switch (lbl_80791808[self->type_0x02]) {
    case 0:
        fn_8010AB1C(self);
        return;
    case 1:
    case 4:
        fn_8010AB88(self);
        return;
    case 2:
        fn_8010ABF4(self);
        return;
    case 3:
        fn_8010AC94(self);
        return;
    default:
        return;
    }
}

/* 0x8010AD00 - the state_0x05 dispatcher; state 0 is the nested type dispatch. */
extern "C" void fn_8010AD00(_EFT013* self) {
    switch (self->state_0x05) {
    case 1:
        fn_8010B154(self);
        return;
    case 2:
        fn_8010BBCC(self);
        return;
    case 3:
        fn_8010BBDC(self);
        return;
    case 0:
        switch (lbl_80791808[self->type_0x02]) {
        case 0:
            fn_8010AD90(self);
            return;
        case 1:
        case 4:
            fn_8010AE0C(self);
            return;
        case 2:
            fn_8010AF50(self);
            return;
        case 3:
        case 5:
            fn_8010B008(self);
            return;
        default:
            return;
        }
    default:
        return;
    }
}

/* 0x8010AD90 - state-0 setup: copy the source's flag/area, roll the type-0 random word and hand off to
 * the fn_8010B154 type dispatch. */
extern "C" void fn_8010AD90(_EFT013* self) {
    _EFT014_POOL_A* work = (_EFT014_POOL_A*)self->work_0x38;
    _ENEMY_WORK* src = self->source_0x30;
    self->state_0x05++;
    work->phase_0xCF = 0;
    self->flag_0x01 = src->pad_0x1[0];
    self->area_0x44 = src->act_id;
    if (self->type_0x02 == 0) {
        self->field_0x10 = (u16)ran_suu(0) & 1;
    }
    fn_8010B154(self);
}

/* 0x8010B154 - type dispatch over the lbl_80791808 table. */
extern "C" void fn_8010B154(_EFT013* self) {
    switch (lbl_80791808[self->type_0x02]) {
    case 0:
        fn_8010B198(self);
        return;
    case 1:
        fn_8010B71C(self);
        return;
    case 3:
        fn_8010B504(self);
        return;
    case 4:
        fn_8010BA48(self);
        return;
    default:
        return;
    }
}

/* 0x8010A394 - per-frame colour key: pick the type's keyframe table, interpolate at the frame
 * counter and store the three colour bytes. */
extern "C" void fn_8010A394(_EFT013* self) {
    _EFT013_WORK_K* work = (_EFT013_WORK_K*)self->work_0x38;
    f32* keys;
    switch (self->type_0x02) {
    case 9:
        keys = lbl_8059EE68;
        break;
    case 13:
    case 28:
        keys = lbl_8059EEE8;
        break;
    case 26:
        keys = lbl_8059EF68;
        break;
    case 27:
        keys = lbl_8059EFB8;
        break;
    case 29:
        keys = lbl_8059F028;
        break;
    default:
        break;
    }
    f32 o0;
    f32 o1;
    f32 o2;
    getKeyData3(keys, (f32)self->field_0x10, &o0, &o1, &o2);
    work->field_0x29 = (u8)o0;
    work->field_0x2A = (u8)o1;
    work->field_0x2B = (u8)o2;
}

/* 0x8010A7D4 - spawn the enemy effect the type selects (the 212-slot variant at area 0/7, the 72-slot
 * one at type 1) and hand it to fn_8010AA24. */
extern "C" void fn_8010A7D4(void* self, u32 type) {
    switch ((u8)type) {
    case 0:
    case 7: {
        _ENEMY_WORK* enemy = (_ENEMY_WORK*)self;
        if (enemy->act_id != get_now_areano()) {
            return;
        }
        _EFT013* e = (_EFT013*)eft_res_slot_get(212);
        if (e == NULL) {
            return;
        }
        ((_EFT013_WORK_F*)e->work_0x38)->count = 16;
        fn_8010AA24(e, enemy, (u8)type, &enemy->pos, 0, 0);
        return;
    }
    case 1: {
        _ENEMY_WORK* enemy = (_ENEMY_WORK*)self;
        _EFT013* e = (_EFT013*)eft_res_slot_get(72);
        if (e == NULL) {
            return;
        }
        fn_8010AA24(e, enemy, (u8)type, &enemy->pos, 1, 0);
        _EFT013_WORK_F* work = (_EFT013_WORK_F*)e->work_0x38;
        work->count = 7;
        work->field_0x44 = 0;
        return;
    }
    case 2:
    case 5:
    default:
        return;
    }
}

/* 0x8010AB1C - release the 12-byte-stride effect pool. */
extern "C" void fn_8010AB1C(_EFT013* self) {
    s32 i;
    _EFT014_POOL_A* work = (_EFT014_POOL_A*)self->work_0x38;
    for (i = 0; i < work->count_0xD0; i++) {
        push_eft_effect_heap_num(&work->slots[i].effect, 1);
    }
    work->count = 0;
}

/* 0x8010AB88 - release the 4-byte-stride effect pool. */
extern "C" void fn_8010AB88(_EFT013* self) {
    s32 i;
    _EFT014_POOL_B* work = (_EFT014_POOL_B*)self->work_0x38;
    for (i = 0; i < work->count; i++) {
        push_eft_effect_heap_num(&work->slots[i], 1);
    }
    work->count = 0;
}

/* 0x8010BD3C - release the 72-byte-stride model pool. */
extern "C" void fn_8010BD3C(_EFT013* self) {
    s32 i;
    _EFT014_POOL_C* work = (_EFT014_POOL_C*)self->work_0x38;
    for (i = 0; i < work->count; i++) {
        fn_800F8A44(&work->slots[i].model, 1);
    }
    work->count = 0;
}

/* 0x8010A31C - transform the position triple by the matrix and add the matrix's translation column. */
extern "C" void fn_8010A31C(_EFT013* self, nw4r::math::MTX34* mtx, nw4r::math::VEC3* v) {
    mulVecMat(v, mtx);
    self->pos_0x18.x = mtx->m[0][3] + v->x;
    self->pos_0x18.y = mtx->m[1][3] + v->y;
    self->pos_0x18.z = mtx->m[2][3] + v->z;
}

/* 0x8010A6A0 - stamp the walker's +0xB4 field on every live model of the object. */
extern "C" void fn_8010A6A0(void* self, void* obj) {
    u16 n = fn_800A51D0(obj);
    for (s32 i = 0; i < n; i++) {
        if (i < (u16)fn_800A51D0(obj)) {
            ((_EFT014_OBJ*)fn_800A51D8(obj, i))->field_0xB4 = 3;
        }
    }
}

/* 0x8010A728 - show/hide the model's material slots for the two visibility modes. */
extern "C" void fn_8010A728(_EFT013* self, u32 mode) {
    s32 i;
    _EFT013_WORK_H* work = (_EFT013_WORK_H*)self->work_0x38;
    switch ((u8)mode) {
    case 0:
        for (i = 2; i < 4; i++) {
            ((MHchar*)work->effects[0])->setVisibility(i, true);
        }
        ((MHchar*)work->effects[0])->setVisibility(1, false);
        break;
    case 1:
        for (i = 2; i < 4; i++) {
            ((MHchar*)work->effects[0])->setVisibility(i, false);
        }
        ((MHchar*)work->effects[0])->setVisibility(1, true);
        break;
    default:
        break;
    }
}

/* 0x8010A8CC - spawn a 12-slot enemy effect at joint 3 and hand it to fn_8010AA24. */
extern "C" void fn_8010A8CC(_ENEMY_WORK* self, u32 a, f32 scale) {
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    if (self->act_id != get_now_areano()) {
        return;
    }
    _EFT013* e = (_EFT013*)eft_res_slot_get(12);
    if (e == NULL) {
        return;
    }
    _EFT013_POOL* work = (_EFT013_POOL*)e->work_0x38;
    work->count = 1;
    work->scale_0x08 = scale;
    get_joint_wpos_em(self, 3, &v);
    fn_8010AA24(e, self, (u8)a, &v, 0, 0);
}

/* 0x80107CA0 - state_0x05 dispatcher for the eft013 family. */
extern "C" void fn_80107CA0(_EFT013* self) {
    switch (lbl_8059EE1C[self->type_0x02]) {
    case 0:
    case 3:
        fn_80107CE4(self);
        return;
    case 1:
        fn_80107D20(self);
        return;
    case 2:
        fn_80107DA0(self);
        return;
    default:
        return;
    }
}

/* 0x80107CE4 - release the pool block (effects at +0x04). */
extern "C" void fn_80107CE4(_EFT013* self) {
    _EFT013_POOL* pool = (_EFT013_POOL*)self->work_0x38;
    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* 0x80107D20 - release the pool block, tear down the two g3d model handles and release the model pool. */
extern "C" void fn_80107D20(_EFT013* self) {
    s32 i;
    _EFT013_WORK_H* work = (_EFT013_WORK_H*)self->work_0x38;
    push_eft_effect_heap_num(work->effects, work->count);
    for (i = 1; i >= 0; i--) {
        if (work->models[i] != NULL) {
            push_g3d_wk((struct _g3d_work*)work->models[i]);
        }
    }
    fn_800F8A44(work->unused_0x08, 1);
    work->count = 0;
}

/* 0x80107DA0 - release the model pool (pointer at +0x04). */
extern "C" void fn_80107DA0(_EFT013* self) {
    _EFT013_WORK_H* work = (_EFT013_WORK_H*)self->work_0x38;
    fn_800F8A44(work->effects, work->count);
    work->count = 0;
}

/* 0x801089C0 - create the model handle (res 78, arg 8) and hand off to the fn_80108A2C dispatcher. */
extern "C" void fn_801089C0(_EFT013* self) {
    _EFT013_WORK_H* work = (_EFT013_WORK_H*)self->work_0x38;
    self->state_0x05++;
    if (res_eft_model_create((struct MHchar*)work->effects[0], 78, 8) == NULL) {
        fn_8010A318(self);
        return;
    }
    self->flag_0x01 = 1;
    fn_80108A2C(self);
}

/* 0x80108A2C - state_0x05 dispatcher for the fn_80108A74 family. */
extern "C" void fn_80108A2C(_EFT013* self) {
    switch (lbl_8059EE1C[self->type_0x02]) {
    case 0:
        fn_80108A74(self);
        return;
    case 1:
        fn_801093F4(self);
        return;
    case 2:
        fn_80109A84(self);
        return;
    case 3:
        fn_80109D00(self);
        return;
    default:
        return;
    }
}

/* 0x80107314 - player setter: spawn via fn_80107640 and store the caller's id word. */
extern "C" void fn_80107314(_EFT013_PL* self, u32 a, u32 b, f32 scale) {
    u32 areano = self->area_0x16;
    if (areano != get_now_areano()) {
        return;
    }
    if (self->flag_0x01 == 0) {
        return;
    }
    _EFT013* e = fn_80107640(self, (u8)a, scale, areano);
    if (e == NULL) {
        return;
    }
    e->field_0x10 = (u8)b;
    ((_EFT013_WORK_F*)e->work_0x38)->field_0x44 = 0;
}

#pragma peephole on

/* 0x80107640 - the eft013 allocator: pool a 72-slot object, seed the work block's count/scale from the
 * type and install the fn_80107CA0/fn_80107DDC pair; the type also selects the eft_state_flags_set flags. */
extern "C" _EFT013* fn_80107640(void* self, u32 a, f32 scale, u32 areano) {
    _EFT013* e = (_EFT013*)eft_res_slot_get(72);
    if (e == NULL) {
        return NULL;
    }
    _EFT013_WORK_F* work = (_EFT013_WORK_F*)e->work_0x38;
    if (((u8)a - 26) <= 1) {
        work->count = 0;
    } else {
        work->count = 1;
    }
    work->scale_0x28 = scale;
    work->scale_0x2C = scale;
    e->source_0x30 = (_ENEMY_WORK*)self;
    e->area_0x44 = (u8)areano;
    e->field_0x03 = 13;
    e->type_0x02 = (u8)a;
    e->timer_0x0C = 0;
    e->field_0x10 = 0;
    switch (e->type_0x02) {
    case 6:
    case 7:
        eft_state_flags_set((_EFT*)e, 1, 0);
        break;
    case 0:
    case 1:
    case 3:
    case 4:
    case 11:
        eft_state_flags_set((_EFT*)e, 0, 4);
        break;
    default:
        eft_state_flags_set((_EFT*)e, 0, 0);
        break;
    }
    e->release_0x40 = fn_80107CA0;
    e->dispatch_0x34 = fn_80107DDC;
    return e;
}

#pragma peephole off

/* 0x80107914 - the per-variant setter: read the source record the selector names, then spawn the type
 * through fn_80107A18 and seed the work block's state/payload. */
extern "C" void fn_80107914(_EFT013_SEL* self) {
    _EFT013_ENEMY_VIEW* enemy;
    u32 b;
    u32 areano;
    u8 v8;
    u32 state;
    u16 v16;
    f32 scale;
    switch (self->field_0x30) {
    case 0: {
        _EFT013_PL2* p = (_EFT013_PL2*)self->field_0x2C;
        state = 0;
        areano = p->area_0x16;
        v16 = p->field_0x00C;
        v8 = p->field_0x00A;
        scale = lbl_807967A8;
        b = 3;
        break;
    }
    case 1: {
        enemy = (_EFT013_ENEMY_VIEW*)self->field_0x2C;
        state = 1;
        areano = enemy->act_id;
        v16 = enemy->state_sub;
        v8 = enemy->action_0x1E5;
        f32 s = get_em_scale((_ENEMY_WORK*)enemy);
        _EFT013_ENEMY_DATA* ed = (_EFT013_ENEMY_DATA*)get_enemy_data((_ENEMY_WORK*)enemy);
        scale = ed->field_0x9C->scale_0x04 * s;
        b = enemy->field_0x204;
        break;
    }
    case 3: {
        _EFT013_PL2* p = (_EFT013_PL2*)self->field_0x2C;
        state = 2;
        areano = p->field_0x1A4;
        v16 = p->field_0x172;
        v8 = p->field_0x171;
        scale = lbl_807967AC;
        b = 2;
        break;
    }
    default:
        break;
    }
    _EFT013* e = (_EFT013*)fn_80107A18(self->field_0x2C, 13, b, scale, areano);
    if (e != NULL) {
        _EFT013_WORK_E* work = (_EFT013_WORK_E*)e->work_0x38;
        work->state_0x28 = (u8)state;
        work->field_0x32 = v16;
        work->field_0x31 = v8;
    }
}

/* 0x80107518 - enemy-joint setter: spawn via `fn_80107640` and place the pooled effect (state 1). */
extern "C" void fn_80107518(_ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a, f32 scale, u32 b) {
    u32 areano = self->act_id;
    if (areano != get_now_areano()) {
        return;
    }
    _EFT013* e = (_EFT013*)fn_80107640(self, (u8)b, scale, areano);
    if (e != NULL) {
        _EFT013_WORK_F* work = (_EFT013_WORK_F*)e->work_0x38;
        work->field_0x44 = 1;
        copyVec3(&work->pos_0x30, pos);
        work->field_0x40 = a;
    }
}

/* 0x801075AC - the player variant of fn_80107518 (state 2, area from the player record). */
extern "C" void fn_801075AC(_EFT013_PL* self, nw4r::math::VEC3* pos, u32 a, f32 scale, u32 b) {
    u32 areano = self->field_0x1A4;
    if (areano != get_now_areano()) {
        return;
    }
    _EFT013* e = (_EFT013*)fn_80107640(self, (u8)b, scale, areano);
    if (e != NULL) {
        _EFT013_WORK_F* work = (_EFT013_WORK_F*)e->work_0x38;
        work->field_0x44 = 2;
        copyVec3(&work->pos_0x30, pos);
        work->field_0x40 = a;
    }
}

/* 0x801077C4 - clear the setter state of the pooled type-9 effect. */
extern "C" void fn_801077C4(_EFT013_PL* self) {
    f32 scale;
    _EFT013* e = (_EFT013*)fn_80107A18(self, 9, 3, scale, self->area_0x16);
    if (e != NULL) {
        ((_EFT013_WORK_E*)e->work_0x38)->state_0x28 = 0;
    }
}

/* 0x80107804 - arm the enemy setter (state 1) and copy the action/state bytes into the work block. */
extern "C" void fn_80107804(_ENEMY_WORK* self, u32 a) {
    f32 scale;
    _EFT013* e = (_EFT013*)fn_80107A18(self, 9, a, scale, self->act_id);
    if (e != NULL) {
        _EFT013_WORK_E* work = (_EFT013_WORK_E*)e->work_0x38;
        work->state_0x28 = 1;
        work->field_0x32 = self->state_sub;
        work->field_0x31 = self->action_0x1E5;
    }
}

/* 0x80107860 - the player variant of fn_80107804 (state 2, player record's key/state bytes). */
extern "C" void fn_80107860(_EFT013_PL* self, u32 a) {
    f32 scale;
    _EFT013* e = (_EFT013*)fn_80107A18(self, 9, a, scale, self->field_0x1A4);
    if (e != NULL) {
        _EFT013_WORK_E* work = (_EFT013_WORK_E*)e->work_0x38;
        work->state_0x28 = 2;
        work->field_0x32 = self->field_0x172;
        work->field_0x31 = self->field_0x171;
    }
}

/* 0x801078BC - arm the enemy setter (state 1) with the caller's id in the low byte. */
extern "C" void fn_801078BC(_ENEMY_WORK* self, u32 id, u32 b) {
    f32 scale;
    _EFT013* e = (_EFT013*)fn_80107A18(self, (u8)id, b, scale, self->act_id);
    if (e != NULL) {
        _EFT013_WORK_E* work = (_EFT013_WORK_E*)e->work_0x38;
        work->state_0x28 = 1;
        work->field_0x32 = self->state_sub;
        work->field_0x31 = self->action_0x1E5;
    }
}

/* 0x80107250 (0xC4): the player setter: spawns the eft013 type in the player's area; the master-player types
 * need `Pl_master_ck`, types 10 and 25 need the player's flag. */
void eft013_set(struct _PLW* plw, u8 type) {
    _EFT013_PL* self = (_EFT013_PL*)plw;
    if (self->area_0x16 != get_now_areano()) {
        return;
    }
    switch (type) {
    case 0:
    case 3:
    case 4:
    case 11:
        if (Pl_master_ck(plw) == 0) {
            return;
        }
        break;
    case 10:
    case 25:
        if (self->flag_0x01 == 0) {
            return;
        }
        break;
    }
    _EFT013* e = fn_80107640(plw, type, lbl_807967A8, self->area_0x16);
    if (e != NULL) {
        ((_EFT013_WORK_F*)e->work_0x38)->field_0x44 = 0;
    }
}

#pragma peephole off

/* 0x80107784 (0x40): arms the player's damage effect of `type` and clears its setter state. */
void eft013_set_dmeft_pl(struct _PLW* plw, u8 type, f32 scale) {
    _EFT013* e = (_EFT013*)fn_80107A18(plw, type, 3, scale, ((_EFT013_PL*)plw)->area_0x16);
    if (e != NULL) {
        ((_EFT013_WORK_E*)e->work_0x38)->state_0x28 = 0;
    }
}

#pragma peephole off

/* 0x80107BB4 (0xEC): pools an eight-byte eft013 object for the player with one model and installs the
 * eft013 release/dispatch pair. */
extern "C" void eft013_set_pl_model(_EFT013_PL* self, u8 type) {
    _EFT013* e = (_EFT013*)eft_res_slot_get(8);
    if (e == NULL) {
        return;
    }
    e->type_0x02 = type;
    e->release_0x40 = fn_80107CA0;
    e->dispatch_0x34 = fn_80107DDC;
    _EFT013_WORK_H* work = (_EFT013_WORK_H*)e->work_0x38;
    work->count = 1;
    for (s32 i = 0; i < work->count; i++) {
        work->effects[i] = (nw4r::ef::Effect*)eft_res_model_get();
        if (work->effects[i] == NULL) {
            eft_res_slot_release(e);
            return;
        }
    }
    e->source_0x30 = (_ENEMY_WORK*)self;
    e->area_0x44 = self->area_0x16;
    e->field_0x03 = 13;
    eft_rot_vec_copy(&e->rot_0x24, &self->rot_0x54);
    e->timer_0x0C = 0;
    e->field_0x10 = 0;
    eft_state_flags_set((_EFT*)e, 1, 0);
}

/* 0x80107DDC (0x80): the eft013 state dispatcher; state 0 picks the type's setup. */
extern "C" void fn_80107DDC(_EFT013* self) {
    switch (self->state_0x05) {
    case 0:
        switch (lbl_8059EE1C[self->type_0x02]) {
        case 0:
        case 3:
            eft013_setup_effect(self);
            return;
        case 1:
            eft013_setup_uv_model(self);
            return;
        case 2:
            fn_801089C0(self);
            return;
        default:
            return;
        }
    case 1:
        fn_80108A2C(self);
        return;
    case 2:
        eft013_retire_step(self);
        return;
    case 3:
        fn_8010A318(self);
        return;
    default:
        return;
    }
}

/* 0x8010AA24 (0xAC): seeds a pooled eft014 object from the enemy (position, rotation, area) and installs the
 * eft014 release/dispatch pair. */
extern "C" void fn_8010AA24(_EFT013* e, _ENEMY_WORK* self, u8 a, nw4r::math::VEC3* v, u32 b, u32 c) {
    copyVec3(&e->pos_0x18, v);
    e->rot_0x24.x = self->pos_0x1BC.x;
    e->rot_0x24.y = self->pos_0x1BC.y;
    e->rot_0x24.z = 0;
    e->field_0x03 = 14;
    e->type_0x02 = a;
    e->timer_0x0C = 0;
    e->field_0x10 = 0;
    e->source_0x30 = self;
    e->area_0x44 = self->act_id;
    eft_state_flags_set((_EFT*)e, b, c);
    e->release_0x40 = fn_8010AAD0;
    e->dispatch_0x34 = fn_8010AD00;
}

/* 0x8010A988 (0x9C): spawns the 12-slot cannon effect at `pos` in the enemy's area. */
void eft014_set_daihouden(_ENEMY_WORK* self, f32 scale, nw4r::math::VEC3* pos) {
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    if (self->act_id != get_now_areano()) {
        return;
    }
    _EFT013* e = (_EFT013*)eft_res_slot_get(12);
    if (e == NULL) {
        return;
    }
    _EFT013_POOL* work = (_EFT013_POOL*)e->work_0x38;
    work->count = 1;
    work->scale_0x08 = scale;
    fn_8010AA24(e, self, 4, pos, 0, 0);
}

/* 0x8010ABF4 (0xA0): releases the three slots' g3d handles and model lists. */
extern "C" void fn_8010ABF4(_EFT013* self) {
    s32 k;
    s32 i;
    _EFT014_POOL_D* work = (_EFT014_POOL_D*)self->work_0x38;
    for (k = 0; k < 3; k++) {
        for (i = 1; i >= 0; i--) {
            if (work->handles[k][i] != NULL) {
                push_g3d_wk(work->handles[k][i]);
            }
        }
        if (work->slots[k].models != NULL) {
            fn_800F8A44(&work->slots[k], 1);
        }
    }
    work->count = 0;
}

/* 0x8010AC94 (0x6C): releases the 4-byte-stride effect pool. */
extern "C" void fn_8010AC94(_EFT013* self) {
    s32 i;
    _EFT014_POOL_B* work = (_EFT014_POOL_B*)self->work_0x38;
    for (i = 0; i < work->count; i++) {
        push_eft_effect_heap_num(&work->slots[i], 1);
    }
    work->count = 0;
}

/* 0x8010AF50 (0xB8): state-0 setup of the slot pool: staggers the slots' start delays at random, copies the
 * source's flag and area and hands off to the fn_8010B154 type dispatch. */
extern "C" void fn_8010AF50(_EFT013* self) {
    s32 i;
    _EFT014_POOL_D* work = (_EFT014_POOL_D*)self->work_0x38;
    _ENEMY_WORK* src = self->source_0x30;
    self->state_0x05++;
    work->slots[0].delay_0x19 = 1;
    for (i = 1; i < work->count; i++) {
        work->slots[i].delay_0x19 = i * 2;
        work->slots[i].delay_0x19 += ran_suu(0) & 3;
        work->slots[i].delay_0x19 += 1;
    }
    self->flag_0x01 = src->pad_0x1[0];
    self->area_0x44 = src->act_id;
    fn_8010B154(self);
}

/* 0x8010A1D4 (0x144): state 2 of the follow types (5, 8, 21): retires the emitters once, then keeps the effects
 * on the object's position until the counter runs out; every other type advances at once. */
extern "C" void eft013_retire_step(_EFT013* self) {
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    s32 i;
    switch (self->type_0x02) {
    case 5:
    case 8:
    case 21: {
        self->flag_0x01 = 1;
        _EFT013_POOL* pool = (_EFT013_POOL*)self->work_0x38;
        switch (self->field_0x06) {
        case 0:
            self->field_0x06++;
            for (i = 0; i < pool->count; i++) {
                pool->effects[i]->RetireEmitterAll();
            }
        case 1:
            for (i = 0; i < pool->count; i++) {
                SetRootMtxTrans(pool->effects[i], &self->pos_0x18);
                effect_retire(pool->effects[i], 0);
            }
            if (--self->field_0x10 < 0) {
                self->state_0x05++;
                return;
            }
            eft_res_models_spawn((_EFT*)self, (void**)pool->effects, 1, pool->count, NULL);
            return;
        }
        break;
    }
    default:
        self->state_0x05++;
        break;
    }
}

/* 0x801073B8 (0x160): the selector setter: spawns the eft013 type for the selector's source in its area and
 * places it - free (variant 0), on the source's joint as a joint-local offset (1), or on the player (3). */
extern "C" void eft013_set_from_sel(_EFT013_SEL* self, nw4r::math::VEC3* pos, u8 type) {
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    u8 areano = self->area_0x0F;
    if (areano != get_now_areano()) {
        return;
    }
    _EFT013* e = fn_80107640(self->field_0x2C, type, lbl_807967A8, areano);
    if (e == NULL) {
        return;
    }
    _EFT013_WORK_F* work = (_EFT013_WORK_F*)e->work_0x38;
    switch (self->field_0x30) {
    case 0:
        work->field_0x44 = 0;
        return;
    case 1: {
        nw4r::math::VEC3 d;
        if (get_option_21() != 0) {
            work->option_0x46 = 1;
        }
        work->field_0x44 = 1;
        work->field_0x40 = *self->joint_0x28;
        get_joint_wmat_em(e->source_0x30, work->field_0x40, &mtx);
        mtx34_trans_get(&mtx, &v);
        subVec3(&d, pos, &v);
        copyVec3(&work->pos_0x30, &d);
        mtx34_inverse(&mtx, &mtx);
        mulVecMat(&work->pos_0x30, &mtx);
        return;
    }
    case 3:
        if (get_option_21() != 0) {
            work->option_0x46 = 1;
        }
        work->field_0x44 = 2;
        work->field_0x40 = 2;
        break;
    }
}

/* 0x80107A18 (0x19C): the model allocator of the eft013 family: pools a 0x3C-byte work block with one model,
 * white colours and a uniform scale, a random rotation, and installs the eft013 release/dispatch pair. */
extern "C" void* fn_80107A18(void* self, u8 a, u32 b, f32 scale, u8 c) {
    if (c != get_now_areano()) {
        return NULL;
    }
    _EFT013* e = (_EFT013*)eft_res_slot_get(0x3C);
    if (e == NULL) {
        return NULL;
    }
    _EFT013_WORK_M* work = (_EFT013_WORK_M*)e->work_0x38;
    if ((u32)(a - 26) <= 1) {
        work->count = 0;
    } else {
        work->count = 1;
    }
    e->type_0x02 = a;
    e->release_0x40 = fn_80107CA0;
    e->dispatch_0x34 = fn_80107DDC;
    memset(work->handles, 0, 8);
    work->model = eft_res_model_get();
    if (work->model == NULL) {
        eft_res_slot_release(e);
        return NULL;
    }
    work->field_0x24 = b;
    work->color0.r = work->color0.g = work->color0.b = work->color0.a = 0xFF;
    work->color1.r = work->color1.g = work->color1.b = work->color1.a = 0xFF;
    setVector3(&work->scale_0x10, scale, scale, scale);
    work->scale_0x1C = scale;
    work->scale_0x20 = scale;
    e->source_0x30 = (_ENEMY_WORK*)self;
    e->area_0x44 = c;
    e->field_0x03 = 13;
    e->rot_0x24.x = (u16)ran_suu(0);
    e->rot_0x24.y = (u16)ran_suu(0);
    e->rot_0x24.z = (u16)ran_suu(0);
    e->field_0x10 = 0;
    e->timer_0x0C = 0;
    eft_state_flags_set((_EFT*)e, 0, 0);
    return e;
}

/* 0x8010AE0C (0x144): state-0 setup of the effect pool: creates one effect per slot (type 1 from the id table,
 * its first effect owned by the object; type 5 the fixed id 366 and a one-frame timer). */
extern "C" void fn_8010AE0C(_EFT013* self) {
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    s32 i;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    _EFT014_POOL_B* work = (_EFT014_POOL_B*)self->work_0x38;
    self->state_0x05++;
    switch (self->type_0x02) {
    case 1:
        for (i = 0; i < work->count; i++) {
            work->slots[i] = res_eft_create(eft014_effect_ids[i], 38, 0);
            if (work->slots[i] == NULL) {
                fn_8010BBDC(self);
                return;
            }
            if (i == 0) {
                work->slots[i]->owner_0x44 = self;
            }
        }
        break;
    case 5:
        for (i = 0; i < work->count; i++) {
            work->slots[i] = res_eft_create(366, 38, 0);
            if (work->slots[i] == NULL) {
                fn_8010BBDC(self);
                return;
            }
        }
        self->timer_0x0C = 1;
        break;
    default:
        fn_8010BBDC(self);
        return;
    }
    self->flag_0x01 = 1;
    fn_8010B154(self);
}

/* 0x8010B008 (0x14C): state-0 setup of the rotated effect pool: creates the type's effect per slot (type 4 adds
 * the id 319 one), then roots every effect at the object's rotation and position. */
extern "C" void fn_8010B008(_EFT013* self) {
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    s32 i;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    _EFT014_POOL_B* work = (_EFT014_POOL_B*)self->work_0x38;
    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        work->slots[i] = res_eft_create(eft014_type_effect_ids[self->type_0x02 - 3], 38, 0);
        if (work->slots[i] == NULL) {
            fn_8010BBDC(self);
            return;
        }
    }
    switch (self->type_0x02) {
    case 4:
        work->slots[i] = res_eft_create(319, 38, 0);
        if (work->slots[i] == NULL) {
            fn_8010BBDC(self);
            return;
        }
        work->count++;
        break;
    }
    cpSetRotMatrix(&self->rot_0x24, &mtx);
    mtx34_set_trans(&mtx, &self->pos_0x18);
    for (i = 0; i < work->count; i++) {
        work->slots[i]->SetRootMtx(mtx);
    }
    self->flag_0x01 = 1;
    fn_8010B154(self);
}

/* 0x8010BA48 (0x184): per-frame step of the enemy-joint trail: ends when the enemy dies or the timer runs out,
 * otherwise follows the enemy and moves each effect to a random joint of its slot's list. */
extern "C" void fn_8010BA48(_EFT013* self) {
    s32 i;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;
    nw4r::math::MTX34 mtx;
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    VEC3_ctor(&c);
    MTX34_ctor(&mtx);
    _ENEMY_WORK* src = self->source_0x30;
    _EFT014_POOL_B* work = (_EFT014_POOL_B*)self->work_0x38;
    if (em_work_die_ck(src) != 0) {
        self->state_0x05++;
        self->field_0x06 = 0;
        self->field_0x10 = 20;
    }
    if (--self->timer_0x0C < 0) {
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }
    self->area_0x44 = src->act_id;
    copyVec3(&self->pos_0x18, &src->pos);
    for (i = 0; i < work->count; i++) {
        s32* joints = eft014_joint_lists[i];
        u8 pick = (u16)ran_suu(0) % eft014_joint_counts[i];
        get_joint_wmat_em(src, joints[pick], &mtx);
        work->slots[i]->SetRootMtx(mtx);
        if (effect_move(work->slots[i]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
        }
        eft_res_models_spawn((_EFT*)self, (void**)&work->slots[i], 1, 1, NULL);
    }
}

/* 0x8010BBE0 (0x15C): the shell setter: pools a two-slot trail object for the shell in its area, a model per
 * slot at `pos` moving with `vel`, and installs the eft015 dispatch/release pair. */
void eft015_set(_SHELL_W* shell, u8 type, nw4r::math::VEC3* pos, nw4r::math::VEC3* vel, _CP_VECTOR* rot,
                u8 areano) {
    s32 i;
    if (areano != get_now_areano()) {
        return;
    }
    _EFT013* e = (_EFT013*)eft_res_slot_get(0x9C);
    if (e == NULL) {
        return;
    }
    e->type_0x02 = type;
    e->dispatch_0x34 = fn_8010BDA8;
    e->release_0x40 = fn_8010BD3C;
    _EFT014_POOL_C* work = (_EFT014_POOL_C*)e->work_0x38;
    work->count = 2;
    for (i = 0; i < work->count; i++) {
        work->slots[i].model = eft_res_model_get();
        if (work->slots[i].model == NULL) {
            eft_res_slot_release(e);
            return;
        }
        copyVec3(&work->slots[i].vel, vel);
        copyVec3(&work->slots[i].pos, pos);
        work->slots[i].state[0] = 0;
        work->slots[i].state[1] = 0;
        work->slots[i].state[2] = 0;
        work->slots[i].state[3] = 0;
    }
    copyVec3(&e->pos_0x18, pos);
    e->rot_0x24.x = rot->x;
    e->rot_0x24.y = rot->y;
    e->rot_0x24.z = rot->z;
    e->area_0x44 = areano;
    e->field_0x03 = 15;
    e->flag_0x01 = 1;
    e->source_0x30 = (_ENEMY_WORK*)shell;
    eft_state_flags_set((_EFT*)e, 0, 4);
}
