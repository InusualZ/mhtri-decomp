/* ef/eft029.cpp - the head of the eft029 effect family: the per-frame handler `fn_80119DEC` (runs the timer down,
 *   picks the (id, param) pair from the map area, stage area and movie state, builds the pool and places it by
 *   type) and its create arm `fn_8011A2A0` (`res_eft_create(idTable[type], 4, 0)` per pool entry).
 * RANGE. .text 0x80119DEC-0x8011AD58 (10 functions); extab 0x8000C56C-0x8000C5A4, extabindex 0x8002670C-0x80026760,
 *   .ctors 0x8056F314-0x8056F318, .data 0x805A06F0-0x805A07F0, .bss 0x806A4548-0x806A4560, .sdata
 *   0x80791970-0x80791978, .sdata2 0x80796B30-0x80796B50.  The family's setters `eft029_set_scale`/`eft029_set_kaihou`
 *   (the runtime dump's names, which name the family) and the rest of it are `ef/eft029_fx.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` over both bodies.
 * NAMES. The map has only `fn_` stems here, so the definitions are `extern "C"` in a C++ unit.
 * RESIDUALS. 8 rows unwritten: 0x8011A34C-0x8011AD58 (`fn_8011A34C` to `fn_8011AD04`); `fn_8011A34C` and
 *   `fn_8011AD00` are declared because the two written bodies call them.  Both written rows match.
 *   flipcheck: `.bss`/`.ctors`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0x560 of 0xF6C), extab (0x10 of 0x38),
 *   extabindex (0x18 of 0x54) and `.data` (0x6C of 0x100) short of the claim and differing.
 * SHAPES. `_EFT029_WORK` is a lower bound: only `count` (+0x00) and `effects[]` (+0x04) are reached.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft001.h"
#include "unsplit/unknown.h"
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */

extern "C" {
void fn_800DD1F0(u32 id, nw4r::math::VEC3* pos);
u32 getItemListSelection();
void fn_8011AD00(_EFT* self);
void fn_8011A34C(_EFT* self);
}

/* `map_se_req(u8, nw4r::math::VEC3*)` - `sound/se_req.cpp`'s (0x800DDCB8), declared locally with the map's mangled
 * spelling (a rule-2 residual). */
void map_se_req(u8 id, nw4r::math::VEC3* pos);

/* The per-family work block `_EFT::work_0x38` points at.  Only the fields these two functions read are
 * named; the block continues past +0x08 (the TU's other functions use it), so the size is a lower
 * bound.  size: 0x08 (approximation) */
struct _EFT029_WORK {
    /* +0x00 */ s32 count;                        /* the number of pooled effects at +0x04 */
    /* +0x04 */ nw4r::ef::Effect* effects[1];     /* the pool `res_eft_create` fills */
};

extern "C" {
u16 lbl_805A06F0[]; /* effect id per `type_0x02`, .data 0x805A06F0 */
u16 lbl_805A0710[]; /* its parameter pair,      .data 0x805A0710 */
}

#pragma peephole off

/* 0x80119DEC (0x4B4): Runs the timer down and, the frame it expires, picks the (id, param) pair, creates the pool
 * and places it by type; a failure destroys the record through `fn_8011AD00`. */
