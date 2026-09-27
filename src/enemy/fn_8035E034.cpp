/* enemy/fn_8035E034.cpp - the em033/em035 enemy-program band, `.text` 0x8035E034..0x8035F2B4
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address here resolves to a `zz_XXXXXXXX_` dump
 * name and a bare `.text` entry in config/RMHE08/symbols.txt, so no real function name survives).
 *
 * WHAT IT IS.  The enemy-side program handlers of the em033 monster (0x8035E034..0x8035F060) and the
 * head of em035 (0x8035F060..0x8035F2B4).  Every function takes the shared `_ENEMY_WORK` record and
 * drives its action/id state; the `.data` program tables `em033_prog_tbl` (0x805ED370) and
 * `em035_prog_tbl` (0x805ED838) list this range's entry points (`fn_8035E034`/`fn_8035E578`/
 * `fn_8035E1E0` in em033, `fn_8035F060`/`fn_8035F174`/`fn_8035F178` in em035), and `lbl_805ED808`
 * mixes `fn_8035F004`/`fn_8035EF50`/`fn_8035EF58` with the registered enemy-band functions
 * `fn_801394D4`..`fn_8013A650`.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable from the
 * range: every `lis`/`addi` pair and every `lbl_` reference in the 16 objects resolves to the
 * `.sdata2` float pool (0x8079B704..0x8079B718) or to a call, never to a source-file-name literal.
 * 2. `dumpmap.py lookup` gives only `zz_XXXXXXXX_` placeholders.  3. The `em033_prog_tbl` /
 * `em035_prog_tbl` names and the `_ENEMY_WORK` record place the unit in the `enemy` module (the
 * program tables sit in the `.data` auto object with the enemy ones, and the range's records are the
 * enemy work block).  The file therefore keeps the map's `fn_8035E034` stem (brief option 4); no
 * name was invented and no module was guessed - the surrounding proposals (0x80358624, 0x8035F2B4)
 * are menu screens, but they are unregistered proposals, not a naming scheme.
 *
 * SEAM.  The brief's range is one `attribute.py` `--max-bytes` run.  `tudiscover.py at 0x8035E034`
 * puts a strong seam at 0x8035F060 (`.sdata2` run jump lbl_8079B710 -> lbl_8079B718) and reports the
 * 2-function match set 0x8035E034..0x8035E578 with the 13-function extension to 0x8035F060; the
 * `em035_prog_tbl` starts at 0x8035F060, so the true boundary is 0x8035F060 (em033|em035), not the
 * range's cap edge.  This file is registered for the whole brief range and both programs are written;
 * a re-cut at 0x8035F060 is a `config_requests` follow-up.
 *
 * LANGUAGE AND SECTIONS.  C++: the map's `mangled_undefined` set is `findInterSection__F...`,
 * `get_now_areano__Fv`, `get_now_mapno__Fv`, `ran_suu__Fl`, `__dl__FPv`, all C++-linkage
 * declarations at global scope.  Every plain `fn_` definition is `extern "C"` so it keeps the map's
 * name (playbook 42); the mangled callees are called through their real signatures (rule 9).  The
 * lib is `enemy` (`cflags_main`, `-Cpp_exceptions on`), and the object carries extab/extabindex
 * (13 records) plus the `.sdata2` pool.
 *
 * STATUS / RESIDUALS (measured with `recompile.py --measure`, official report metric).  12 of the 16
 * bodies are at or above the 80 % bar - `fn_8035E578`, `fn_8035EF50`, `fn_8035F174` are byte-identical;
 * `fn_8035F004` 95.4, `fn_8035EAA0` 95.2, `fn_8035E034` 94.5, `fn_8035E81C` 86.5, `fn_8035F178` 85.8,
 * `fn_8035F060` 85.4, `fn_8035E984` 83.4, `fn_8035EE40` 81.3, `fn_8035EB80` 80.3.  Four are short and are
 * the honest residual:
 *   * `fn_8035E1E0` 79.1 % - the outer switch's case bodies each end in `fn_8012B380(self,5,2,10)` in
 *     retail (920 B); our build shares that tail (756 B), so the diff is switch-body duplication.
 *   * `fn_8035E580` 75.6 % - the roster scan.  Retail's `prev` byte is read before any store
 *     (uninitialised in the object); ours starts it at 0, and the `findInterSection` call's stack
 *     vector is reconstructed, so the loop colours differently (676 vs 668 B).
 *   * `fn_8035ECD8` 78.8 % - retail keeps `(self->act_id & 0xF)` (`clrlwi r0,r4,28`) in every case
 *     body; under our `(u8)(act_id-1)<=3` guard MWCC proves the range and folds the mask, so each id
 *     build is four instructions short (320 vs 360 B).
 *   * `fn_8035EF58` 52.1 % - the branch tree is right; the prologue's callee-saved set and the
 *     `p = *(a+4)` pointer local colour differently (180 vs 172 B), and objdiff reports the whole
 *     straight-line region as a replace.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "fn_8004CAD8.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "ef/fn_800CDB2C.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* ---------------------------------------------------------------------------------------------- *
 * Callees outside this unit.
 * ---------------------------------------------------------------------------------------------- */

