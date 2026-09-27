/*
 * ai/fn_802CC794.cpp - the 0x802CC794-0x802D0DCC AI-NPC work band (63 functions, 17976 B).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py lookup + `grep -n "fn_802CC" config/RMHE08/symbols.txt`: every defined
 * name lands as a bare `.text` entry, and no `__FILE__` string or runtime-dump name covers it).
 *
 * Evidence for the home and the type:
 *   - the pointer at r3 is passed straight to `get_enemy_data` / `get_em_chg_scale` (whose parameter
 *     the map spells `_ENEMY_WORK*`) and to `ai_skill_ck` / `ai_torch_ck` (parameter `_AINPC_W*`),
 *     while the prologue of `fn_802CC794` computes `get_move_work_adrs(3) + self->enemy_index*0xB18`
 *     to reach the `_ENEMY_WORK` records.  So the `self` this band owns is the AI-NPC work record,
 *     and its name must be `_AINPC_W` or the callees mangle to the wrong map name (rule 9);
 *   - the registered unit that brackets the band above is `ai/fn_802D0DCC.c` (0x802D0DCC), and
 *     `ai_skill_ck`/`ai_torch_ck` sit in the same band, so the module is `ai`.
 *
 * Registered NonMatching; the body below is the part reconstructed so far, in address order.
 *
 * Score at this commit: 42 of 63 functions, 40/63 at >=80 % (27 at 100 %, 13 in 91-99 %), 46.57 %
 * of the unit's bytes (measured with `tools/units/recompile.py ai/fn_802CC794 --measure <sym>`
 * against `build/RMHE08/obj/ai/fn_802CC794.o`).
 *
 * Residuals: (1) `fn_802CDAB8` 62.9 % - the target's `field_0x1F0 <= (s32)((f32)field_0x1F4 *
 * formation->budget)` uses a mixed-sign 64-bit idiom (srawi/srwi/subfc/adde) that no C spelling
 * tried reproduces (direct 62.9, s64 cast 53.1, reversed >= 41.3).  (2) `fn_802CD588` 78.5 % - the
 * `switch` on `field_0x420` emits a 4-instruction signed range test where the target emits the
 * 3-instruction `subi`/`cmplwi` unsigned one (a `u32` switch local made it worse at 74.3 %).
 * (3) 21 functions are not attempted: they read `_PLW` fields at +0x3C, and `pl.h` cannot be
 * included beside `mh3_pad.h` because `ef.h` re-declares `VEC3_ctor`/`setVec3` with
 * signatures that clash with their owners' headers ((10197) illegal function overloading).
 */

#include "types.h"
#include "nw4r/math.h"
#include "fn_8004CAD8.h"
#include "mh3_pad.h"
#include "ef/fn_800CDB2C.h"
#include "Pl/pl_act.h"

/* The target's player work (`_PLW`), stored at +0x16C; only pointed at and forwarded by this band
 * (its +0x3C triple is the position).  `pl.h` cannot be included beside `mh3_pad.h` - `ef.h`
 * re-declares `VEC3_ctor`/`setVec3` with signatures that clash with the owners' headers. */
struct _PLW;

/* The 0x10-byte record `+0x41C` points at; only its float at +0xC (`fn_802CDAB8`'s frame-budget
 * factor) is evidenced by this band.  size: 0x10 (approximate, evidenced to +0xC) */
struct AINPCFormation {
    /* +0x0 */ u8 unused_0x00[0xC];
    /* +0xC */ f32 budget_0x0C;
};

/* The AI-NPC work record (`_AINPC_W*` in the target's own manglings, `ai_skill_ck__FP8_AINPC_WUc`
 * and `ai_torch_ck__FP8_AINPC_W`).  The layout is the union of the offsets this band accesses; an
 * offset no reconstructed function touches is kept as padding so the numbers stay exact.
 * size: 0x4A4 (approximate - evidenced to +0x492 only) */
