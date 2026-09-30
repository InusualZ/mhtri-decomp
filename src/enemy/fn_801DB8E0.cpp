/* enemy/fn_801DB8E0.cpp - the `em` enemy effect/part band, `.text` 0x801DB8E0..0x801E0ADC
 * (28 functions, 20988 bytes).
 *
 * what it is.  One enemy's effect and part-selection band, the same family as the registered units
 * `enemy/fn_801D428C.cpp` (the band below, 0x801D428C..0x801D80EC) and `enemy/fn_801993E0.cpp`: every
 * function takes the shared `_ENEMY_WORK`, reads its part table (`+0x38`) and drives the per-effect
 * clusters through `eft009_set_pos`/`fn_801048B4`/`fn_8010D2B0`/`fn_80106694`, the `MHchar` at
 * `+0x024`, and the res-effect API (`res_eft_create`, `res_eft_model_create_light`).  The range is one
 * band of `fn_801DBE0C`, the 0x34EC motion dispatcher, over the whole file.
 *
 * module and name (brief section 2, evidence order).
 *   1. No `__FILE__` string covers the range: the range references no `.data`/`.rodata` string at all
 *      (its only absolute loads are `system_w`, the two `.data` jumptables `jumptable_805B71E0`/
 *      `jumptable_805B728C`, the effect-selector table `lbl_805B7608` and the two `fn_8012A014`
 *      arguments `lbl_805B6A44`/`lbl_805B6A50`).
 *   2. `dumpmap.py lookup 0x801DB8E0` answers `zz_01db8e0_` (a placeholder is not evidence).
 *   3. The code is enemy-band: every function takes the `_ENEMY_WORK` (`em_get_mot_no`,
 *      `em_after_frame_check`, `get_joint_wpos_em`, `em_parts_damage_level_get`), the bracket below is
 *      `enemy/fn_801D428C.cpp` and the module's naming scheme is the map's own `fn_XXXXXXXX` stem.
 * The file therefore keeps the map stem (brief option 4); no name was invented.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address of the range answers `zz_XXXXXXXX_` in the
 * shared runtime dump and carries a bare `fn_XXXXXXXX = .text:0x...` entry in
 * config/RMHE08/symbols.txt; no `__FILE__` string is reachable from the range).
 *
 * language.  C++: the range's callees are C++ manglings (`setVector3__FPQ34nw4r4math4VEC3fff`,
 * `calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3`, `getTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor`,
 * `eft009_set_pos__FUcPQ34nw4r4math4VEC3P10_CP_VECTORfUl`, `__nw__FUl`/`__dl__FPv`), the `MHchar` at
 * `_ENEMY_WORK::char_0x024` is called as a class, and the range installs a class descriptor
 * (`lbl_805B76B8`).  Rule 9: those are declared at C++ scope with the signature their mangling encodes
 * and called through it; every `fn_*` definition stays `extern "C"`.
 *
 * seam.  Pinned by the discovery pool evidence (`.sdata2` run jump `lbl_8079979C -> lbl_807997A0`) at
 * the left edge; the right edge at 0x801E0ADC is exact (`fn_801E0AC0` ends there and the discovered run
 * below starts there).  Unproven as a TU, as the brief says; the extent settles as the bodies match.
 *
 * residuals (not written this pass - each is a jump-table/interpreter state machine over the same
 * `_ENEMY_WORK` and needs the +0x38 part-manager record reconstructed first):
 *   fn_801DBE0C (0x34EC, 202-case motion dispatcher over `jumptable_805B728C`),
 *   fn_801DFEEC (0x94), fn_801DFF80 (0xD8),
 *   fn_801E0240 (0x58), fn_801E0298 (0x90), fn_801E0328 (0xD8), fn_801E0400 (0xDC),
 *   fn_801E0574 (0x3C4), fn_801E0938 (0x3C), fn_801E0974 (0x110), fn_801E0A84 (0x3C).
 * The measurement for each is in the worker's outbox.
 */

