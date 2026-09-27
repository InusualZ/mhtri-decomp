/* ef/eft029.cpp - the `eft029` effect family, `.text` 0x80119DEC..0x8011D448 (37 functions).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: the runtime dump resolves only the two family
 * setters `eft029_set_scale` (0x8011AD84) and `eft029_set_kaihou` (0x8011AFBC); every other address in
 * the range is the dump's placeholder `FUN_`/`zz_XXXXXXXX_`).
 *
 * Registration (docs/plan.md 12).  Class 2 evidence named the TU: the runtime dump's own names
 * `eft029_set_scale` and `eft029_set_kaihou` are defined inside this `.text` range (verified with
 * `dumpmap.py lookup 0x8011AD84` / `0x8011AFBC`), so the module is `ef` and the file is `eft029.cpp` -
 * the scheme of its neighbours `ef/eft001.cpp` ... `ef/eft019.cpp`.  It is C++ (the two family setters
 * arrive mangled - `eft029_set_scale__FP4_PLWUcf`, `eft029_set_kaihou__FP4_PLWUc`), so every definition
 * whose map name is plain (`fn_80119DEC`, ...) is `extern "C"` so its emitted name stays the map's stem
 * and objdiff can pair it (playbook row 42).
 *
 * The handed pooled brief (tools/units/briefs/80119dec-fn-80119dec-e663.md) is STALE: it says
 * `.text` 0x80119DEC..0x8011A34C, 2 functions.  `brief.py --pool` skips a brief that already exists,
 * and the attribution queue was regenerated with TU-bounded ranges (`70c91ab0`, queue mtime 10:22:55)
 * after that brief was pooled; `queue.py next` then copied the stale file at claim time (brief mtime
 * 10:25:38).  The authoritative queue entry `proposal/80119DEC_fn_80119DEC` is 0x80119DEC..0x8011D448,
 * 37 functions, 13916 bytes, and its own TU analysis declined the brief's endpoint: 0x8011A34C is the
 * `jumptable_805A0780 -> jumptable_805A07AC` data-run seam ("a candidate seam inside it was not
 * taken"), not a TU seam.  Registered at the queue's TU range per the owner's decision, so the
 * remainder cannot be proposed a second time under the same `eft029` name.
 *
 * Sections: `.text` 0x80119DEC..0x8011D448, `extab` 0x8000C56C..0x8000C5F4 (17 records) and
 * `extabindex` 0x8002670C..0x800267D8 (17 x 12 B), the runs the bracketing registered units
 * (`ef/fn_80119C44.c` below, `proposal/8011D448` above) leave for this one.  No `.ctors`/`.dtors` word.
 *
 * State (this round): the brief's 2 functions are written -
 *   fn_80119DEC (0x4B4)  per-frame handler: runs the timer down, advances state_0x05, picks the effect
 *                        (id, param) pair from the map area / stage area / movie state, then builds the
 *                        pool and places it by type
 *   fn_8011A2A0 (0xAC)   the create arm: fills the pool with `res_eft_create(idTable[type], 4, 0)`
 * The 35 remaining functions of the TU are NOT written (residuals, in address order):
 *   fn_8011A34C 0x38, fn_8011A384 0x3D8, fn_8011A75C 0x140, fn_8011A89C 0x268, fn_8011AB04 0x1EC,
 *   fn_8011ACF0 0x10, fn_8011AD00 0x4, fn_8011AD04 0x54, fn_8011AD58 0x2C, eft029_set_scale 0x28,
 *   fn_8011ADAC 0x210, eft029_set_kaihou 0x110, fn_8011B0CC 0x30, fn_8011B0FC 0x3C, fn_8011B138 0x48,
 *   fn_8011B180 0x6C, fn_8011B1EC 0x2FC, fn_8011B4E8 0x2B8, fn_8011B7A0 0x60, fn_8011B800 0x4E0,
 *   fn_8011BCE0 0x370, fn_8011C050 0x29C, fn_8011C2EC 0x308, fn_8011C5F4 0x398, fn_8011C98C 0x1E8,
 *   fn_8011CB74 0x10, fn_8011CB84 0x4, fn_8011CB88 0x9C, fn_8011CC24 0xC4, fn_8011CCE8 0x54,
 *   fn_8011CD3C 0x3C, fn_8011CD78 0x14C, fn_8011CEC4 0x570, fn_8011D434 0x10, fn_8011D444 0x4.
 * `fn_8011AD00` and `fn_8011A34C` are two of them but the two written functions call them, so they are
 * declared here.
 *
 * Types.  `_EFT`, `_CP_VECTOR` come from `ef.h`; `VEC3`/`MTX34` from `nw4r/math.h`; `fn_800FBB90` from
 * its owner `ef/eft001.h`; `res_eft_create`/`SetRootMtxTrans`/`cpSetRotMatrix`/`get_now_mapno` from the
 * shared `unsplit/unknown.h`.  The work block is a unit-local `_EFT029_WORK` cast from `_EFT::work_0x38`
 * (the pattern `ef/eft001.cpp` uses): only its `count` (+0x00) and `effects[]` (+0x04) are reached by
 * the two functions written, so its size is a lower bound.
 *
 * Data.  The unit owns no pool section emitted here: the two `u16` tables `lbl_805A06F0` (effect id per
 * type) and `lbl_805A0710` (its parameter) and the jump tables `jumptable_805A0780` / `jumptable_805A0740`
 * are `extern`-declared by their map names and never defined (playbook 23/29).
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft001.h"
#include "unsplit/unknown.h"
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* The unit's own pooled tables (never emitted here). */
extern "C" {
u16 lbl_805A06F0[]; /* effect id per `type_0x02`, .data 0x805A06F0 */
u16 lbl_805A0710[]; /* its parameter pair,      .data 0x805A0710 */
}

