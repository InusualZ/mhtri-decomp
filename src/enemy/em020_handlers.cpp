/* enemy/em020_handlers.cpp - three handler entries of the em020 enemy program, `.text`
 * 0x80375084..0x80375424 (928 B): `em020_model_refresh` 0x20C (0x80375084), `em020_condition_ck`
 * 0x110 (0x80375290) and `em020_area_model_set` 0x84 (0x803753A0).  Registered once, at its final
 * home (docs/plan.md 12), from proposal/80375084_fn_80375084.cpp.
 *
 * WHAT IT IS.  All three take the shared `_ENEMY_WORK` record in r3 and each is one entry of the
 * `.data` program table `em020_prog_tbl` (0x805EE098, 0x70 B, the map's own global name): the table
 * lists `em020_model_refresh` at its +0x20, `em020_condition_ck` at +0x24 and `em020_area_model_set`
 * at +0x34, next to the em020 band's other entry points (0x8036E2BC, 0x8036E320, 0x8036E6B8,
 * 0x80372D58, 0x8036E570, 0x8036E574, 0x803733BC and, below this range, 0x80375424 at +0x3C and
 * 0x80375494 at +0x58).  `em020_model_refresh` refreshes the model's two material effect matrices and
 * pushes the record's K-colours into the `MHchar` material; `em020_condition_ck` answers the program's
 * per-mode condition query; `em020_area_model_set` switches the stage/model state when the area
 * changes.  Module `enemy` (brief section 2, class 3): the record is `_ENEMY_WORK` (every callee is
 * the band's `em_*` API and `include/enemy/ENEMY_WORK.h` is the record's home), the bracketing
 * registered units are `enemy/*`, and the program table is an enemy-program table like
 * `em035_prog_tbl`, whose handlers the registered `enemy/em035_prog.cpp` reconstructs.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string reaches the range: its
 * only data references are the `.sdata2` float pool below and `em020_prog_tbl`, which no `s_`-prefixed
 * dump symbol covers (the .data scan of 0x80500000..0x80600000 finds no file-name literal between
 * menu_note.cpp at 0x805E91F8 and the end of the section).  2. `dumpmap.py lookup` answers only
 * `zz_0375084_`/`zz_0375290_`/`zz_03753a0_`.  3. `em020_prog_tbl` names the subsystem, so the file is
 * `em020_handlers` - the em020 program's handlers - and each symbol is derived from its own body:
 *   * `em020_model_refresh` - refreshes the model's effect matrices and K-colours (GUESS, from the
 *     body: `GetEffectMtx`/`SetEffectMtx` on the slots 8/9 material access and
 *     `MHchar::setTevKColor` on the 6/3 and 0..3 slots);
 *   * `em020_condition_ck` - the per-mode condition query (GUESS; the module's own `em*_ck` scheme,
 *     `em030_condition_ck`'s sibling precedent: the argument selects one of four conditions and the
 *     result is a 0..4 level);
 *   * `em020_area_model_set` - sets the stage/model state for the current area (GUESS, from the body:
 *     the `fn_802B0A98`/`fn_802D94C4` pair under an area switch).
 *
 * SEAM - UNPROVEN, AND A RE-DRAW CANDIDATE.  This is the run `attribute.py` left between the
 * neighbouring proposals; the extab/extabindex runs around it are contiguous and carry no seam
 * (`em020_prog_tbl` lists entry points on both sides of the range: 0x803733BC at +0x18 above,
 * 0x80375424 at +0x3C below).  If `em020_prog_tbl` is one translation unit's table, the real TU
 * spans 0x8036E2BC..0x80375494 and this range is its middle third - a seam re-draw for the round that
 * registers those neighbouring proposals, not something this registration can settle.
 *
 * FLAGS.  The lib's `cflags_main` plus a file-wide `#pragma peephole off` (the sibling
 * `enemy/em035_prog.cpp` precedent in this band): the target keeps the *unfused* form of three folds
 * the pass makes - `clrlwi r0,r4,24` + `cmpwi r0,0` for the mode switch (the pass emits the
 * record-form `clrlwi.` and drops the compare), `slwi r0,r0,7` + `clrlwi r3,r0,16` for the shake
 * angle (`clrlslwi`), and `srwi r0,r0,5` + `clrlwi r3,r0,24` for mode 1's bool (`extrwi`).  Measured
 * with the real command line, one function at a time: pass on 97.57327 (98.75572 / 94.117645 /
 * 100.0), pass off 100.0 on all three.  The range's three framed functions also carry the three
 * 8-byte extab records `-Cpp_exceptions on` emits (0x80017A5C `08 0A`, 0x80017A64 `00 0A`, 0x80017A6C
 * `08 08` - the frame descriptions of the 0x20C/0x110/0x84 bodies, r31-only / LR-only / r31-only).
 * No `.data`/`.sdata` run is claimed: the target objects for this range own `.text`, `extab` and
 * `extabindex` only, and `em020_prog_tbl` is a separate data unit's.
 *
 * RESULT (2026-09-27).  All three functions and both data sections are byte-identical to the target:
 * `.text` 0x3A0, extab 0x18 and extabindex 0x24 at 100 %, unit 100.0 fuzzy / 100 % matched code and
 * data, and `datagap.py --unit` reports no gap row.  Two shared-file fixes rode this unit:
 *   * `include/enemy/fn_8012BDF4.h` - the `s32 em_act_ck(...)` declaration sat inside the
 *     `extern "C"` block, so a caller including that header emitted the unmangled `em_act_ck` where
 *     the map (and the retail objects) reference `em_act_ck__FP11_ENEMY_WORKUcUc`; the first
 *     declaration of a name fixes its language linkage.  Removed, so the C++-scope declaration at the
 *     bottom of that header is the first (three other units' objects had the same wrong reloc name -
 *     no score moved, but their objects are now linkable);
 *   * `include/Pl/fn_8027D684.h` - `fn_8027DC64` declared `u32` here, not the owner's `s32`: the
 *     target's caller compares it unsigned (`cmplwi r3,0x1`), which is what mode 2 needs.
 *
 * rule 7 deferred: references only to other units' unrenamed `fn_XXXXXXXX` symbols (`MTX34_ctor`,
 * `fn_8005024C`, `fn_800E2994`, `fn_8006F304`, `fn_8013A9F4`, `fn_802B0668`, `fn_802B0A98`,
 * `fn_802D94C4`, `fn_8027DC64`), each declared by its
 * owner's header below; checked with `grep -n "fn_" include/fn_8004CAD8.h include/unsplit/{g3d,sound,unknown}.h
 * include/enemy/fn_80138074.h include/stage/stg_w.h include/ai/fn_802D44F4.h include/Pl/fn_8027D684.h`.
 */

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck (the C++ declaration, which mangles to the map's name) */
#include "enemy/fn_80138074.h" /* fn_8013A9F4, the owner's own declaration */
#include "sound/mhchar.h"
#include "nw4r/g3d/scnmdl.h"     /* ScnMdl::CopiedMatAccess */
#include "nw4r/g3d/g3d_resmat.h" /* ResTexSrt */
#include "fn_8004CAD8.h"         /* MTX34_ctor, fn_8005024C (their owner's header) */
#include "unsplit/g3d.h"         /* fn_8006F304 */
#include "unsplit/sound.h"       /* fn_800E2994 */
#include "unsplit/unknown.h"     /* SystemWork / system_w, fn_802B0668 */
#include "enemy/em020_ai.h"      /* em020_aim_target_ck (its owner is enemy/em020_ai.cpp) */
#include "stage/stg_w.h"         /* fn_802B0A98 (the owner is stage/stg_w.cpp) */
#include "ai/fn_802D44F4.h"      /* fn_802D94C4 (the owner is ai/fn_802D44F4.cpp) */
#include "Pl/fn_8027D684.h"      /* fn_8027DC64 (the owner is Pl/fn_8027D684.cpp) */

/* The `.sdata2` floats this range reads (bare marker symbols in the map; the values are below).
 * Declared, never defined here: the pool belongs to the data pass (playbook 29/58). */
extern "C" const f32 lbl_8079B8CC; /* 0.5f   - the shake amplitude */
extern "C" const f32 lbl_8079BC64; /* 0.0025f - the per-frame step of the wrap test */
extern "C" const f32 lbl_8079B848; /* 1.0f   - the wrap bound */
extern "C" const f32 lbl_8079B858; /* 1400.0f */
extern "C" const f32 lbl_8079B870; /* 400.0f */
extern "C" const f32 lbl_8079B8D0; /* -1400.0f */
extern "C" const f32 lbl_8079BC68; /* -400.0f */

/* The file-wide `#pragma peephole off` this range's flags need: the target keeps the *unfused* forms
 * of three folds (`clrlwi r0,r4,24` + `cmpwi r0,0` for the mode switch, `slwi r0,r0,7` +
 * `clrlwi r3,r0,16` for the shake angle, `srwi r0,r0,5` + `clrlwi r3,r0,24` for the mode-1 bool),
 * where the pass emits the record-form `clrlwi.`/`clrlslwi`/`extrwi`.  Measured 2026-09-27 with the
 * real command line: on, the unit scores 97.57327 (98.75572 / 94.117645 / 100.0); off, 99.46.
 * Sibling precedent in the same band: `enemy/em035_prog.cpp`'s file-wide pragma. */
#pragma peephole off

/* Pushes the record's model state into the scene model: refreshes the effect matrix of the two
 * material slots (8 and 9) the record drives, then rewrites the material's K-colours - the 6/3 slot
 * from the record's own triple and the 0..3 slots from the area/action test. */
extern "C" void em020_model_refresh(_ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    _GXColor color;

    MTX34_ctor(&mtx);
    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access_a((nw4r::g3d::ScnMdl*) self->field_0x13C, 8);
        nw4r::g3d::ScnMdl::CopiedMatAccess access_b((nw4r::g3d::ScnMdl*) self->field_0x13C, 9);

        if (fn_800E2994(&access_a) != 0 && fn_800E2994(&access_b) != 0) {
            nw4r::g3d::ResTexSrt srt_a;
            nw4r::g3d::ResTexSrt srt_b;

            fn_8006F304(&srt_a, access_a.GetResTexSrt(false));
            fn_8006F304(&srt_b, access_b.GetResTexSrt(false));
            srt_a.GetEffectMtx(1, &mtx);
            mtx.m[0][3] = lbl_8079B8CC * fn_8005024C((u16) (system_w.field_0x0c << 7));
            mtx.m[1][3] += lbl_8079BC64;
            if (mtx.m[1][3] > lbl_8079B848) {
                mtx.m[1][3] -= lbl_8079B848;
            }
            srt_a.SetEffectMtx(1, &mtx);
            srt_b.SetEffectMtx(1, &mtx);
        }
    }

    ((MHchar*) &self->char_0x024)->getTevKColor(6, GX_KCOLOR3, &color);
    color.r = self->em020_kcolor_0x328.kcolor_r_0x339;
    color.g = self->em020_kcolor_0x328.kcolor_g_0x33A;
    color.b = self->em020_kcolor_0x328.kcolor_b_0x33B;
    ((MHchar*) &self->char_0x024)->setTevKColor(6, GX_KCOLOR3, &color);

    ((MHchar*) &self->char_0x024)->getTevKColor(0, GX_KCOLOR0, &color);
    if (self->area_no == 0 || em_act_ck(self, 13, 2) != 0) {
        color.r = 150;
        color.g = 150;
        color.b = 150;
    } else {
        color.r = 255;
        color.g = 255;
        color.b = 255;
    }
    ((MHchar*) &self->char_0x024)->setTevKColor(0, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(1, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(2, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(3, GX_KCOLOR0, &color);
}

/* The program's per-mode condition query: mode 0 is the target/self height difference as a 0..4
 * level, mode 1 the aim-target flag, mode 2 the slot-free test and mode 3 a timer's sign. */
extern "C" u8 em020_condition_ck(_ENEMY_WORK* self, u32 mode) {
    f32 diff;

    switch ((u8) mode) {
    case 0:
        diff = self->vec_0x36C.y - self->pos.y;
        if (diff >= lbl_8079B858) {
            return 2;
        }
        if (diff >= lbl_8079B870) {
            return 1;
        }
        if (diff <= lbl_8079B8D0) {
            return 4;
        }
        if (diff <= lbl_8079BC68) {
            return 3;
        }
        return 0;
    case 1:
        return em020_aim_target_ck(self) == 1;
    case 2:
        if (self->em020_0x328.timer_0x334 <= 0 && fn_8027DC64() == 1) {
            return 1;
        }
        return 0;
    case 3:
        return self->em020_0x328.timer_0x33E > 0;
    default:
        return 0;
    }
}

/* Switches the stage/model state when the program's map is the one that owns the two area models:
 * the record's interpreter stack is reset first, then the area selects the stage table entry and the
 * model's area mode. */
extern "C" void em020_area_model_set(_ENEMY_WORK* self) {
    fn_8013A9F4(self);

    if (fn_802B0668(self->field_0x1E0) == 7) {
        switch (self->area_no) {
        case 2:
            fn_802B0A98(9, 1);
            fn_802D94C4(0);
            break;
        case 3:
            fn_802B0A98(10, 1);
            fn_802D94C4(1);
            break;
        }
    }
}