#pragma peephole off

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "sound/mhchar.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "stage/stg_w.h"

/* ----------------------------------------------------------------------------------------------------
 * the pool the range reads (owned elsewhere; declared, never defined - playbook 29)
 * -------------------------------------------------------------------------------------------------- */

extern f32 lbl_807994F8;
extern f32 lbl_807994FC;
extern f32 lbl_80799518;
extern f64 lbl_807995A0;
extern f32 lbl_80799614;
extern f32 lbl_8079964C;
extern f32 lbl_80799650;
extern f32 lbl_80799654;
extern f32 lbl_80799668;
extern f32 lbl_8079975C;
extern f32 lbl_80799760;
extern f32 lbl_80799764;
extern f32 lbl_80799768;
extern f32 lbl_8079976C;
extern f32 lbl_80799770;
extern f32 lbl_80799774;
extern f32 lbl_80799778;
extern f32 lbl_8079977C;
extern f32 lbl_80799780;

/* this range's own `.data` tables (no `.data` range is registered for this unit yet, so they stay the
 * shared pool's bytes; only the ones the code loads explicitly are declared). */
extern u8 lbl_805B6A44[];
extern u8 lbl_805B6A50[];

/* ----------------------------------------------------------------------------------------------------
 * the C++-mangled callees (rule 9: declared with the signature the mangling encodes, called through it)
 * -------------------------------------------------------------------------------------------------- */

void rotVecY(nw4r::math::VEC3* v, u32 angle);                    /* rotVecY__FPQ34nw4r4math4VEC3Ul */
u32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                                /* calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                            /* calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void rotMatrixX(u32 angle, nw4r::math::MTX34* m);              /* rotMatrixX__FUlPQ34nw4r4math5MTX34 */
void rotMatrixZ(u32 angle, nw4r::math::MTX34* m);              /* rotMatrixZ__FUlPQ34nw4r4math5MTX34 */
void eft009_set_pos(u8 kind, nw4r::math::VEC3* pos, void* rot, f32 scale, u32 area);
                                           /* eft009_set_pos__FUcPQ34nw4r4math4VEC3P10_CP_VECTORfUl */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
                                          /* em_parts_damage_level_get__FP11_ENEMY_WORKUc */
u16 em_get_mot_no(struct _ENEMY_WORK* self);                       /* em_get_mot_no__FP11_ENEMY_WORK */
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                                  /* em_after_frame_check__FP11_ENEMY_WORKUsff */
f32 get_em_chg_scale(struct _ENEMY_WORK* self);                  /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                        /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */

/* the runtime's allocator pair; spelling the manglings `__nw__FUl`/`__dl__FPv` as identifiers would be
 * rule 9's violation, so the C++ definitions the compiler mangles to them are declared and called. */
void* operator new(unsigned long size);
void operator delete(void* ptr) throw();

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------------------------------
 * the enemy-band callees
 *
 * The signatures are the call sites' (the argument counts are the ones the callers set in r4..r8 and
 * f1..f3); where a callee's body disagrees with its owner header, the body won.
 * -------------------------------------------------------------------------------------------------- */

/* enemy/fn_8012E968.cpp (0x8012E968..0x8012EC74) - both bodies read the work record. */
u32 fn_8012EC60(struct _ENEMY_WORK* self);
u32 fn_8012EC3C(struct _ENEMY_WORK* self);

/* enemy/fn_801251D0.cpp (0x801251D0..0x8012BA00). */
void fn_80126278(struct _ENEMY_WORK* self, u16 id, nw4r::math::VEC3* out);
u32 fn_80129D3C(struct _ENEMY_WORK* self);
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
u32 fn_8012A014(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);
s32 fn_8012A204(struct _ENEMY_WORK* self);

/* enemy/fn_8012BDF4.cpp (0x8012BDF4..0x8012E968). */
u32 fn_8012E5A8(struct _ENEMY_WORK* self);