struct _AINPC_W {
    /* +0x000 */ u8 unused_0x000[0x003 - 0x000];
    /* +0x003 */ u8 team;              /* read by `get_enemy_data` as its first argument */
    /* +0x004 */ u8 unused_0x004[0x00A - 0x004];
    /* +0x00A */ u8 area;              /* read by `get_enemy_data` as its second argument */
    /* +0x00B */ u8 unused_0x00B[0x054 - 0x00B];
    /* +0x054 */ u32 field_0x054;
    /* +0x058 */ u32 field_0x058;
    /* +0x05C */ u8 unused_0x05C[0x16C - 0x05C];
    /* +0x16C */ struct _PLW* plw_0x16C;  /* the player work it targets (its +0x3C is a VEC3) */
    /* +0x170 */ u8 variant;            /* 1 or 2 - selects the odd/even arm of the dispatchers */
    /* +0x171 */ u8 field_0x171;
    /* +0x172 */ u8 unused_0x172[0x178 - 0x172];
    /* +0x178 */ nw4r::math::VEC3 vec_0x178;
    /* +0x184 */ u8 unused_0x184[0x194 - 0x184];
    /* +0x194 */ u32 field_0x194;
    /* +0x198 */ u8 unused_0x198[0x1A4 - 0x198];
    /* +0x1A4 */ u8 field_0x1A4;
    /* +0x1A5 */ u8 unused_0x1A5[0x1B0 - 0x1A5];
    /* +0x1B0 */ nw4r::math::VEC3 vec_0x1B0;
    /* +0x1BC */ u8 unused_0x1BC[0x1C0 - 0x1BC];
    /* +0x1C0 */ f32 field_0x1C0;
    /* +0x1C4 */ u8 enemy_index;        /* scaled by 0xB18 into the `_ENEMY_WORK` array */
    /* +0x1C5 */ u8 unused_0x1C5[0x1C8 - 0x1C5];
    /* +0x1C8 */ u32 field_0x1C8;
    /* +0x1CC */ u8 field_0x1CC;
    /* +0x1CD */ u8 pad_0x1CD;
    /* +0x1CE */ u8 pad_0x1CE;
    /* +0x1CF */ u8 field_0x1CF;
    /* +0x1D0 */ u8 step;               /* the per-motion step every dispatcher switches on */
    /* +0x1D1 */ u8 unused_0x1D1[0x1E1 - 0x1D1];
    /* +0x1E1 */ u8 field_0x1E1;
    /* +0x1E2 */ u8 unused_0x1E2[0x1E5 - 0x1E2];
    /* +0x1E5 */ u8 field_0x1E5;
    /* +0x1E6 */ u8 unused_0x1E6[0x1F0 - 0x1E6];
    /* +0x1F0 */ u32 field_0x1F0;
    /* +0x1F4 */ u32 field_0x1F4;
    /* +0x1F8 */ u8 unused_0x1F8[0x30E - 0x1F8];
    /* +0x30E */ u8 field_0x30E;
    /* +0x30F */ u8 unused_0x30F[0x326 - 0x30F];
    /* +0x326 */ u8 field_0x326;
    /* +0x327 */ u8 unused_0x327[0x349 - 0x327];
    /* +0x349 */ u8 field_0x349;
    /* +0x34A */ u8 field_0x34A;
    /* +0x34B */ u8 field_0x34B;
    /* +0x34C */ u8 field_0x34C;
    /* +0x34D */ u8 field_0x34D;
    /* +0x34E */ u8 field_0x34E;
    /* +0x34F */ u8 field_0x34F;
    /* +0x350 */ s16 field_0x350;
    /* +0x352 */ u16 field_0x352;
    /* +0x354 */ u8 field_0x354;
    /* +0x355 */ u8 unused_0x355[0x358 - 0x355];
    /* +0x358 */ u8 field_0x358;
    /* +0x359 */ u8 unused_0x359[0x35C - 0x359];
    /* +0x35C */ u8 field_0x35C;
    /* +0x35D */ u8 unused_0x35D;
    /* +0x35E */ s16 field_0x35E;
    /* +0x360 */ u8 unused_0x360[0x364 - 0x360];
    /* +0x364 */ u32 field_0x364;
    /* +0x368 */ u8 unused_0x368[0x376 - 0x368];
    /* +0x376 */ u8 field_0x376;
    /* +0x377 */ u8 field_0x377;
    /* +0x378 */ u8 field_0x378;
    /* +0x379 */ u8 unused_0x379[0x380 - 0x379];
    /* +0x380 */ u16 field_0x380;
    /* +0x382 */ u8 field_0x382;
    /* +0x383 */ u8 unused_0x383;
    /* +0x384 */ u8 field_0x384;
    /* +0x385 */ u8 unused_0x385[0x389 - 0x385];
    /* +0x389 */ u8 field_0x389;
    /* +0x38A */ u8 unused_0x38A[0x38E - 0x38A];
    /* +0x38E */ u8 field_0x38E;
    /* +0x38F */ u8 unused_0x38F[0x3D2 - 0x38F];
    /* +0x3D2 */ u8 field_0x3D2;
    /* +0x3D3 */ u8 unused_0x3D3[0x3DE - 0x3D3];
    /* +0x3DE */ u8 field_0x3DE;
    /* +0x3DF */ u8 unused_0x3DF[0x3F8 - 0x3DF];
    /* +0x3F8 */ u8 field_0x3F8;
    /* +0x3F9 */ u8 unused_0x3F9[0x41C - 0x3F9];
    /* +0x41C */ struct AINPCFormation* formation_0x41C;
    /* +0x420 */ u8 field_0x420;
    /* +0x421 */ u8 unused_0x421;
    /* +0x422 */ s16 field_0x422;
    /* +0x424 */ u8 unused_0x424[0x42E - 0x424];
    /* +0x42E */ u8 skill_0x42E[3];     /* the three skill slots `ai_skill_ck` compares against */
    /* +0x431 */ u8 unused_0x431[0x43D - 0x431];
    /* +0x43D */ u8 field_0x43D;
    /* +0x43E */ s16 field_0x43E;
    /* +0x440 */ u8 field_0x440;
    /* +0x441 */ u8 unused_0x441[0x445 - 0x441];
    /* +0x445 */ u8 field_0x445;
    /* +0x446 */ u8 unused_0x446;
    /* +0x447 */ u8 field_0x447;
    /* +0x448 */ u8 field_0x448;
    /* +0x449 */ u8 unused_0x449[0x451 - 0x449];
    /* +0x451 */ u8 field_0x451;
    /* +0x452 */ u8 unused_0x452[0x460 - 0x452];
    /* +0x460 */ u8 field_0x460;
    /* +0x461 */ u8 field_0x461;
    /* +0x462 */ u16 field_0x462;
    /* +0x464 */ s16 field_0x464;
    /* +0x466 */ u8 unused_0x466[0x468 - 0x466];
    /* +0x468 */ u8 field_0x468;
    /* +0x469 */ u8 unused_0x469;
    /* +0x46A */ s16 field_0x46A;
    /* +0x46C */ u8 unused_0x46C[0x482 - 0x46C];
    /* +0x482 */ u8 field_0x482;
    /* +0x483 */ u8 unused_0x483[0x485 - 0x483];
    /* +0x485 */ u8 field_0x485;
    /* +0x486 */ u8 unused_0x486[0x492 - 0x486];
    /* +0x492 */ u8 field_0x492;
};

