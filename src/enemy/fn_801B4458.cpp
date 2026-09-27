/* enemy/fn_801B4458.cpp - the enemy seat/effect-action band, 0x801B4458..0x801B7020 (48 functions).
 *
 * The unit owns `.text` 0x801B4458..0x801B7020, the extab run 0x8000F6B4..0x8000F7EC (39 records),
 * the extabindex run 0x8002B0F8..0x8002B2CC (39 records) and one `.ctors` word at 0x8056F348 (the
 * static initializer `fn_801B6FB0`).
 *
 * MODULE AND NAME (brief section 2, evidence order).
 *   * Option 1 (a `__FILE__` string) fails: the range references no file-name string at all (its
 *     only `.rodata` reference is the numeric table `lbl_80570260`, and the `.sdata2` references are
 *     the float pool in `fn_801B4458.h`'s comment).
 *   * Option 2 (a real runtime-dump name) fails: `python tools/symbols/dumpmap.py lookup` answers
 *     `zz_<addr>_` for every one of the 48 addresses, which is not evidence.
 *   * Option 3 (what the code does plus the neighbours' scheme) fixes the module: the bracketing
 *     registered units are `enemy/fn_801B0010.cpp` below and `enemy/fn_801B7020.cpp` above, and every
 *     callee out of the range is enemy-band (`_ENEMY_WORK`, `em_act_ck`, `em_mot_set`,
 *     `get_move_work_adrs(3)`).
 *   * Option 4 therefore decides the FILE NAME: the neighbours' scheme is `fn_XXXXXXXX.<ext>`
 *     (`src/enemy/` is 27 such files plus `enemy_control.cpp`), and no evidence names the original
 *     source file, so the file keeps the map's own stem.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup` on every address of the 48-function inventory: every one
 * answers `zz_<addr>_`, and `config/RMHE08/symbols.txt` carries nothing but the bare `fn_XXXXXXXX`
 * entries).
 *
 * Language C++.  Every callee the range reaches through a mangling is declared at its real signature
 * (`em_act_ck`, `em_die_ck`, `em_get_mot_no`, `em_frame_check`, `em_after_frame_check`,
 * `get_move_work_adrs`, `get_move_work_max`, `ran_suu`, `get_em_chg_scale`, `get_joint_wpos_em`,
 * `setVector3`, `vec_to_mh_vec3`) - rule 9 never spells the mangling.  The range's own flat symbols
 * stay C-linkage through the `extern "C"` block in `include/enemy/fn_801B4458.h`.
 *
 * Object: `_ENEMY_WORK`, included from `include/enemy/ENEMY_WORK.h` (the one shared home; the fields
 * this range names were added there - +0x00F, +0x1CC, the +0x328 seat view, +0x440, +0x833 and
 * +0x888, and `EmAreaWork`'s +0x008/+0x3C for the area records `get_move_work_adrs(2)` hands back).
 *
 * The seam is unproven (docs/plan.md 8.3).  It is exactly the unclaimed gap between the two
 * registered units, so both edges are their `.text` edges.
 *
 * Flags: no deviation - the `enemy` lib's `cflags_main` measured every body below.  No `#pragma`.
 *
 * State of the reconstruction.  All 48 bodies are written and every one clears the 80 % bar.
 *
 * STATUS (measured with `tools/units/recompile.py enemy/fn_801B4458 --measure <symbol>`, the official
 * report metric against MAIN's retired single-symbol split objects `auto_fn_*_text.o`).  Mean
 * 95.0569 %; 19 bodies are byte-identical: fn_801B4E38, fn_801B4E3C, fn_801B4EA8, fn_801B4F08,
 * fn_801B4F84, fn_801B5000, fn_801B5108, fn_801B5184, fn_801B53E8, fn_801B5464, fn_801B54E0,
 * fn_801B555C, fn_801B5AE4, fn_801B5B60, fn_801B5E20, fn_801B60D4, fn_801B64E8, fn_801B64FC,
 * fn_801B701C.  Object sections: `.text` 0x2C28 (target 0x2BC8), extab 0x138 (target 0x138),
 * extabindex 0x1D4 (target 0x1D4).
 *
 * Residuals, by measurement (all near-misses are codegen shapes, not comprehension):
 *   * RECORD-WALKER COLOURING - fn_801B4694 82.43 and fn_801B45B0 82.81: the `list` base reload and
 *     the `index << 5` offset land in different registers from the target's own split.
 *   * `clrlwi`/`rlwinm` + `cmpwi` FUSION - fn_801B4C54 96.56 and fn_801B6C38 94.47: -O3's peephole
 *     folds the byte test into the record form where the target keeps the unfused pair.  A
 *     `#pragma peephole off` was probed and rejected: it fixes those two to 100 but regresses the
 *     bodies that keep the fused/eliminated form (fn_801B4F08 100 -> 96.77, fn_801B5AE4 100 ->
 *     96.77, fn_801B5030 97.87 -> 92.31, fn_801B6010 97.65 -> 93.57).
 *   * `.ctors` REGISTER ORDER - fn_801B6FB0 84.93: the two `setVec3` calls colour their float
 *     registers differently.
 *   * MOTION SELECT - fn_801B63E8 86.33: the `field_0x00A & 1` motion select materialises in a
 *     different register, and fn_801B65B8 87.24 the same in the effect-id select.
 *   * `fn_801B4458` 91.00 / fn_801B47A4 88.79: the seat picker and the per-tick update keep a
 *     close instruction multiset but pair a few blocks differently.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801B0010.h"
#include "enemy/fn_801B4458.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* Flags: no deviation - the `enemy` lib's `cflags_main` (peephole on) measured every body below.
 * The band's unfused `clrlwi`/`rlwinm` + `cmpwi` near-misses (`fn_801B4C54`, `fn_801B6C38`) are a
 * source-shape residual, recorded with the unit's status; a `#pragma peephole off` was probed and
 * rejected: it fixes those two but regresses the eleven bodies that keep the fused/eliminated form
 * (`fn_801B4F08`, `fn_801B5030`, `fn_801B5AE4`, `fn_801B6010`, ...). */