/* enemy/fn_8012EC74.cpp (0x8012EC74..0x80137604). */
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
u32 fn_8013023C(struct _ENEMY_WORK* self);
u8* fn_801377D0(u8 index);
void fn_80136B50(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 fn_80135748(struct _ENEMY_WORK* self, u32 a);
void fn_8013A654(struct _ENEMY_WORK* self, u32 a);

/* enemy/fn_80138074.c (0x80138074..0x8013ACC4). */
void fn_8013918C(void* helper, s16 flag);

/* the object `fn_801E0538` dispatches on (the part-manager record; this unit names only the step byte
 * it reads).
 * size: 0x6 */
struct EmEftPartsMan {
    /* +0x00 */ u8 unused_0x00[0x05];
    /* +0x05 */ u8 state; /* the step index `fn_801E0538` switches on */
};

/* this range's own part-manager step functions (declared so the dispatcher compiles; the bodies are
 * the band's residuals, see the file header). */
void fn_801E0574(struct EmEftPartsMan* self);
void fn_801E0D38(struct EmEftPartsMan* self);
void fn_801E1A2C(struct EmEftPartsMan* self);
void fn_801E1A3C(struct EmEftPartsMan* self);

/* ef module - the effect clusters. */
void fn_801048B4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_8010562C(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos, f32 c);
void fn_80106694(struct _ENEMY_WORK* self, void* pos, u8 a, f32 b);
void fn_8010D2B0(void* pos, u8 a, u8 b, s32 c, f32 d);

/* the base runtime helpers the range reaches. */
f32 fn_80050EF4(void* a, void* b);
void fn_80051378(void* out, void* a, void* b);
void fn_8005D0CC(void* out, void* src);
void fn_8005D1AC(void* out, u32 a);
void fn_8006FDCC(void* a);
void fn_800810DC(void* self, u32 a);

/* `stage_map_kind_get`, `get_now_mapno` and `get_now_areano` come from `include/unsplit/unknown.h`. */

/* ----------------------------------------------------------------------------------------------------
 * the definitions (C linkage: they keep the map's own `fn_XXXXXXXX` names)
 * -------------------------------------------------------------------------------------------------- */

/* 0x801DB8E0 (0x98) - arm the +0x1BC/-0x20C effect spawn once the work record answers the map query
 * and `system_w`'s counter has run a multiple of 24 frames. */
void fn_801DB8E0(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    if (fn_8012EC60(self) == 1) {
        if (system_w.field_0x0c % 0x18 == 0) {
            setVector3(&pos, lbl_807994FC, lbl_8079964C, lbl_80799650);
            fn_8010562C(self, 0x13, 0x16, &pos, lbl_807994F8);
        }
    }
}

/* 0x801DB978 (0x494) - the effect-spawn dispatcher: one case per effect id, each either calling the
 * owner's `eft009_set_pos` directly (when the joint is 0xFF) or handing the joint to
 * `fn_801048B4`/`fn_8010D2B0`/`fn_80106694`. */
void fn_801DB978(struct _ENEMY_WORK* self, u8 mode, u8 kind, u32 joint, u32 id, f32 scale) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    if (mode == 0) {
        if ((self->field_0x228 & 0x6) != 0) {
            switch (kind) {
            case 0:
                kind = 0xd;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xf, &pos, (void*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    fn_801048B4(self, joint, 0xf, id, scale);
                }
                break;
            case 2:
                kind = 0xc;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xe, &pos, (void*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    fn_801048B4(self, joint, 0xe, id, scale);
                }
                break;
            case 4:
                kind = 0x13;
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xe, &pos, (void*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    fn_801048B4(self, joint, 0xe, id, scale);
                }
                break;
            case 1:
            case 3:
            case 6:
            case 7:
            case 9:
            case 0xa:
            case 0xb:
            case 0x24:
            case 0x25:
            case 0x29:
            case 0x2a:
                return;
            default:
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                }
                break;
            }
        } else {
            if ((u32)kind - 0xc > 0xd && kind != 0x26) {
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x20C;
                    pos.z = self->pos.z;
                }
            }
        }
        if (joint == 0xff) {
            eft009_set_pos(kind, &pos, (void*)&self->field_0x1BC, scale, self->area_no);
        } else {
            fn_801048B4(self, joint, kind, id, scale);
        }
    } else if (mode == 1) {
        if ((self->field_0x228 & 0x6) != 0) {
            switch (kind) {
            case 0:
            case 6: {
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x11, &pos, (void*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    fn_801048B4(self, joint, 0x11, id, scale);
                }
                break;
            }
            case 1: {
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x10, &pos, (void*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    fn_801048B4(self, joint, 0x10, id, scale);
                }
                break;
            }
            case 5: {
                if (joint == 0xff) {
                    pos.x = self->pos.x;
                    pos.y = self->pos.y + lbl_80799654 + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0x14, &pos, (void*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    fn_801048B4(self, joint, 0x14, id, scale);
                }
                break;
            }
            default: /* 3, 4, ...: nothing */
                break;
            }
        } else {
            if (joint == 0xff) {
                copyVec3(&pos, &self->pos);
            } else {
                get_joint_wpos_em(self, joint, &pos);
            }
            pos.y = self->field_0x20C;
            scale = scale * get_em_chg_scale(self);
            fn_8010D2B0(&pos, self->area_no, kind, id, scale);
        }
    } else if (mode == 2) {
        if (joint == 0xff) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &pos);
        }
        pos.y = self->field_0x20C;
        scale = scale * get_em_chg_scale(self);
        fn_80106694(self, &pos, kind, scale);
    }
}