/* The C++-spelled callees: their map names carry argument lists, so declaring them at global C++
 * scope and calling the plain name is what reproduces the target's mangling (rule 9).  `ai_*` are
 * unowned (module gap), so rule 2 leaves them here. */
s32 ai_torch_ck(struct _AINPC_W* self);
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill);

#ifdef __cplusplus
extern "C" {
#endif

/* The unregistered helpers this band dispatches into.  0x8005xxxx/0x8027xxxx/0x802Dxxxx are outside
 * this unit's range and owned by nobody yet, so stylelint's rule 2 leaves them a counted gap
 * (`tools/units/stylelint.py`, "address band interleaves modules").  The signatures are the call
 * sites' registers here. */
u32 fn_802CB900(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 a);
u32 fn_8027D74C(struct _PLW* plw);
void fn_802CCF00(struct _AINPC_W* self, u32 a);
void fn_802D32B4(struct _AINPC_W* self, void* entry);
u32 fn_8027E06C(struct _PLW* plw, u32 a);
u32 fn_802D86D4(struct _AINPC_W* self, u16* a, s16* b);
void fn_802D65CC(struct _AINPC_W* self);
void fn_802D675C(struct _AINPC_W* self);
void fn_802D67F8(struct _AINPC_W* self);
void fn_802D3A20(struct _AINPC_W* self);
void fn_802D3AD8(struct _AINPC_W* self);
f32 fn_802D7258(struct _AINPC_W* self, u32 a);

/* 0x805D4030 - the 4-pointer table the dispatchers hand to `fn_802D32B4`; its entries live in the
 * (`auto`) data band, so it is only referenced here (playbook 29). */
extern void* lbl_805D4030[4];
s32 fn_802D3184(struct _AINPC_W* self, u32 flag);
void fn_802D3B10(struct _AINPC_W* self);
void fn_802D3B24(struct _AINPC_W* self);
void fn_802D4200(struct _AINPC_W* self);
void fn_802D4218(struct _AINPC_W* self);
void fn_802D4230(struct _AINPC_W* self, f32 value);
void fn_802D4238(struct _AINPC_W* self);
void fn_802D2A00(struct _AINPC_W* self, u32 a, u32 b, u32 c);
void fn_802D9D30(struct _AINPC_W* self, s32 a, u32 b, u32 c, u32 d);
s16 fn_802D9D14(void);

/* 0x802CD20C/0x802CCF00/0x802CCE18 are inside this unit and reconstructed below. */

/* 0x802CCD90 - the 0x350-frame gate: once the countdown has run out it latches +0x354 and arms the
 * 0x708-frame re-check. */
s32 fn_802CCD90(struct _AINPC_W* self)
{
    if (self->field_0x354 != 0) {
        return 1;
    }
    if (self->field_0x350 >= fn_802D9D14()) {
        self->field_0x354 = 1;
        self->field_0x352 = 0x708;
        fn_802D4218(self);
        return 1;
    }
    return 0;
}

/* 0x802CCE04 - true while the gate byte is exactly 2. */
s32 fn_802CCE04(struct _AINPC_W* self)
{
    return self->field_0x354 == 2;
}

/* 0x802CD75C - true while the +0x468 flag is set. */
u32 fn_802CD75C(struct _AINPC_W* self)
{
    return self->field_0x468 != 0;
}

/* 0x802CDF4C - queue the variant's intro command. */
void fn_802CDF4C(struct _AINPC_W* self)
{
    if (self->variant == 2) {
        fn_802D2A00(self, 1, 0xF, 0);
    } else {
        fn_802D2A00(self, 1, 8, 0);
    }
}

/* 0x802CDF78 - the 4-step "recover from stagger" handler. */
void fn_802CDF78(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 150.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        if (fn_802D3184(self, 0x2000) == 0) {
            fn_802D2A00(self, 2, 0x10, 0);
            self->step += 1;
        } else {
            if ((u16)ran_suu(1) & 1) {
                fn_802D2A00(self, 3, 9, 0);
            } else {
                fn_802D2A00(self, 3, 0xD, 0);
            }
            self->step = 3;
        }
        break;
    case 2:
        if ((u16)ran_suu(1) & 1) {
            fn_802D2A00(self, 3, 9, 0);
        } else {
            fn_802D2A00(self, 3, 0xD, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE0D8 - the 3-step "approach" handler. */
void fn_802CE0D8(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 3, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE170 - rebuild `vec_0x1B0` from `vec_0x178` rotated by the angle the two make, then queue
 * the follow-up command.  The stack VEC3 is the rotated offset `fn_80051378` turns into the new
 * aim point. */
void fn_802CE170(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
    {
        nw4r::math::VEC3 offset;
        s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

        setVector3(&tmp, 0.0f, 0.0f, 800.0f);
        rotVecY(&tmp, angle);
        fn_80051378(&offset, &self->vec_0x178, &tmp);
        copyVec3(&self->vec_0x1B0, &offset);
        self->field_0x389 = 0;
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 3, 0x12, 0);
        self->step += 1;
        break;
    }
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0xA, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF390 - the 3-step "hold/flinch" handler. */
void fn_802CF390(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 7, 0);
        } else if (self->field_0x1C0 > 30.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 3, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (self->field_0x358 != 0) {
            fn_802D2A00(self, 1, 4, 0);
        } else if (self->field_0x35C != 0) {
            fn_802D2A00(self, 1, 0x1A, 0);
        }
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFABC - latch +0x3D2 and queue the variant's command. */
void fn_802CFABC(struct _AINPC_W* self)
{
    self->field_0x3D2 = 1;
    if (self->variant == 2) {
        fn_802D2A00(self, 5, 6, 0);
    } else {
        fn_802D2A00(self, 5, 5, 0);
    }
}

/* 0x802CFBC8 - queue the +0x440-dependent command. */
void fn_802CFBC8(struct _AINPC_W* self)
{
    if (self->field_0x440 == 1) {
        fn_802D2A00(self, 1, 0x54, 0);
    } else {
        fn_802D2A00(self, 1, 0x52, 0);
    }
}

/* 0x802CF704 - the 4-step "use item" handler. */
void fn_802CF704(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 8, 0);
        } else {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CFFD0 - the 3-step "guard" handler. */
void fn_802CFFD0(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D07AC - the 3-step "roar" handler. */
void fn_802D07AC(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D08D4 - the 4-step "pick target" handler. */
void fn_802D08D4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CD6FC - true once the +0x460 latch is set or the player work reports state 1. */
u32 fn_802CD6FC(struct _AINPC_W* self)
{
    if (self->field_0x460 == 1) {
        return 1;
    }
    if (fn_8027D74C(self->plw_0x16C) == 1) {
        self->field_0x460 = 1;
        return 1;
    }
    return 0;
}

/* 0x802CDAB8 - true while the elapsed frame count has not passed the formation's budget-scaled
 * factor. */
u32 fn_802CDAB8(struct _AINPC_W* self)
{
    return self->field_0x1F0 <= (s32)((f32)self->field_0x1F4 * self->formation_0x41C->budget_0x0C);
}

/* 0x802CDDC4 - the 4-step "switch to another action" handler. */
void fn_802CDDC4(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 130.0f);
        if (arg == 0) {
            fn_802D2A00(self, 2, 4, 0);
        } else {
            fn_802D2A00(self, 3, 0x12, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (fn_802D3184(self, 0x2000) == 0) {
            fn_802D2A00(self, 2, 5, 0);
            self->step += 1;
        } else {
            if (self->vec_0x1B0.y - self->vec_0x178.y > 180.0f) {
                fn_802D2A00(self, 3, 8, 0);
            } else {
                fn_802D32B4(self, lbl_805D4030[2]);
            }
            self->step = 3;
        }
        break;
    case 2:
        if (self->vec_0x1B0.y - self->vec_0x178.y > 180.0f) {
            fn_802D2A00(self, 3, 8, 0);
        } else {
            fn_802D32B4(self, lbl_805D4030[2]);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE2A0 - the distance-banded dispatcher behind the "watch" action. */
void fn_802CE2A0(struct _AINPC_W* self)
{
    if (self->field_0x1C0 > 700.0f) {
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 7, 0);
        return;
    }
    if (self->field_0x1C0 > 400.0f) {
        fn_802D4230(self, 250.0f);
        fn_802D2A00(self, 2, 2, 0);
        return;
    }
    if (self->field_0x1C0 > 200.0f) {
        fn_802D2A00(self, 2, 0, 0);
        return;
    }
    if (fn_802D3184(self, 0x5000) == 0 && self->field_0x1C0 > 50.0f) {
        fn_802D2A00(self, 2, 5, 0);
        return;
    }
    if (fn_802CCE04(self) == 1) {
        fn_802D32B4(self, lbl_805D4030[3]);
        return;
    }
    fn_802D32B4(self, lbl_805D4030[0]);
}

/* 0x802CE618 - the distance-banded dispatcher behind the "hold ground" action. */
void fn_802CE618(struct _AINPC_W* self)
{
    if (self->field_0x1C0 > 700.0f) {
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 0xB, 0);
        return;
    }
    if (self->field_0x1C0 > 300.0f) {
        fn_802D4230(self, 150.0f);
        fn_802D2A00(self, 2, 0xA, 0);
    }
}

/* 0x802D0068 - the 4-step "turn" handler. */
void fn_802D0068(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        if (arg == 0) {
            fn_802D2A00(self, 3, 6, 0);
        } else {
            fn_802D2A00(self, 3, 7, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0148 - the 4-step "turn" handler, the long variant. */
void fn_802D0148(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 1000.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        if (arg == 0) {
            fn_802D2A00(self, 3, 0xF, 0);
        } else {
            fn_802D2A00(self, 3, 0x10, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0228 - the 3-step "ready stance" handler. */
void fn_802D0228(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 3, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0x13, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D02C0 - the 3-step "ready stance" handler, the long variant. */
void fn_802D02C0(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0x14, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0358 - the 2-step "sidestep" handler: it rotates the +0x178 aim-offset into +0x1B0. */
void fn_802D0358(struct _AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
    {
        s32 angle;

        fn_802CCF00(self, 0);
        angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);
        setVector3(&v, 0.0f, 0.0f, 800.0f);
        if (arg == 0) {
            setVector3(&v, 0.0f, 0.0f, fn_802D7258(self, 0));
        } else {
            setVector3(&v, 0.0f, 0.0f, fn_802D7258(self, 1));
        }
        rotVecY(&v, angle);
        self->vec_0x1B0.x = self->vec_0x178.x + v.x;
        self->vec_0x1B0.y = self->vec_0x178.y;
        self->vec_0x1B0.z = self->vec_0x178.z + v.z;
        fn_802D4230(self, 30.0f);
        if (arg == 0) {
            fn_802D2A00(self, 1, 6, 0);
        } else {
            fn_802D2A00(self, 1, 0x10, 0);
        }
        self->step += 1;
        break;
    }
    case 1:
        fn_802D4200(self);
        fn_802D3A20(self);
        break;
    }
}

/* 0x802D04CC - the 4-step "step in" handler. */
void fn_802D04CC(struct _AINPC_W* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 == 400.0f) {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&v, 0.0f, 0.0f, 800.0f);
            rotVecY(&v, angle);
            self->vec_0x1B0.x = self->vec_0x178.x + v.x;
            self->vec_0x1B0.y = self->vec_0x178.y;
            self->vec_0x1B0.z = self->vec_0x178.z + v.z;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->field_0x389 = 0;
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 1);
        self->field_0x389 = 1;
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 3, 0x15, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D063C - the 4-step "step in" handler, the far variant. */
void fn_802D063C(struct _AINPC_W* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 == 400.0f) {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&v, 0.0f, 0.0f, 800.0f);
            rotVecY(&v, angle);
            self->vec_0x1B0.x = self->vec_0x178.x + v.x;
            self->vec_0x1B0.y = self->vec_0x178.y;
            self->vec_0x1B0.z = self->vec_0x178.z + v.z;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        }
        self->field_0x389 = 0;
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 1);
        self->field_0x389 = 1;
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 3, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0840 - the 3-step "roar" handler. */
void fn_802D0840(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0994 - the 2-step "post-roar" handler. */
void fn_802D0994(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->variant == 2) {
            fn_802D2A00(self, 1, 0x16, 0);
        } else {
            fn_802D2A00(self, 1, 0xA, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802D4200(self);
        fn_802D3AD8(self);
        break;
    }
}

/* 0x802D0A20 - the 4-step "taunt" handler. */
void fn_802D0A20(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D4230(self, 500.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 2:
        if (fn_80278310(self->field_0x1A4, &self->vec_0x178, self->field_0x326) == 1) {
            fn_802D2A00(self, 1, 0x1F, 0);
        }
        self->field_0x448 += 1;
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802D0B28 - the 4-step "taunt" handler, the long variant. */
void fn_802D0B28(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D4230(self, 500.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 2:
        if (fn_80278310(self->field_0x1A4, &self->vec_0x178, self->field_0x326) == 1) {
            fn_802D2A00(self, 1, 0x3B, 0);
        }
        self->field_0x448 += 1;
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CD20C - the player-action predicate chain the state machines test. */
s16 fn_802CD20C(struct _AINPC_W* self)
{
    struct _PLW* plw = self->plw_0x16C;

    if (Pl_condition_ck(plw, 0x30A) == 1) {
        return 0;
    }
    if (Pl_condition_ck(plw, 4) == 1) {
        return 1;
    }
    if (Pl_condition_ck(plw, 0x40000) == 1) {
        return ai_torch_ck(self) != 1;
    }
    if (fn_8027E06C(plw, 0) == 1) {
        return 0;
    }
    return fn_8027E06C(plw, 1) != 1;
}

/* 0x802CD2E0 - latch the +0x34D gate once the predicate chain answers non-negative. */
u32 fn_802CD2E0(struct _AINPC_W* self)
{
    s16 r;

    if (self->field_0x34D != 0) {
        return 1;
    }
    r = fn_802CD20C(self);
    if (r >= 0) {
        self->field_0x34D = 1;
        self->field_0x34E = (u8)r;
        return 1;
    }
    return 0;
}

/* 0x802CD588 - the stance/guard dispatcher. */
u32 fn_802CD588(struct _AINPC_W* self)
{
    u16 a;
    s16 b;

    if (self->field_0x461 == 1) {
        return 1;
    }
    if (self->field_0x43E <= 0) {
        self->field_0x43D = 0;
    }
    if (self->field_0x171 == 5) {
        return 0;
    }
    if (self->field_0x43D == 1) {
        self->field_0x43D = 2;
        fn_802D4218(self);
        switch (self->field_0x420) {
        case 3:
        case 4:
            if (fn_802D86D4(self, &a, &b) == 1) {
                if (a != 0) {
                    self->field_0x462 = a;
                    self->field_0x464 = b;
                    self->field_0x461 = 1;
                    fn_802D9D30(self, (s8)self->field_0x482, 0x12, 0, 2);
                }
            } else {
                fn_802D9D30(self, (s8)self->field_0x482, 0x11, 0, 2);
            }
            break;
        case 1:
            fn_802D65CC(self);
            break;
        case 2:
            fn_802D675C(self);
            break;
        case 5:
            fn_802D67F8(self);
            break;
        default:
            fn_802D9D30(self, (s8)self->field_0x482, 0x11, 0, 2);
            break;
        }
    }
    return self->field_0x43D != 0;
}

/* 0x802CE3B4 - the 4-step "flinch" handler. */
void fn_802CE3B4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 180.0f) {
            fn_802D4230(self, 180.0f);
            fn_802D2A00(self, 2, 8, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (fn_802CD20C(self) < 0) {
            fn_802D3B10(self);
            fn_802D4200(self);
        } else {
            fn_802D2A00(self, 2, 5, 0);
            self->step += 1;
        }
        break;
    case 2:
        if (self->field_0x34E == 0) {
            fn_802D2A00(self, 3, 0, 4);
        } else {
            fn_802D2A00(self, 3, 1, 4);
        }
        fn_802D9D30(self, (s8)self->field_0x482, 0x1B, 0, 2);
        self->step += 1;
        break;
    case 3:
        fn_802D3B10(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE4EC - the 4-step "flinch" handler, the long variant. */
void fn_802CE4EC(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 180.0f);
        fn_802D2A00(self, 2, 0xB, 0);
        self->step += 1;
        break;
    case 1:
        if (fn_802CD20C(self) < 0) {
            fn_802D3B10(self);
            fn_802D4200(self);
        } else {
            fn_802D2A00(self, 2, 0x10, 0);
            self->step += 1;
        }
        break;
    case 2:
        if (self->field_0x34E == 0) {
            fn_802D2A00(self, 3, 9, 4);
        } else {
            fn_802D2A00(self, 3, 0xD, 4);
        }
        fn_802D9D30(self, (s8)self->field_0x482, 0x1B, 0, 2);
        self->step += 1;
        break;
    case 3:
        fn_802D3B10(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF48C - the 3-step "call" handler. */
void fn_802CF48C(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xB, 0);
        } else if (self->field_0x1C0 > 30.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xE, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (self->field_0x358 != 0) {
            fn_802D2A00(self, 1, 0xE, 0);
        } else if (self->field_0x35C != 0) {
            fn_802D2A00(self, 1, 0x34, 0);
        }
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF8E4 - the 4-step "call" handler, the long variant. */
void fn_802CF8E4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        } else {
            fn_802D4230(self, 50.0f);
            fn_802D2A00(self, 2, 0xE, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CFBF4 - the 3-step "call" handler, the short variant. */
void fn_802CFBF4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0xE, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFC94 - the 4-step "sidestep" handler. */
void fn_802CFC94(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 offset;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 800.0f);
            fn_802D2A00(self, 2, 4, 0);
        } else {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&tmp, 0.0f, 0.0f, 800.0f);
            rotVecY(&tmp, angle);
            fn_80051378(&offset, &self->vec_0x178, &tmp);
            copyVec3(&self->vec_0x1B0, &offset);
            self->field_0x389 = 0;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x39, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D2A00(self, 1, 0xA, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFE20 - the 4-step "sidestep" handler, the far variant. */
void fn_802CFE20(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 delta;
    nw4r::math::VEC3 offset;
    u32 x, z;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 1100.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        } else {
            fn_80050CA0(&delta, &self->vec_0x178, &self->vec_0x1B0);
            copyVec3(&tmp, &delta);
            calcVecAngXY(&tmp, &x, &z);
            setVector3(&tmp, 0.0f, 0.0f, 800.0f);
            rotVecX(&tmp, x);
            rotVecY(&tmp, (u16)z);
            fn_80051378(&offset, &self->vec_0x178, &tmp);
            copyVec3(&self->vec_0x1B0, &offset);
            self->field_0x389 = 0;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802CCF00(self, 0);
        fn_802D2A00(self, 2, 0x10, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x3A, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D2A00(self, 1, 0x16, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0C30 - the 2-step "sleep" handler. */
void fn_802D0C30(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 1, 0x5A, 0);
        self->step += 1;
        break;
    case 1:
        self->field_0x485 = 0;
        fn_802D4200(self);
        break;
    }
}

#ifdef __cplusplus
}
#endif