/* -------------------------------------------------------------------------------------------------
 * the band's callees, declared with the signatures their call sites set (rule 2: the plain
 * prototypes are not `extern` declarations of another unit's symbol; the owners' headers carry the
 * canonical spellings and are the follow-up).
 * ------------------------------------------------------------------------------------------------- */

/* enemy/fn_8012EC74.cpp (0x8012EC74..0x80137604) - the action/motion arming helpers. */
void fn_8013072C(struct _ENEMY_WORK* self, u32 mode, u32 value);

/* enemy/fn_801251D0.cpp (0x801251D0..0x8012BA00) - the program/entry helpers. */
void fn_801251D0(void* tbl, u32 a, u32 b);
void fn_801251D8(void* tbl, u32 a, u32 b);
void* fn_80125F54(void* out);
u32 fn_801421E4(u32 id, void* out);
u8 fn_802B0668(u8 map);
u32 fn_80126324(struct _ENEMY_WORK* self, u32 a, u32 b, f32 c);
u32 fn_80129668(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 fn_8012D0B4(struct _ENEMY_WORK* self, void* area);
u32 fn_8012D1A8(u8 kind);

/* enemy/fn_80137604.cpp (the 0x8013xxxx motion setters). */
void fn_8013581C(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b, u32 c, u16 d,
                 f32 e);
void fn_80135764(nw4r::math::VEC3* out, nw4r::math::VEC3* a, u32 c, u16 d, f32 e);
void fn_80133E3C(struct _ENEMY_WORK* self, s32 a, f32 b, f32 c);
void fn_80134004(struct _ENEMY_WORK* self, u32 a, f32 b);
u32 fn_80134114(struct _ENEMY_WORK* self, s32 a, s32 b);
u32 fn_80134964(struct _ENEMY_WORK* self, void* tbl, s32 a, s32 b, s32 c);
u32 fn_80134B0C(struct _ENEMY_WORK* self, void* tbl);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void fn_80131DF4(struct _ENEMY_WORK* self);
void em_mot_set(struct _ENEMY_WORK* self, s32 a, s32 b, s32 c);
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
void em_action_finish(struct _ENEMY_WORK* self);
void fn_8012CEB4(struct _ENEMY_WORK* self, s16 timer, u8 index);
u32 fn_80132184(void);

/* the base vector/effect helpers (owned elsewhere; declared, never defined - playbook 29). */
void fn_80051490(void* out, void* in);
void fn_80051378(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void fn_8004FFC8(void* a, void* b, void* c, f32 d);
void fn_800AD9C0(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 scale);
f32 fn_80050EF4(void* a, void* b);
f32 fn_80050F80(void* a, void* b);
void fn_800FA378(void* out);
u32 fn_800CF280(void);
s32 ran_suu(s32 a);
void fn_800E2F40(void* self, s32 a, s32 b, u8 c, s32 d, s32 e, u8 f);
s32 fn_8028F558(void* a, void* b);
s32 fn_802907BC(void* a, void* b);
void fn_8010D2B0(void* pos, u8 area, u8 kind, s32 mode, f32 scale);
void fn_801048B4(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint, f32 scale);
void fn_801049D0(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint, nw4r::math::VEC3* pos,
                 f32 scale);
void fn_800FC0D4(void* dst, void* src);
s32 fn_803B9A40(s32 handle);
void fn_803B9994(s32 handle);
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

/* -------------------------------------------------------------------------------------------------
 * the `.sdata2` float pool and the two data tables the band reads.
 * ------------------------------------------------------------------------------------------------- */

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
extern u8 lbl_806A7AA0[0x18];

/* the 0x805B1Bxx enemy-control program tables `fn_801251D0`/`fn_801251D8` walk. */
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

/* The 0x20-byte ground record `fn_80125F54` prepares and `fn_801421E4` fills lives in
 * `include/enemy/ENEMY_WORK.h` now (rule 1: one definition for both units that own one). */

/* -------------------------------------------------------------------------------------------------
 * the range's functions, in address order
 * ------------------------------------------------------------------------------------------------- */

/* 0x801B4458 (0x158).  Pick the seat record of the `kind` set `fn_801B4398` hands back whose point
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
            fn_80051378(&delta, &a, &b);
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

/* 0x801B45B0 (0xE4).  Whether the seat record is within `radius` of `point`: a two-point seat
 * (code 1) builds the box between its points and tests the point against it, a single-point seat
 * measures straight distance. */
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

/* 0x801B4694 (0x110).  Move the work record's aim to seat `index`: the two-point seats install a
 * segment (through `fn_8013581C`), a single-point seat the point itself (`fn_80135764`), each with
 * the seat's own float and a random u16 phase. */
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
            fn_80135764(&self->aim, &a, 0, phase, list[i].value_0x1C);
        }
    }
}