/* The unmangled `fn_XXXXXXXX` callees: the range's own placeholder names, and the unsplit
 * `fn_800DD1F0` / `fn_8021F238` (neither address has a registered owner that publishes a header, so
 * both stay declared here - rule 2's named gap). */
extern "C" {
void fn_800DD1F0(u32 id, nw4r::math::VEC3* pos);
u32 fn_8021F238();
void fn_8011AD00(_EFT* self);
void fn_8011A34C(_EFT* self);
}

/* `map_se_req(u8, nw4r::math::VEC3*)` - the map's own mangled spelling (unsplit, no owner header). */
void map_se_req(u8 id, nw4r::math::VEC3* pos);

/* The per-family work block `_EFT::work_0x38` points at.  Only the fields these two functions read are
 * named; the block continues past +0x08 (the TU's other functions use it), so the size is a lower
 * bound.  size: 0x08 (approximation) */
struct _EFT029_WORK {
    /* +0x00 */ s32 count;                        /* the number of pooled effects at +0x04 */
    /* +0x04 */ nw4r::ef::Effect* effects[1];     /* the pool `res_eft_create` fills */
};

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Retail keeps the unfused forms the `-O3` peephole pass folds: `addi`+`cmpwi` for the timer test and
the `clrlwi` narrowing of the effect id.  The peephole pass off matches the target object (the same
per-unit lever eft009 and sound/fn_800DD1F0 use, playbook row 39). */
#pragma peephole off

/* 0x80119DEC - the family's per-frame handler.  Runs `timer_0x0C` down; only the frame it goes
 * negative does anything.  `state_0x05` advances, the (id, param) pair is chosen from the map number /
 * stage area / movie state, the whole pool is created, and the placement stage then dispatches on
 * `type_0x02`.  Any failure destroys the record through `fn_8011AD00`. */
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
        if (fn_8021F238() == 0) {
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

/* 0x8011A2A0 - the create arm: fills the pool with one `res_eft_create(idTable[type], 4, 0)` per slot
 * and, on success, flags the record live and runs the placement dispatcher.  Destroys the record on the
 * first failed create. */
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

#pragma peephole reset