/* 0x801DF2F8 (0x248) - fade the four K-colour alphas of the embedded `MHchar`: the value follows
 * `field_0x340` (stepped by `lbl_8079975C` toward `lbl_80799760`/0), or its scaled and clamped form
 * when the work record answers part kind 4 with area 4/6. */
void fn_801DF2F8(struct _ENEMY_WORK* self) {
    _GXColor color;
    u32 strength;
    u32 scaled;
    if (fn_8012EC60(self) == 1) {
        self->action_0x328.field_0x340 += lbl_8079975C;
        if (self->action_0x328.field_0x340 > lbl_80799760) {
            self->action_0x328.field_0x340 = lbl_80799760;
        }
    } else {
        self->action_0x328.field_0x340 -= lbl_8079975C;
        if (self->action_0x328.field_0x340 < lbl_807994FC) {
            self->action_0x328.field_0x340 = lbl_807994FC;
        }
    }
    scaled = 0;
    strength = stage_map_kind_get(self->field_0x1E0);
    if (strength == 4 && (self->area_no == 4 || self->area_no == 6)) {
        scaled = 1;
    }
    if (scaled == 1) {
        f32 v = lbl_80799518 * self->action_0x328.field_0x340 + lbl_80799668;
        if (v > lbl_80799760) {
            v = lbl_80799760;
        }
        strength = (u32)(s32)v;
    } else {
        strength = (u32)(s32)self->action_0x328.field_0x340;
    }
    ((MHchar*)self->char_0x024)->getTevKColor(0, GX_KCOLOR3, &color);
    color.a = strength;
    ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR3, color);
    ((MHchar*)self->char_0x024)->getTevKColor(1, GX_KCOLOR3, &color);
    color.a = strength;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR3, color);
    ((MHchar*)self->char_0x024)->getTevKColor(2, GX_KCOLOR3, &color);
    color.a = strength;
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, color);
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
    if (fn_80135748(self, 1) == 1) {
        u32 fade = 0;
        if (strength == 4 && (self->field_0x48E == 4 || self->field_0x48E == 6)) {
            fade = 1;
        }
        if (fade == 1) {
            if ((f32)color.a > lbl_80799764) {
                color.a--;
            }
        } else {
            if (color.a != 0) {
                color.a--;
            }
        }
    } else {
        color.a = strength;
    }
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, color);
}