/* 0x801B47A4 (0x4B0).  The per-tick seat update: it walks the (3) work records and the (2) area
 * records for seats near the enemy, marks which of the up-to-10 seat records are occupied, re-picks
 * the current seat (the seat selection itself is `fn_801B4458`), and arms the seat action through
 * `fn_8013072C`. */
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
                if (area->active != 0 && fn_8012D1A8(area->field_0x008) == 0 &&
                    fn_8012D0B4(self, area) != 0) {
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
            fn_80050F80(&self->pos, &self->aim) < lbl_80798C7C) {
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

    fn_80125F54(&rec);
    if (fn_800CF280() == 0) {
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

/* 0x801B4E3C (0x6C).  Release the latched effect handle +0x888 when neither the 0xA/0x7D nor the
 * 0xB/0x25 action is live. */
extern "C" void fn_801B4E3C(_ENEMY_WORK* self) {
    if (em_act_ck(self, 0x0A, 0x7D) == 0 && em_act_ck(self, 0x0B, 0x25) == 0 &&
        self->field_0x888 != -1) {
        fn_803B9994(self->field_0x888);
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

/* 0x801B5000 (0x30).  Dispatch the 0x801B4F08/0x801B4F84 pair on the +0x1E6 sub-state. */
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

/* 0x801B555C (0x4C).  Dispatch the 0x801B5030..0x801B54E0 set on the +0x1E6 sub-state. */
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
        fn_80134004(self, 0, lbl_80798C8C);
        return;
    case 1:
        if (fn_80134114(self, 0, 0xA0) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801B563C (0x138).  Seat sub-state 12 (two variants): motion 9/4 with the kind-selected timer,
 * then either finish or continue with 0x25/4. */
extern "C" void fn_801B563C(_ENEMY_WORK* self, u8 kind) {
    u8 state = self->state;
    u32 motion = 0x0D;
    f32 timer = lbl_80798C8C;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 4, 0);
        if (kind == 1) {
            timer = lbl_80798C90;
        }
        fn_80134004(self, 0, timer);
        return;
    case 1:
        if (fn_80134114(self, 0, 0xA0) == 1) {
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
    u32 motion = 0x0D;
    f32 timer = lbl_80798C8C;
    u32 done;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x0A, 4, 0);
        if (kind == 1) {
            timer = lbl_80798C90;
        }
        fn_80134004(self, 0x10, timer);
        if (flag == 1) {
            self->state_0x006 = (u8)(((self->bits_0x1EC & 1) * 2) + 2);
        }
        return;
    case 1:
        done = fn_80134114(self, 0, 0x100);
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
            fn_80133E3C(self, 0x7800, lbl_80798C74, lbl_80798C94);
            return;
        }
        fn_80133E3C(self, -0x7800, lbl_80798C74, lbl_80798C94);
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
        fn_80134964(self, lbl_80570260, 0, 1, 0);
        return;
    case 1:
        if (fn_80134B0C(self, lbl_80570260) == 1) {
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
        fn_80133E3C(self, angle, lbl_80798C74, lbl_80798C94);
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

/* 0x801B5E20 (0xB4).  Dispatch the 0x801B55A8..0x801B5D24 set on the +0x1E6 sub-state. */
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
        fn_80129668(self, 0, 1);
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
            fn_80129668(self, 0, 2);
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
            fn_80129668(self, 0, 2);
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

/* 0x801B60D4 (0x38).  Dispatch the 0x801B5ED4/0x801B6010 trio on the +0x1E6 sub-state. */
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
 * `fn_801251D0` (table, flag, id) triple; the default finishes the action. */
extern "C" void fn_801B610C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x01:
        fn_801251D0(lbl_805B1BB0, 0, 1);
        return;
    case 0x02:
        fn_801251D0(lbl_805B1BD8, 0, 2);
        return;
    case 0x17:
        fn_801251D0(lbl_805B1C30, 1, 0x17);
        return;
    case 0x05:
        fn_801251D0(lbl_805B1C30, 1, 5);
        return;
    case 0x23:
        fn_801251D0(lbl_805B1C30, 1, 0x23);
        return;
    case 0x7A:
        fn_801251D0(lbl_805B1C78, 0, 0x7A);
        return;
    case 0x7B:
        fn_801251D0(lbl_805B1CA0, 0, 0x7B);
        return;
    case 0x7C:
        fn_801251D0(lbl_805B1CC8, 0, 0x7C);
        return;
    case 0x8D:
        fn_801251D0(lbl_805B1CF0, 0, 0x8D);
        return;
    case 0x7D:
        fn_801251D0(lbl_805B1D30, 0, 0x7D);
        return;
    case 0x8E:
        fn_801251D0(lbl_805B1DA0, 0, 0x8E);
        return;
    case 0x84:
        fn_801251D0(lbl_805B1C30, 1, 0x84);
        return;
    case 0x9F:
        fn_801251D0(lbl_805B1DC8, 0, 0x9F);
        return;
    case 0xA0:
        fn_801251D0(lbl_805B1DC8, 0, 0xA0);
        return;
    case 0xA8:
        fn_801251D0(lbl_805B1E08, 0, 0xA8);
        return;
    case 0xA9:
        fn_801251D0(lbl_805B1C30, 1, 0xA9);
        return;
    default:
        em_action_finish(self);
        return;
    }
}

/* 0x801B6308 (0xE0).  The `fn_801251D8` sibling of 0x801B610C (the table is the second half of the
 * set; the default arms id 0 on the first table). */
extern "C" void fn_801B6308(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x00:
        fn_801251D8(lbl_805B1E50, 1, 0);
        return;
    case 0x05:
        fn_801251D8(lbl_805B1E50, 1, 5);
        return;
    case 0x1D:
        fn_801251D8(lbl_805B1E90, 0, 0x1D);
        return;
    case 0x1E:
        fn_801251D8(lbl_805B1E50, 1, 0x1E);
        return;
    case 0x38:
        fn_801251D8(lbl_805B1F18, 0, 0x38);
        return;
    case 0x25:
        fn_801251D8(lbl_805B1FF0, 0, 0x25);
        return;
    case 0x2A:
        fn_801251D8(lbl_805B20E8, 1, 0x2A);
        return;
    default:
        fn_801251D8(lbl_805B1E50, 0, 0);
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
        fn_80134004(self, 0x10, lbl_80798C90);
        if (self->field_0x43B != 3 && self->field_0x43B != 2) {
            fn_801B4348(self);
            return;
        }
        return;
    case 1:
        if (fn_80134114(self, 0, 0x100) == 1) {
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

/* 0x801B65B8 (0x154).  Spawn the seat's effect at the motion's joint (kind 1) or directly (kind 0),
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
        fn_801048B4(self, joint, id, arg4, scale);
        return;
    case 1:
        get_joint_wpos_em(self, joint, &pos);
        pos.y = self->field_0x20C;
        fn_8010D2B0(&pos, self->area_no, id, arg4, scale * get_em_chg_scale(self));
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
                fn_801048B4(self, 0x1A, 0x33, 0, lbl_80798CA4);
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

/* 0x801B6B94 (0xA4).  Seed the two alpha/colour passes of the embedded MHchar base from the +0x1D4
 * ratio. */
extern "C" void fn_801B6B94(_ENEMY_WORK* self) {
    f32 ratio;
    s32 a;
    s32 c;

    ratio = self->field_0x1D4;
    a = (s32)(lbl_80798CD0 * ratio);
    c = (s32)(lbl_80798CD4 * ratio);
    fn_800E2F40(self->char_0x024, 0, 6, (u8)a, 0, 3, (u8)c);
    fn_800E2F40(self->char_0x024, 1, 6, (u8)a, 0, 3, (u8)c);
}

/* 0x801B6C38 (0x4C).  Whether the latch kind is clear and the +0x888 handle can be re-armed (the
 * Haskell side reports none live). */
extern "C" s32 fn_801B6C38(_ENEMY_WORK* self, u8 flag) {
    if (flag == 0 && self->field_0x888 != -1 && fn_803B9A40(self->field_0x888) <= 0) {
        return 1;
    }
    return 0;
}

/* 0x801B6C84 (0x270).  The enemy-control seat picker: map the map id (`fn_802B0668`) and area to a
 * motion pair through `fn_80126324`; when no map/area matches, report the unmatched state and copy
 * the control record's position/rotation onto the work. */
extern "C" void fn_801B6C84(_ENEMY_WORK* self, s8* out_state, s8* out_flag) {
    EmGroundRec rec;
    u32 unmatched = 0;

    fn_80125F54(&rec);
    switch (fn_802B0668(self->field_0x1E0)) {
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
            fn_800FC0D4(&self->field_0x1BC, &rec.field_0x14);
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
    fn_80051490(lbl_806A7AA0, &tmp);
    setVec3(&tmp, lbl_80798C74, lbl_80798CF0, lbl_80798CF4);
    fn_80051490(lbl_806A7AA0 + 0x0C, &tmp);
}

/* 0x801B701C (0x4).  The tail-call slot the neighbouring unit's table keeps. */
extern "C" u32 fn_801B701C(_ENEMY_WORK* self) {
    return fn_80132184();
}
