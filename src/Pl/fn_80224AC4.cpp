/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py - `zz_0224ac4_` at 0x80224AC4 - and config/RMHE08/symbols.txt, where every
 * name between 0x80224AC4 and 0x80229ECC is a bare `fn_` or `dtor_` placeholder with no signature)
 *
 * Pl/fn_80224AC4.cpp - the player actor's per-model SE/motion rig update.
 *
 * `.text` 0x80224AC4-0x80229ECC (0x8408 B, 44 functions), extab 0x80011B34-0x80011C7C and extabindex
 * 0x8002E7B8-0x8002E9A4 (41 records) - all three ranges are registered in splits.txt, and both
 * exception runs agree with the unit's edges exactly.  The `.data` run the range references (the
 * motion jump tables at 0x805BAB20 and the two vtables at 0x805BAB58/0x805BAB74) is *not* claimed:
 * `Pl/fn_8021E1EC.cpp`, the unit on the other side of the same seam, left its own run unclaimed for
 * the same reason (the run leaks into neighbouring units, and defining the symbols costs the target's
 * pool relocations - playbook 23).
 *
 * Module (brief section 2 evidence order):
 *   1. no `__FILE__` string - every `.c`/`.cpp` string in the DOL was extracted with its address and
 *      none falls inside the range's data (the nearest are 0x805A1BB8 `enemy_control.cpp` and
 *      0x805CDFC8 `menu_item.cpp`, i.e. the whole 0x805B... band carries none);
 *   2. no runtime-dump name - `dumpmap.py lookup 0x80224AC4` answers `zz_0224ac4_`;
 *   3. the code is `Pl`: every actor parameter is a `_PLW*` (`Get_motion_no__FP4_PLW`,
 *      `Pl_master_ck__FP4_PLW`, `Pl_act_ck__FP4_PLWUcUs`), the SE work is `_PLW.field_0xAF4`, and
 *      `include/unsplit/Pl.h` already homes this range's tail (`fn_80229CB4`/`fn_80229E10`/
 *      `fn_80229EA8`) in the `Pl` band.  The `lobby` lib next door cannot host it: the target object
 *      carries extab/extabindex while `cflags_lobby` is `-Cpp_exceptions off`, and the range contains
 *      zero record-form instructions - the `-opt nopeephole` fingerprint of `cflags_pl`.
 *   Class 4 for the file name: nothing names it, so it keeps the map's `fn_80224AC4` stem.
 *
 * Language: C++ (high).  `move__6MHcharFUs` (called as a member through `MHchar`), `PlayMode_ck__Fv`
 * and `se_req_frame_set__FP5_se_wllll` are manglings.  The unit's own functions are `extern "C"`
 * because the map spells them unmangled (playbook 42).
 *
 * Flags: exactly `cflags_pl` (`Wii/1.0`, `-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`) - no
 * per-unit deviation, no pragma.
 *
 * Residuals (measured one row at a time with `recompile.py <unit> --measure <symbol>`; the official
 * per-symbol report metric).  14 of the 15 reconstructed functions are byte-identical (100.00 %);
 * 2944 of the range's 21512 `.text` bytes are reconstructed.
 *  - fn_80224AC4 93.72811 % (868 B target, 860 B ours): every instruction is the right one in the right
 *    order and only the allocator's colouring is left - the target keeps its loop bases at `r23`/`r24`
 *    and so saves `r23`-`r31` (`_savegpr_23`), ours saves `r25`-`r31`, and the third loop's element
 *    pointer is folded from `rig` (target: `lwz r4,284(r28)` off `rig`, ours: `lwz r4,280(r28)` off
 *    `rig+4`).  Both are encodings of the same address, so no source shape reaches them from here.
 *  - fn_80226A4C 0x80226A4C (148 B) is written but **not landed**: its whole body is two
 *    `fn_80045330` copies, and that symbol is owned by `src/mh3_pad.cpp`, whose header re-declares
 *    `VEC3_ctor`/`setVec3` with different parameter types than `ef.h` - so including
 *    `include/mh3_pad.h` from a unit that also includes `pl.h` (which pulls `ef.h`) is an illegal
 *    overload.  The fix is a rule-2/rule-1 pass over `include/mh3_pad.h` (one declaration per symbol,
 *    in the owner's header); until then the function is left out rather than declared locally.
 *  - the remaining 28 functions are not reconstructed yet, largest first: fn_802283C8 1584 B,
 *    fn_80227354 1452 B, fn_80226EC4 1168 B, fn_80226728 740 B, fn_8022632C 1020 B, fn_80225734
 *    1024 B, fn_80227D8C 1028 B, fn_80228ED4 1020 B, fn_802289F8 912 B, fn_80225434 768 B,
 *    fn_80225B34 544 B, fn_80227AF0 668 B, fn_80225D54 500 B, fn_802260E4 584 B, fn_80226AE0 400 B,
 *    fn_80227900 496 B, fn_80229698 464 B, fn_802294A4 500 B, fn_80225280 436 B, fn_80229A20 476 B,
 *    fn_80225F48 412 B, fn_8022511C 356 B, fn_802298AC 372 B, fn_80228190 304 B, fn_80228D88 332 B,
 *    fn_802282C0 264 B, fn_80226D90 308 B, fn_80226C70 288 B.  The 0x80225734/0x80225B34/0x80225D54/
 *    0x80225F48/0x802260E4/0x8022632C/0x80226728 family is a template (all seven start with
 *    `lwz r5,316(r3); lfs f0,72(r5); fcmpo` against `lbl_80799CDC` - the `_PLW.physics_0x13C` body
 *    pitch test) and is the cheapest block left; it needs `_PLW_PHYSICS`'s `+0x48` float, which is
 *    `MHchar +0x44` in today's `include/pl.h` (still a `pad_0x44` there) and would have to move out of
 *    `src/ef/fn_80114E34.cpp` first.
 */

#define MHTRI_FN_800E3B3C_TAKES_ACTOR 1

#include "types.h"
#include "pl.h"
#include "Pl/pl_master.h"
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "enemy/fn_80138074.h"
#include "sound/fn_800DD1F0.h"
#include "sound/fn_800D7F54.h"
#include "sound/se_w.h"
#include "unsplit/Pl.h"
#include "Pl/fn_80258FCC.h"
#include "lobby/fn_8021E1EC.h"
#include "unsplit/g3d.h"
#include "unsplit/unknown.h"
#include "sys_mem.h"
#include "Pl/fn_80224AC4.h"
#include "Pl/fn_802693C4.h"

/* One 4-byte row of the table `fn_80229868` scans. size: 0x4 */
typedef struct PlSeRow {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 pad_0x01[0x3];
} PlSeRow; /* size: 0x4 */

/* This unit's own functions, declared in address order so an earlier body can call a later one.  All
 * of them are `extern "C"` because the map spells them unmangled (playbook 42). */
extern "C" {
u32 fn_80224E28(_PLW* self, u32 slot);
f32 fn_802250E4(void);
f32 fn_80225434(PlSeRig* rig, s32 index, u8 id);
u32 fn_80225734(_PLW* self, u16 motion);
u32 fn_80225B34(_PLW* self, u16 motion);
u32 fn_80225D54(_PLW* self, u16 motion);
u32 fn_80225F48(_PLW* self, u16 motion);
u32 fn_802260E4(_PLW* self, u16 motion);
u32 fn_8022632C(_PLW* self, u16 motion);
u32 fn_80226728(_PLW* self, u16 motion);
PlSeVecPair* fn_80226A0C(void* ctx, s32 index, u8 mode);
void fn_80226D90(PlSeRig* rig, PlSeAttach* attach, u8 id);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80224AC4 */

/* Clears the actor's live flag when any gate says the model must not move, then drives every model
 * record the rig carries: gate byte, model helper, placement and the model's own `move`. */
extern "C" void fn_80224AC4(PlSeRig* rig)
{
    u32 flag = rig->plw->field_0x001;
    u32 i;
    u32 v;

    if ((u8)fn_800CF208() == 1) {
        if (fn_8026FD94(rig->plw) == 0) {
            flag = 0;
        }
        if ((u8)PlayMode_ck() == 3) {
            if (fn_8025EFF4(rig->plw) == 1) {
                flag = 0;
            }
        }
    }
    if (rig->plw->field_0x19 > 0) {
        flag = 0;
    }
    if (Pl_master_ck(rig->plw) == 0) {
        if ((u8)fn_800CF208() == 2 && game_ready_ck() == 1 && rig->plw->field_0xB04 == 0) {
            flag = 0;
        }
    }
    if (rig->field_0x9C0[0] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[1] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[2] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[3] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[4] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[5] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[6] == 0) {
        flag = 0;
    }

    if (Pl_master_ck(rig->plw) == 1) {
        v = fn_800E3B3C(rig->plw) | 0x200;
    } else {
        v = 0;
    }

    for (i = 0; i < 7; i++) {
        PlSeModel* m = &rig->models_0x004[i];

        if (m->model.field_0x118 == 0) {
            continue;
        }
        m->model.field_0x34 = flag;
        if (flag != 0) {
            m->model.field_0x34 = rig->field_0x9C0[i];
        }
        fn_80223258(rig->plw, i);
        if (m->model.field_0x34 != 0) {
            fn_80223708(rig, rig->plw->field_0x25C, i);
            if (i == 5) {
                fn_80223830(rig, rig->plw->field_0x258);
            }
        }
        fn_80224820((u32)rig, &m->model, rig->plw, lbl_80799CD8);
        m->model.move((u16)v);
    }

    for (i = 0; i < 3; i++) {
        PlSeModel* m = &rig->sub_0x0A14[i];
        PlSeAttach* a = &rig->attachments_0x1230[i];

        if (m->model.field_0x118 == 0) {
            continue;
        }
        m->model.field_0x34 = flag;
        if (flag != 0) {
            m->model.field_0x34 = rig->field_0x9C7[i];
        }
        if (fn_80224944(rig->plw) == 0) {
            m->model.field_0x34 = 0;
        }
        if (rig->plw->field_0x01E != 0) {
            m->model.field_0x34 = 0;
        }
        if (m->model.field_0x34 != 0) {
            fn_8022375C(rig, i);
        }
        fn_80224820((u32)rig, &m->model, rig->plw, fn_80225434(rig, i, a->field_0x10));
        m->model.move((u16)v);
        fn_80226D90(rig, a, a->field_0x10);
    }

    if (flag != 0) {
        for (i = 0; i < 7; i++) {
            PlSeModel* m = &rig->models_0x004[i];

            if (m->model.field_0x118 != 0 && m->model.field_0x34 != 0) {
                g3d_root_model_bind(pRoot, m->model.field_0x118);
            }
        }
        for (i = 0; i < 3; i++) {
            PlSeModel* m = &rig->sub_0x0A14[i];

            if (m->model.field_0x118 != 0 && m->model.field_0x34 != 0) {
                g3d_root_model_bind(pRoot, m->model.field_0x118);
            }
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80224E28 */

/* Maps one of the actor's motion slots onto the per-slot decision table, then lets the motion number
 * override it: 0 = blocked, 1 = allowed, 2 = the model's own answer. */
extern "C" u32 fn_80224E28(_PLW* self, u32 slot)
{
    u16 motion = Get_motion_no(self);
    u32 flag = 2;

    switch (slot) {
    case 0:
        flag = fn_80225734(self, motion);
        break;
    case 1:
        flag = fn_80225D54(self, motion);
        break;
    case 3:
        flag = fn_80225F48(self, motion);
        break;
    case 4:
        flag = fn_802260E4(self, motion);
        break;
    case 7:
        flag = fn_8022632C(self, motion);
        break;
    case 2:
        flag = fn_80225B34(self, motion);
        break;
    case 8:
        flag = fn_80226728(self, motion);
        break;
    default:
        if (fn_8026A3A0(self) != 0) {
            flag = 1;
        }
        break;
    }

    switch (motion) {
    case 202:
    case 204:
        if (self->field_0x18 != 0) {
            flag = 1;
        } else {
            flag = 2;
        }
        break;
    case 205:
    case 206:
    case 208:
    case 209:
    case 211:
    case 212:
    case 213:
    case 214:
    case 215:
    case 219:
    case 220:
    case 253:
    case 255:
    case 263:
    case 264:
    case 271:
        flag = 2;
        break;
    case 313:
    case 356:
        if (fn_8026A328(self, 1, lbl_80799CE0, lbl_80799CDC) == 1) {
            flag = 2;
        } else if (fn_8026A328(self, 1, lbl_80799CE4, lbl_80799CDC) == 1) {
            flag = 1;
        } else {
            flag = 2;
        }
        break;
    case 60:
    case 61:
    case 135:
    case 136:
    case 421:
    case 422:
    case 500:
    case 501:
    case 502:
    case 503:
    case 504:
    case 505:
    case 506:
    case 507:
    case 508:
    case 509:
    case 510:
    case 511:
    case 512:
    case 513:
    case 514:
    case 515:
    case 516:
    case 517:
    case 518:
    case 520:
    case 521:
    case 522:
    case 523:
    case 524:
    case 525:
    case 526:
    case 527:
    case 528:
    case 529:
    case 530:
    case 531:
    case 532:
    case 533:
    case 534:
        flag = 2;
        break;
    default:
        break;
    }

    return flag;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802250E4 */

/* Reads the current SE bank index, then returns that bank's gain from the `.sdata` gain table. */
extern "C" f32 fn_802250E4(void)
{
    return lbl_80792010[lbl_805BAAD4[fn_8027EE24()]];
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80226A0C */

/* Returns the 24-byte vector pair the motion layer uses for `mode`: 0 indexes the per-index table,
 * 1 is the single shared pair, anything else is null. */
extern "C" PlSeVecPair* fn_80226A0C(void* ctx, s32 index, u8 mode)
{
    switch (mode) {
    case 0:
        return &lbl_805BB028[index];
    case 1:
        return lbl_805BB130;
    default:
        return NULL;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802292D0 */

/* Returns the animation key frame at the motion layer's current frame. */
extern "C" f32 fn_802292D0(_PLW* self, f32* out)
{
    return getKeyData(out, fn_8026A34C(self));
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229304 */

/* Reads three key-frame channels at the motion layer's current frame. */
extern "C" f32 fn_80229304(_PLW* self, f32* a, f32* b, f32* c)
{
    return fn_80052370(a, b, c, fn_8026A34C(self));
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229358 */

/* Reads four key-frame channels at the motion layer's current frame. */
extern "C" void fn_80229358(_PLW* self, f32* a, f32* b, f32* c, f32* d)
{
    getKeyData3(a, fn_8026A34C(self), b, c, d);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802293BC */

/* Dispatches one track update by its mode: 1 and 2 also run the second-stage update, 4 only the
 * first. */
extern "C" void fn_802293BC(void* self, MHchar* track, s32 mode, u32 arg)
{
    switch (mode) {
    case 1:
    case 2:
        fn_80080B10(track, mode);
        fn_800E3264(track, arg);
        break;
    case 4:
        fn_80080B10(track, mode);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229430 */

/* 0x802293BC's twin: the same dispatch for the second track family. */
extern "C" void fn_80229430(void* self, MHchar* track, s32 mode, u32 arg)
{
    switch (mode) {
    case 1:
    case 2:
        fn_80080B10(track, mode);
        fn_800E3264(track, arg);
        break;
    case 4:
        fn_80080B10(track, mode);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229868 */

/* Finds the first entry of `rows` after `index` whose gate byte is set and hands it to the track
 * update; stops at the table's seventh entry. */
extern "C" void fn_80229868(u32 index, PlSeRow* rows, void* arg)
{
    u32 i;

    for (i = index + 1; i < 7; i++) {
        if (rows[i].field_0x00 != 0) {
            fn_801394C0(arg, i);
            return;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229CB4 */

/* Picks the four SE frame codes out of the actor's equipment decorations and stores them on the
 * actor's SE work, collapsing duplicates to zero. */
extern "C" void fn_80229CB4(_PLW* self)
{
    s32 n0 = fn_8027EBA8(self, &self->equipA[3]);
    s32 n1 = fn_8027EBA8(self, &self->equipA[2]);
    s32 n2 = fn_8027EBA8(self, &self->equipA[0]);
    s32 n3 = fn_8027EBA8(self, &self->equipA[1]);
    u8 v0;
    u8 v1;
    u8 v2;
    u8 v3;

    if (n0 > 0) {
        v0 = lbl_807913D0[self->se_name_set][n0];
    } else {
        v0 = 0;
    }
    if (n1 > 0) {
        v1 = lbl_807913C8[self->se_name_set][n1];
    } else {
        v1 = 0;
    }
    if (n2 > 0) {
        v2 = lbl_807913C0[self->se_name_set][n2];
    } else {
        v2 = 0;
    }
    if (n3 > 0) {
        v3 = lbl_807913D8[self->se_name_set][n3];
    } else {
        v3 = 0;
    }

    if (v0 == v1) {
        v1 = 0;
    }
    if (v0 == v2) {
        v2 = 0;
    }
    if (v1 == v2) {
        v2 = 0;
    }

    self->field_0xAF4->field_0x0A3D = v0;
    self->field_0xAF4->field_0x0A3E = v1;
    self->field_0xAF4->field_0x0A3F = v2;
    self->field_0xAF4->field_0x0A40 = v3;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229E10 */

/* Arms one SE frame request for each of the actor's three frame codes. */
extern "C" void fn_80229E10(_se_w* work, s32 code, s32 val)
{
    if (work->field_0x0A3D != 0) {
        se_req_frame_set(work, code, work->field_0x0A3D, 2, val | 0x1000000);
    }
    if (work->field_0x0A3E != 0) {
        se_req_frame_set(work, code, work->field_0x0A3E, 2, val | 0x1000000);
    }
    if (work->field_0x0A3F != 0) {
        se_req_frame_set(work, code, work->field_0x0A3F, 2, val | 0x1000000);
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229EA8 */

/* Arms the actor's fourth SE frame request. */
extern "C" void fn_80229EA8(_se_w* work, s32 code, s32 x, s32 y)
{
    u32 v = work->field_0x0A40;

    if (v == 0) {
        return;
    }
    se_req_frame_set(work, code, v, x, y | 0x1000000);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229BFC / 0x80229C58 */

/* The first base class's deleting destructor; `mode` > 0 asks for the storage back. */
extern "C" void* dtor_80229BFC(void* self, s16 mode)
{
    if (self != NULL) {
        fn_800810DC(self, 0);
        if (mode > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* The second base class's deleting destructor. */
extern "C" void* dtor_80229C58(void* self, s16 mode)
{
    if (self != NULL) {
        fn_800810DC(self, 0);
        if (mode > 0) {
            operator delete(self);
        }
    }
    return self;
}