/* 0x801DF540 (0x28) - "is part `a` the aim target": only true for part 0 and when the action block's
 * +0x345 byte is 1. */
u32 fn_801DF540(struct _ENEMY_WORK* self, u8 a) {
    if (a == 0 && self->action_0x328.field_0x345 == 1) {
        return 1;
    }
    return 0;
}

/* 0x801DF568 (0x2A4) - arm the part's motion (`em_move_mode_set`) and pick the `fn_80126278` effect id
 * from the work record's map kind (`stage_map_kind_get`) and `area_no`. */
void fn_801DF568(struct _ENEMY_WORK* self, u8* outA, u8* outB) {
    u32 kind;
    em_move_mode_set(self, 4);
    *outA = 0xc;
    *outB = 0;
    kind = stage_map_kind_get(self->field_0x1E0);
    switch ((u8)kind) {
    case 1:
        switch (self->area_no) {
        case 4:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        case 5:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 5), &self->pos);
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 4:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 6), &self->pos);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 1:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 1), &self->pos);
            break;
        case 3:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 4), &self->pos);
            break;
        }
        break;
    case 4:
        switch (self->area_no) {
        case 3:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 7), &self->pos);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 5), &self->pos);
            break;
        }
        break;
    case 5:
        switch (self->area_no) {
        case 4:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 1), &self->pos);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->area_no & 0xf) << 8) | 4), &self->pos);
            break;
        }
        break;
    case 8:
        switch (self->area_no) {
        case 1:
            fn_80126278(self, (u16)((self->area_no & 0xf) << 8), &self->pos);
            break;
        }
        break;
    case 9:
        switch (self->area_no) {
        case 0:
            fn_80126278(self, (u16)((self->area_no & 0xf) << 8), &self->pos);
            break;
        }
        break;
    }
}

/* 0x801DF80C (0x4) - empty body (the band's null override). */
void fn_801DF80C(void) {
}

/* 0x801DF810 (0xDC) - "may this part be damaged": the work record's +0x1E2 gate plus the map- and
 * part-level checks for parts 0/4/5. */