/* The three vector helpers the game-root draw layer owns; `include/mh3_pad.h` and `include/ef.h`
 * now spell them with the same record type (`nw4r::math::VEC3*`, docs/plan.md 6.5 rule 11), so no
 * local copy of the declaration is needed. */
void fn_8008E8D0(void* a, void* b);
void fn_8008DA10(void* a, void* b);
void fn_800513F0(VEC3* v, f32 s);

/* The enemy action helpers whose owner units are unclaimed; their signatures are the call sites'
 * (this range's bracketing registered units name different bands, the rule 2 named gap). */
void fn_8012B380(_ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_8012CEB4(_ENEMY_WORK* self, s16 a, u32 b);

/* The monster-roster helpers at 0x803BDF0C.. (the run 0x803B465C..0x803BE30C, unclaimed).  They
 * index a global 0x224-byte-stride record array; `fn_803BDFF4` returns the record's +0x1D0 aim
 * position, `fn_803BDECC` the record itself. */
struct EmRosterRec {
    /* +0x000 */ u8 pad_0x000[0x1F0];
    /* +0x1F0 */ f32 field_0x1F0;   /* the approach radius `fn_8035EB80` compares against */
}; /* size: 0x1F4 */

/* The object the aim writer's first argument points at, and the source block its +0x4 member names
 * (`fn_8035EF58`).  Only the two fields the body touches are named. */
struct EmAimOwner {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ u8* field_0x004;
}; /* size: 0x8 */
struct EmAimSource {
    /* +0x000 */ u8 pad_0x000[0xA];
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0x00B[0x348 - 0xB];
    /* +0x348 */ nw4r::math::VEC3 field_0x348;
}; /* size: 0x354 */
u8 fn_803BDF0C(u8 area, u8* out, u32 max);
s32 fn_803BE08C(u8 id, u8 area);
VEC3* fn_803BDFF4(u8 id, u8 area);
EmRosterRec* fn_803BDECC(u8 id);

u32 fn_802B0998(u8 index);

/* 0x80295924 - the segment/line intersection test the aiming helpers use; its map name is the C++
 * mangling of exactly this signature (rule 9), so it is declared at C++ scope. */
int findInterSection(VEC3* a, VEC3* b, VEC3* c, u8 d, u32 e, u8 f, u16 g, u8* h);

/* The runtime's scalar deleter; `__dl__FPv` is its mangling. */
void operator delete(void* ptr) throw();

/* This unit's own forward declarations (definitions follow in address order). */
extern "C" void fn_8035EE40(_ENEMY_WORK* self, u8 a, VEC3* out);
extern "C" u8 fn_8035E580(_ENEMY_WORK* self, u8 a);
extern "C" u8 fn_8035E984(u8 a, u8 b);

/* Pool literals (declared, never defined - playbook 29). */
extern "C" f32 lbl_8079B704;
extern "C" f32 lbl_8079B708;
extern "C" f32 lbl_8079B70C;
extern "C" f32 lbl_8079B710;
extern "C" f32 lbl_8079B718;

/* The shared-state accessors. */
u8 get_now_areano();
u8 get_now_mapno();

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E034 - em033's distance/id query.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E034(_ENEMY_WORK* self, u8 mode)
{
    VEC3 a;
    VEC3 b;
    VEC3 c;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    switch (mode) {
    case 0:
        return self->field_0x1E4;
    case 1:
        return self->field_0x33E;
    case 2:
        fn_8035EE40(self, 0, &a);
        fn_80050CA0(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return fn_80050F24((const f32*)&b) <= lbl_8079B704;
    case 3:
        fn_8035EE40(self, 1, &a);
        fn_80050CA0(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return fn_80050F24((const f32*)&b) <= lbl_8079B708;
    case 4:
        return self->field_0x33F;
    case 5:
        fn_8035EE40(self, 2, &a);
        fn_80050CA0(&c, &a, &self->pos);
        copyVec3(&b, &c);
        return fn_80050F24((const f32*)&b) <= lbl_8079B70C;
    case 6:
        return fn_803BDFF4(self->field_0x33B, self->act_id) != 0;
    default:
        return 0;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E1E0 - em033's action/out-pair selector.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035E1E0(_ENEMY_WORK* self, u8* out_a, u8* out_b)
{
    switch ((u8)fn_802B0668(self->field_0x1E0)) {
    case 1:
        switch (self->act_id) {
        case 1:
        case 2:
        case 3:
        case 4:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 2:
        switch (self->act_id) {
        case 4:
            *out_a = 0xC;
            *out_b = 0;
            if (fn_802B0998(3) == 1) {
                fn_80126324(self, 1, 3, lbl_8079B70C);
            } else {
                fn_80126324(self, 0, 2, lbl_8079B70C);
            }
            break;
        case 6:
            *out_a = 0xC;
            *out_b = 2;
            fn_80126278(self, (u16)((self->act_id & 0xF) << 8), &self->pos);
            self->pos_0x1BC.y = 0x6C00;
            break;
        case 9:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 3:
        switch (self->act_id) {
        case 5:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        case 6:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        case 9:
            *out_a = 0xC;
            *out_b = 0;
            fn_80126324(self, 0, 2, lbl_8079B70C);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            fn_8012B380(self, 5, 2, 10);
            break;
        }
        break;
    case 5:
        *out_a = 0xC;
        *out_b = 0;
        if (self->act_id == 2) {
            fn_80126324(self, 0, 2, lbl_8079B70C);
        } else {
            fn_8012B380(self, 5, 2, 10);
        }
        break;
    default:
        *out_a = 0xC;
        *out_b = 0;
        fn_8012B380(self, 5, 2, 10);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E578 - a constant predicate.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E578(void)
{
    return 1;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E580 - the em033 roster scan (select the nearest/valid monster record).
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E580(_ENEMY_WORK* self, u8 a)
{
    VEC3 v1;
    VEC3 v2;
    u8 buf[0x10];
    u8 best = 0xFF;
    u8 prev = 0;
    u8 n;
    int i;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    n = (u8)fn_803BDF0C(self->act_id, buf, 0x10);
    if (n > 0x10) {
        return 0xFF;
    }
    for (i = 0; i < n; i++) {
        u8 id = buf[i];
        s32 type = (s8)fn_803BE08C(id, self->act_id);
        if (a == 0) {
            if (type == 5 || type == 1 || type == 0xF) {
                VEC3* p = fn_803BDFF4(id, self->act_id);
                if (p != 0) {
                    u16 ang = fn_80127E78(self);
                    int ok = 1;
                    if (findInterSection(&self->pos, p, &v1, 1, 0xFFFF, self->act_id, ang, 0) > 0) {
                        VEC3 v3;
                        fn_80050CA0(&v3, &v1, p);
                        copyVec3(&v2, &v3);
                        if (fn_80050F24((const f32*)&v2) > fn_803BDECC(id)->field_0x1F0) {
                            ok = 0;
                        }
                    }
                    if (ok) {
                        if (best == 0xFF) {
                            best = id;
                        } else if (type == 5) {
                            best = id;
                            prev = (u8)type;
                        } else if (type == 1 && prev != 5 && prev != 1) {
                            best = id;
                            prev = (u8)type;
                        }
                        if (type == 5) {
                            return best;
                        }
                    }
                }
            }
        } else {
            switch ((u8)fn_802B0668(get_now_mapno())) {
            case 1:
                if ((u8)(self->act_id - 1) > 3) {
                    return 0xFF;
                }
                break;
            case 2:
                if (self->act_id != 6 && self->act_id != 9) {
                    return 0xFF;
                }
                break;
            case 3:
                if ((u8)(self->act_id - 5) > 1 && self->act_id != 9) {
                    return 0xFF;
                }
                break;
            case 5:
                if (self->act_id != 2) {
                    return 0xFF;
                }
                break;
            default:
                return 0xFF;
            }
            if (type == 4 || type == 0 || type == 0xE) {
                if (fn_803BDFF4(id, self->act_id) != 0) {
                    return id;
                }
            }
        }
    }
    return best;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E81C - em033's map/area gate over the roster.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E81C(u8 a)
{
    u8 buf[0x10];
    u8 n;
    int i;

    switch ((u8)fn_802B0668(get_now_mapno())) {
    case 1:
        if ((u8)(get_now_areano() - 1) > 3) {
            return 0;
        }
        break;
    case 2:
        if (get_now_areano() != 6 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 3:
        if ((u8)(get_now_areano() - 5) > 1 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 5:
        if (get_now_areano() != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    n = (u8)fn_803BDF0C(a, buf, 0x10);
    if (n > 0x10) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        s32 type = (s8)fn_803BE08C(buf[i], a);
        if (type == 5 || type == 1 || type == 0xF) {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035E984 - the same gate for one record id.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 fn_8035E984(u8 a, u8 b)
{
    s32 type;

    switch ((u8)fn_802B0668(get_now_mapno())) {
    case 1:
        if ((u8)(get_now_areano() - 1) > 3) {
            return 0;
        }
        break;
    case 2:
        if (get_now_areano() != 6 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 3:
        if ((u8)(get_now_areano() - 5) > 1 && get_now_areano() != 9) {
            return 0;
        }
        break;
    case 5:
        if (get_now_areano() != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    type = (s8)fn_803BE08C(a, b);
    if (type == 4 || type == 0 || type == 0xE) {
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EAA0 - lock the current target in, or pick a random action.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EAA0(_ENEMY_WORK* self, u8 a)
{
    u8 id = fn_8035E580(self, 0);

    if (id != 0xFF) {
        self->field_0x33B = id;
        copyVec3(&self->target, fn_803BDFF4(id, self->act_id));
        fn_8012B380(self, 6, 0xFF, 0);
        fn_8013072C(self, 5, 1);
    } else {
        self->field_0x33B = 0xFF;
        if (a == 1) {
            fn_8012CEB4(self, (s16)(ran_suu(0) & 0xF), 0);
            fn_8013072C(self, 0, 0);
        } else {
            fn_8013072C(self, 5, 0);
        }
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EB80 - keep tracking the cached record, or drop it.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EB80(_ENEMY_WORK* self)
{
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    if (fn_8035E984(self->field_0x33B, self->act_id) == 1) {
        VEC3* p = fn_803BDFF4(self->field_0x33B, self->act_id);
        u16 ang = fn_80127E78(self);
        int keep = 1;
        if (findInterSection(&self->pos, p, &v1, 1, 0xFFFF, self->act_id, ang, 0) > 0) {
            fn_80050CA0(&v3, &v1, p);
            copyVec3(&v2, &v3);
            if (fn_80050F24((const f32*)&v2) > fn_803BDECC(self->field_0x33B)->field_0x1F0) {
                keep = 0;
            }
        }
        if (keep) {
            copyVec3(&self->target, p);
            fn_8012B380(self, 6, 0xFF, 0);
            self->field_0x33F = 2;
        }
    } else {
        self->field_0x33B = 0xFF;
        fn_8012CEB4(self, (s16)(ran_suu(0) & 0xF), 0);
        self->field_0x33F = 0;
        fn_8013072C(self, 0, 0);
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035ECD8 - sync the aim vector for the current action.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035ECD8(_ENEMY_WORK* self)
{
    switch ((u8)fn_802B0668(self->field_0x1E0)) {
    case 1:
        if ((u8)(self->act_id - 1) <= 3) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    case 2:
        switch (self->act_id) {
        case 4:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        case 6:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        case 9:
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
            break;
        default:
            break;
        }
        break;
    case 3:
        if ((u8)(self->act_id - 5) <= 1 || self->act_id == 9) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    case 5:
        if (self->act_id == 2) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 2), &self->aim);
        }
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EE40 - the small per-field aim dispatcher.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EE40(_ENEMY_WORK* self, u8 a, VEC3* out)
{
    switch (a) {
    case 0:
        if ((u8)fn_802B0668(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 7), out);
        }
        break;
    case 1:
        if ((u8)fn_802B0668(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 8), out);
        }
        break;
    case 2:
        if ((u8)fn_802B0668(self->field_0x1E0) == 3 && self->act_id == 6) {
            fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | 5), out);
        }
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EF50 - a one-line forwarder.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EF50(_ENEMY_WORK* self)
{
    fn_8013A654(self, 1);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035EF58 - the effect/target aim writer.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035EF58(void* a, void* b, void* c, void* d, u32 e, u8* f)
{
    VEC3 v;
    struct EmAimSource* p = (struct EmAimSource*)((struct EmAimOwner*)a)->field_0x004;

    VEC3_ctor(&v);
    if (f[4] == 0xFF) {
        if (e - 5 <= 1) {
            if (p->field_0x00A == 1) {
                fn_8008DA10(b, &v);
                fn_800513F0(&v, lbl_8079B710);
                fn_8008E8D0(b, &v);
            }
        } else if (e == 0x17) {
            fn_8008E8D0(b, &p->field_0x348);
        }
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F004 - the em033 record's release hook.
 * ---------------------------------------------------------------------------------------------- */
extern "C" _ENEMY_WORK* fn_8035F004(_ENEMY_WORK* p, u16 a)
{
    if (p != 0) {
        fn_8013918C(p, 0);
        if ((s16)a > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F060 - the em035 run-state constructor.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035F060(_ENEMY_WORK* self, u8 a)
{
    self->init_0x320.field_0x328 = 0x708;
    self->init_0x320.field_0x32A = 0xB4;
    self->bytes_0x32C.field_0x32C = 0;
    self->bytes_0x32C.field_0x32D = 0;
    switch (a) {
    case 2:
        if (self->field_0x00A == 2) {
            fn_80130248(self);
            fn_801305C4(self);
            fn_80128A8C(self, 3, 0);
        } else {
            fn_80128A8C(self, 1, 0);
        }
        self->field_0x1CC = lbl_8079B718;
        if (self->run_flags_0xB12 & 1) {
            copyVec3(&self->pos, (VEC3*)&self->run_0xB04[0]);
            self->pos_0x1BC.y = self->run_angle_0xB10;
        } else {
            self->run_angle_0xB10 = (u16)self->pos_0x1BC.y;
            copyVec3((VEC3*)&self->run_0xB04[0], &self->pos);
        }
        fn_80133BC0(self);
        break;
    case 0:
        if (self->field_0x00A != 0) {
            copyVec3(&self->pos, (VEC3*)&self->run_0xB04[0]);
            self->pos_0x1BC.y = self->run_angle_0xB10;
        }
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F174 - an empty virtual slot.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035F174(void)
{
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035F178 - em035's effect-summon hook.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void fn_8035F178(_ENEMY_WORK* self, u8 a, u8 b)
{
    u32 s[3];

    switch (a) {
    case 1:
        if (b == 1) {
            s[0] = 0;
            s[1] = (u32)((ran_suu(0) & 0xFF) << 8);
            s[2] = 0;
            fn_80141B88(self->field_0x01A, 0x1A, 4, self->act_id, 0xFF, 1, 0x20, 1,
                        0xFF, (void*)&self->pos, (s32)s);
            self->init_0x320.field_0x32A = 0xF0;
        }
        break;
    case 3:
        if (b == 1) {
            s[0] = 0;
            s[1] = (u32)((ran_suu(0) & 0xFF) << 8);
            s[2] = 0;
            fn_80141B88(self->field_0x01A, 0x1A, 5, self->act_id, 0xFF, 1, 0x20, 1,
                        0xFF, (void*)&self->pos, (s32)s);
            self->init_0x320.field_0x32A = 0xF0;
        }
        break;
    default:
        break;
    }
}