extern "C" void fn_80119DEC(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    _EFT029_WORK* work;
    int id;
    u16 param;
    s32 i;

    MTX34_ctor(&mtx);
    work = (_EFT029_WORK*)self->work_0x38;
    if (--self->timer_0x0C >= 0) {
        return;
    }
    self->state_0x05++;
    id = lbl_805A06F0[self->type_0x02];
    param = lbl_805A0710[self->type_0x02];

    switch (self->type_0x02) {
    case 1:
        switch (get_now_mapno()) {
        case 1:
            switch (self->area_0x44) {
            case 7:
                id = 140;
                param = 53;
                break;
            case 12:
                id = 139;
                param = 17;
                break;
            default:
                fn_8011AD00(self);
                return;
            }
            break;
        case 7:
            switch (self->area_0x44) {
            case 1:
                id = 2472;
                param = 181;
                break;
            case 2:
                id = 2476;
                param = 182;
                break;
            default:
                fn_8011AD00(self);
                return;
            }
            break;
        case 12:
            switch (self->area_0x44) {
            case 7:
                id = 142;
                param = 128;
                break;
            case 12:
                id = 141;
                param = 132;
                break;
            default:
                fn_8011AD00(self);
                return;
            }
            break;
        case 18:
            switch (self->area_0x44) {
            case 1:
                id = 2479;
                param = 183;
                break;
            case 2:
                id = 2483;
                param = 184;
                break;
            default:
                fn_8011AD00(self);
                return;
            }
            break;
        default:
            fn_8011AD00(self);
            return;
        }
        break;
    case 5:
        switch (self->area_0x44) {
        case 1:
            id = 899;
            param = 72;
            break;
        case 2:
            id = 900;
            param = 77;
            break;
        case 3:
            id = 901;
            param = 78;
            break;
        case 4:
            id = 902;
            param = 70;
            break;
        case 5:
            id = 903;
            param = 74;
            break;
        case 7:
            id = 931;
            param = 80;
            break;
        case 9:
            id = 963;
            param = 81;
            break;
        case 10:
            id = 918;
            param = 79;
            break;
        default:
            fn_8011AD00(self);
            return;
        }
        break;
    case 6:
        fn_800DD1F0(0, &self->pos_0x18);
        switch (get_now_mapno()) {
        case 5:
            if ((s32)self->area_0x44 == 9) {
                id = 1364;
                param = 101;
            }
            break;
        case 16:
            if ((s32)self->area_0x44 == 9) {
                id = 2432;
                param = 177;
            } else {
                id = 2440;
                param = 178;
            }
            break;
        }
        break;
    case 9:
        if ((s32)get_now_mapno() == 15) {
            id = 2532;
            param = 163;
        }
        break;
    case 10:
        switch (get_now_mapno()) {
        case 6:
            switch (self->area_0x44) {
            case 1:
                id = 2496;
                param = 61;
                break;
            case 2:
                id = 2497;
                param = 116;
                break;
            default:
                fn_8011AD00(self);
                return;
            }
            break;
        case 17:
            switch (self->area_0x44) {
            case 1:
                id = 2529;
                param = 187;
                break;
            case 2:
                id = 2530;
                param = 188;
                break;
            default:
                fn_8011AD00(self);
                return;
            }
            break;
        }
        break;
    case 11:
        map_se_req(10, &self->pos_0x18);
        break;
    case 12:
        if (getItemListSelection() == 0) {
            id = lbl_805A06F0[self->type_0x02];
            param = lbl_805A0710[self->type_0x02];
        } else {
            id = 1639;
            param = 22;
        }
        break;
    }

    for (i = 0; i < work->count; i++) {
        work->effects[i] = res_eft_create((u16)id, param, 0);
        if (work->effects[i] == NULL) {
            fn_8011AD00(self);
            return;
        }
        id++;
    }

    switch (self->type_0x02) {
    case 1:
    case 6:
    case 9:
    case 11:
    case 14:
    case 15:
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        }
        break;
    case 8:
    case 10:
    case 12:
    case 13:
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        fn_800FBB90(&mtx, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
        }
        break;
    }

    self->flag_0x01 = 1;
    fn_8011A34C(self);
}

/* 0x8011A2A0 (0xAC): Fills the pool with one `res_eft_create(idTable[type], 4, 0)` per slot and runs the placement
 * dispatcher; the first failed create destroys the record. */
extern "C" void fn_8011A2A0(_EFT* self)
{
    s32 i;
    _EFT029_WORK* work = (_EFT029_WORK*)self->work_0x38;

    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        work->effects[i] = res_eft_create(lbl_805A06F0[self->type_0x02], 4, 0);
        if (work->effects[i] == NULL) {
            fn_8011AD00(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    fn_8011A34C(self);
}