u32 fn_801DF810(struct _ENEMY_WORK* self, u8 part) {
    if (self->field_0x1E2 == 0) {
        switch (part) {
        case 0:
            if (fn_8012EC60(self) == 1) {
                return 1;
            }
            break;
        case 4:
            if (fn_8012EC3C(self) == 1) {
                return 1;
            }
            if ((em_parts_damage_level_get(self, 4) & 1) == 0) {
                return 1;
            }
            break;
        case 5:
            if (fn_8012EC3C(self) == 1) {
                return 1;
            }
            if ((em_parts_damage_level_get(self, 5) & 1) == 0) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* 0x801DF8EC (0x31C) - the part-state step: gate on the map kind 1..5, then a `fn_80129DB8` state
 * test, a special-part request through `fn_8012A014` and the "new special part" arming of
 * +0x1FC/+0x1FE/+0x1FF. */
u32 fn_801DF8EC(struct _ENEMY_WORK* self, u16 a) {
    u32 kind;
    u32 found;
    u8 id;
    kind = stage_map_kind_get(self->field_0x1E0);
    if ((u8)kind - 1 > 4) {
        return 0;
    }
    if (fn_80129D3C(self) == 1) {
        return 1;
    }
    found = 0;
    switch ((u8)kind) {
    case 1:
        id = 9;
        break;
    case 2:
        id = 9;
        break;
    case 3:
        id = 0xa;
        break;
    case 4:
        id = 9;
        break;
    case 5:
        id = 4;
        break;
    default:
        id = 0xff;
        break;
    }
    if (id != 0xff) {
        u8 s = fn_80129DB8(self);
        if (s == 1) {
            found = 1;
        } else if (s == 2) {
            return 1;
        }
    }
    if (found == 0) {
        u8 r4;
        u8 r5;
        switch ((u8)kind) {
        case 1:
            r4 = 0x1b;
            r5 = 9;
            break;
        case 2:
            r4 = 0x1b;
            r5 = 6;
            break;
        case 3:
            r4 = 0x1b;
            r5 = 3;
            break;
        case 4:
            r4 = 0x1c;
            r5 = 5;
            break;
        case 5:
            r4 = 0x1b;
            r5 = 3;
            break;
        default:
            r4 = 0;
            r5 = 0xff;
            break;
        }
        if (r4 != 0 || r5 != 0xff) {
            if (fn_8012A014(self, r4, r5, a, lbl_805B6A44, lbl_805B6A50) == 1) {
                return 1;
            }
        }
    }
    if (fn_80129A70(self, a) == 1) {
        return 1;
    }
    if (!(self->value_0x452 < self->field_0x450 && self->field_0x43D == 1)) {
        if (((u8)kind == 4 && self->area_no == 1) ||
            ((u8)kind == 2 && self->area_no == 9)) {
            if (a % 100 < 0x32) {
                if (self->field_0x382 != 0xff) {
                    u8 state;
                    if (self->field_0x380 == 1) {
                        u8* rec = fn_801377D0(self->state_0x381);
                        state = rec[0x5a6] & 0x7f;
                    } else {
                        state = 0xff;
                    }
                    if (state != 0xff && state != fn_8013023C(self)) {
                        u8 mode;
                        if ((u8)kind == 2) {
                            mode = 8;
                        } else if ((u8)kind == 4) {
                            mode = 2;
                        } else {
                            mode = 0xff;
                        }
                        if (mode != 0xff) {
                            self->field_0x1FC = 1;
                            self->field_0x1FE = 0xa;
                            self->field_0x1FF = mode;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return fn_8012A204(self) == 1;
}

/* 0x801DFC08 (0x134) - step the four action-block floats toward their clamps (up when `fn_8012EC60`
 * answers 1, down otherwise). */
void fn_801DFC08(struct _ENEMY_WORK* self) {
    EmActionBlock* blk = &self->action_0x328;
    if (fn_8012EC60(self) == 1) {
        blk->vec_0x328.x += lbl_80799768;
        if (blk->vec_0x328.x > lbl_80799614) {
            blk->vec_0x328.x = lbl_80799614;
        }
        blk->vec_0x328.y += lbl_807994F8;
        if (blk->vec_0x328.y > lbl_8079976C) {
            blk->vec_0x328.y = lbl_8079976C;
        }
        blk->vec_0x334.x += lbl_80799770;
        if (blk->vec_0x334.x > lbl_80799774) {
            blk->vec_0x334.x = lbl_80799774;
        }
        blk->vec_0x334.y += lbl_80799778;
        if (blk->vec_0x334.y > lbl_8079977C) {
            blk->vec_0x334.y = lbl_8079977C;
        }
    } else {
        blk->vec_0x328.x -= lbl_80799768;
        if (blk->vec_0x328.x < lbl_807994F8) {
            blk->vec_0x328.x = lbl_807994F8;
        }
        blk->vec_0x328.y -= lbl_807994F8;
        if (blk->vec_0x328.y < lbl_807994F8) {
            blk->vec_0x328.y = lbl_807994F8;
        }
        blk->vec_0x334.x -= lbl_80799770;
        if (blk->vec_0x334.x < lbl_807994F8) {
            blk->vec_0x334.x = lbl_807994F8;
        }
        blk->vec_0x334.y -= lbl_80799778;
        if (blk->vec_0x334.y < lbl_807994F8) {
            blk->vec_0x334.y = lbl_807994F8;
        }
    }
}

/* 0x801DFD3C (0x170) - aim at `self->vec_0x36C`: turn the angle from `pos` into a rotation about y
 * and write the resulting point into `out`. */
void fn_801DFD3C(struct _ENEMY_WORK* self, nw4r::math::VEC3* out, u16* angleOut, f32 scale) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 rot;
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 res;
    u16 ang;
    u16 delta;
    VEC3_ctor(&dir);
    VEC3_ctor(&rot);
    copyVec3(&dir, &self->vec_0x36C);
    ang = calcVecAng2(&self->pos, &dir);
    *angleOut = ang;
    delta = (u16)(ang - self->field_0x1C0);
    scale = scale * get_em_chg_scale(self);
    if ((u16)(delta + 0xbfff) > 0x7ffe) {
        f32 base = fn_80050EF4(&dir, &self->pos);
        setVector3(&rot, lbl_807994FC, lbl_807994FC, base - scale);
        rotVecY(&rot, *angleOut);
        fn_80051378(&tmp, &self->pos, &rot);
        copyVec3(out, &tmp);
    } else {
        u16 a2 = (delta < 0x8000) ? 0x4000 : 0xc000;
        *angleOut = (u16)(a2 + self->field_0x1C0);
        setVector3(&rot, lbl_807994FC, lbl_807994FC, -scale);
        rotVecY(&rot, *angleOut);
        fn_80051378(&res, &dir, &rot);
        copyVec3(out, &res);
    }
}

/* 0x801DFEAC (0x40) - clear the helper and set part 7. */
void fn_801DFEAC(struct _ENEMY_WORK* self) {
    u8 tmp[0x0C];
    fn_8005D1AC(&tmp, 0);
    fn_8013A654(self, 7);
}

/* 0x801E0058 (0x164) - "is either part 0x1B or 0x1C within (scale * lbl_80799780)^2 of me". */
u32 fn_801E0058(struct _ENEMY_WORK* self, u8 a) {
    struct _ENEMY_WORK* p;
    f32 d;
    f32 lim;
    p = fn_80131034(self, 0x1b, 0);
    if (p != NULL) {
        if (fn_8012E5A8(p) == 1) {
            if (a == 0) {
                return 1;
            }
            d = calcDistanceSqXZ(&self->pos, &p->pos);
            lim = (lbl_80799780 * get_em_chg_scale(self)) *
                  (lbl_80799780 * get_em_chg_scale(self));
            if (d < lim) {
                return 1;
            }
        }
    }
    p = fn_80131034(self, 0x1c, 0);
    if (p != NULL) {
        if (fn_8012E5A8(p) == 1) {
            if (a == 0) {
                return 1;
            }
            d = calcDistanceSqXZ(&self->pos, &p->pos);
            lim = (lbl_80799780 * get_em_chg_scale(self)) *
                  (lbl_80799780 * get_em_chg_scale(self));
            if (d < lim) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x801E01BC (0x28) - "action 0xD with sub-state <= 5". */
u32 fn_801E01BC(struct _ENEMY_WORK* self) {
    if (self->action == 0xd && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* 0x801E01E4 (0x5C) - the deleting destructor of the +0x4 helper. */
void* fn_801E01E4(void* self, s16 flag) {
    if (self != NULL) {
        fn_8013918C(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x801E04DC (0x5C) - the deleting destructor of the res-object helper. */
void* fn_801E04DC(void* self, s16 flag) {
    if (self != NULL) {
        fn_800810DC(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x801E0538 (0x3C) - the part-manager state dispatcher (index at +0x5). */
void fn_801E0538(struct EmEftPartsMan* self) {
    switch (self->state) {
    case 0:
        fn_801E0574(self);
        break;
    case 1:
        fn_801E0D38(self);
        break;
    case 2:
        fn_801E1A2C(self);
        break;
    case 3:
        fn_801E1A3C(self);
        break;
    }
}

/* 0x801E0AC0 (0x1C) - `base + offset`, or NULL when the offset is 0. */
void* fn_801E0AC0(void** base, u32 offset) {
    u8* p = (u8*)*base;
    if (offset != 0) {
        return p + offset;
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif
