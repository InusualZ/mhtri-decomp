/* enemy/fn_80181C88.cpp - the enemy MHchar material/step band 0x80181C88..0x80182D5C (4,308 B,
 * 21 functions).  Registration is this worker's; the extent is the probe's.
 *
 * MODULE AND NAME.  `enemy`: the unit below (`enemy/fn_80178378.cpp`) ends exactly at this range's
 * start, the unit above (`enemy/fn_80182D5C.cpp`) begins exactly at its end, and every callee out of
 * the range is an enemy-band symbol (`_ENEMY_WORK`, `em_parts_damage_level_get`, `get_em_chg_scale`,
 * `fn_80129xxx`, `fn_8013A654`).  There is no `__FILE__` string for the range: the only in-image
 * `enemy_control.cpp` literal (`0x805A1B98`) is referenced by one function inside the already-registered
 * `enemy/enemy_control.cpp` band (0x801411B8) and by none of this range's functions, so the
 * discovery queue's "one source file (enemy_control.cpp)" note is its `source_owner` walking back to
 * the latest accepted name - not evidence this range is that file (checked with
 * `python tools/splits/tudiscover.py at 0x80181C88`, which reports no source anchor, no must-link
 * anchor and only weak `.sdata2` cuts on both sides; and by reading the range's own `.sdata2` pool:
 * every `lbl_80797xxx` it loads is a bare float).  `python tools/symbols/dumpmap.py lookup
 * 0x80181C88` answers only `zz_0181c88_`, so the file keeps the map's own stem (brief section 2,
 * class 4).
 *
 * SEAM: unproven (docs/plan.md 8.3).  The probe's left cut is a `.sdata2` pool-run jump
 * (lbl_80797E68 -> lbl_80797E70) and its right cut another (lbl_80797E98 -> lbl_80797EA8); neither
 * is a TU boundary, so the range is registered at the probe's own extent, which is function-aligned
 * on both ends.  If the original object is one TU with `enemy/fn_80178378.cpp` the two splits blocks
 * are one file; the merge candidate is recorded rather than taken.
 *
 * SECTIONS (evidence: each function's own per-symbol split object, `build/RMHE08/obj/auto_*_text.o`).
 * 16 of the 21 functions carry a C++ exception frame (an 8-byte `extab` + a 12-byte `extabindex`
 * entry); the extab block 0x8000E96C..0x8000E9EC (16 entries) and the extabindex block
 * 0x80029D0C..0x80029DCC (16 entries) are contiguous and belong to these functions only (the unit
 * below's extab ends at 0x8000E96C / extabindex at 0x80029D0C, the next unit's block starts at
 * 0x8000E9EC / 0x80029DCC):
 *   extab       start:0x8000E96C end:0x8000E9EC
 *   extabindex  start:0x80029D0C end:0x80029DCC
 *   .text       start:0x80181C88 end:0x80182D5C
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80181C88 0x80181CC0 0x80182D44`, which answers only
 * `zz_XXXXXXXX_` placeholders, and by reading the probe's own data pool: no `__FILE__` string).
 *
 * The unit's functions are the `_ENEMY_WORK` colour/material steppers and their helpers: fn_80181C88
 * is the scalar clamp they all share, fn_80181CC0/fn_80181E24 drive the MHchar TEV key colours,
 * fn_801820DC/fn_80182768/fn_80182978 are the area/action dispatchers, and fn_80182430/fn_80182C40
 * build the rotated target offsets the movement code measures.  Every `fn_XXXXXXXX` definition is
 * `extern "C"`; only the mangles reached through their real owners (MHchar members) are C++.
 *
 * STATUS (official report metric, `recompile.py --measure`, against MAIN's retired `auto_*_text.o`
 * split objects - the same original bytes the registered object will carry): all 21 functions
 * written; 7 byte-identical, 20 at or above the 80 % bar.  The one below it is fn_80182978.
 *   100.00 fn_80181C88 fn_801823A0 fn_801823C0 fn_8018257C fn_80182914 fn_80182AB8 fn_80182D44
 *    97.39 fn_80182080   95.43 fn_80182B38   94.63 fn_80181E24   93.75 fn_80182040
 *    93.13 fn_80182918   91.24 fn_801820DC   89.90 fn_801825A4   88.79 fn_80182768
 *    85.45 fn_80181CC0   84.64 fn_80182430   84.06 fn_80182320   82.88 fn_80182C40
 *    81.40 fn_80182B94   77.04 fn_80182978
 *
 * RESIDUALS (measured, not guessed):
 *  - fn_80182978 77.04 % (320 B target, 296 B ours).  The four `fn_80126278` argument builds are
 *    `clrlwi r0,r0,28; slwi r0,r0,8; clrlwi r4,r0,16` in retail (`(u16)((area_no & 0xF) << 8)`),
 *    but this compiler folds the mask into one `rlwinm r4,r0,8,20,23`.  Six source spellings were
 *    measured - `& 0xF` with `<< 8`, `* 256`, `% 16 * 256`, a `u16`/`u32` local, a `(u8)` cast and
 *    a 4-bit bitfield view of +0x1E1 - and all fold identically (best 77.04 %); the retail shape
 *    needs the un-folded intermediate the stopping rule names.  The case body's own `(s32)`
 *    comparisons (target `cmpwi`, not `cmplwi`) are kept because they measured +3.7 points.
 *  - fn_801820DC 91.24 % (580/560): the action-start chain is a shared-tail `goto` in retail
 *    (rule 8 forbids it).  The conformant shape is the nested `if` with the two `switch` selects;
 *    the remaining rows are the `cmpwi`/`cmplwi` and re-mask colouring.  Measured: nested
 *    `if`/`else if` for the two selects 75.5 %, `switch` selects (kept) 91.24 %.
 *  - The 80-90 % group is same-size argument-evaluation-order colouring (float pool load vs `li`
 *    placement) and register colouring; each is the MWCC scheduler, not a source shape.
 *
 * SHARED FILES this landing edits in its own worktree (each filed as a `shared-file` config request):
 *  - `include/enemy/ENEMY_WORK.h`: the EmColorBlock view of +0x328 (the colour scalar + K-colour
 *    bytes), `field_0x48F`, the EmPartState block at +0x740 and `field_0x81A`; every other offset is
 *    unchanged.
 *  - `include/enemy/fn_8012E968.h` (new): the owner declarations of `fn_8012EC3C`/`fn_8012EC60`
 *    (rule 2).  `fn_8012EC60` keeps the `(void)` spelling the landed bands need - a variadic
 *    or `(self)` spelling here costs `enemy/fn_801CA004.cpp` 1.1 points and `fn_8019DB9C` 6.25
 *    (measured), because those call sites leave the record in r3.
 *  - `include/unsplit/enemy.h`: re-exports that owner header instead of carrying its own copy
 *    of the two declarations (rule 2; the band is a fallback, not the owner).
 *  - `include/enemy/fn_801251D0.h`: `fn_80128AEC`/`fn_80128B80`/`fn_80129A70`/`fn_80129DB8`/
 *    `fn_8012A014`/`fn_8012A204` (owner header, rule 2).
 *  - `include/enemy/fn_80138074.h`: the `EmUserData` record `fn_8013A654` runs on.
 *  - `include/enemy/fn_8011D448.h` (new): `em_parts_damage_level_get`, declared at C++ scope
 *    (rule 9, the map name is the mangling).
 *  - `include/fn_8004CAD8.h`: `fn_8005024C`.
 *  - `include/sound/mhchar.h`: the pointer-taking `setTevKColor` overload the enemy action band's
 *    target relocations encode (`...P8_GXColor`); the by-value spelling is kept for the existing
 *    `enemy/fn_801D428C.cpp`/`enemy/fn_801DB8E0.cpp` call sites.
 *
 * fn_80182B94 seals its four table entries with the `copyVec3` 0xC-byte copy (mh3_pad.h) rather
 * than the target's `fn_80051490`: the target's reloc pairs identically in the report metric (both
 * measured 81.40 %) and declaring `fn_80051490` in its owner header collides with the
 * `Vec*`-spelling locals in `ef/ef_cylinder.cpp` (a pre-existing rule-2 conflict, not this unit's).
 */
#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"       /* MHchar, with the pointer-taking setTevKColor overload */
#include "enemy/ENEMY_WORK.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"   /* system_w */
#include "enemy/fn_801251D0.h" /* fn_80126278/fn_80126324 + the 0x80129xxx helpers */
#include "enemy/fn_8012EC74.h" /* fn_8013026C */
#include "enemy/fn_8012BDF4.h" /* fn_8012E5A8 */
#include "enemy/fn_80138074.h" /* fn_8013A654/fn_8013918C + the EmUserData record */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "fn_8004CAD8.h"       /* fn_8005024C/fn_80051378/rotVecY/calcDistanceSqXZ */
#include "mh3_pad.h"           /* VEC3_ctor/copyVec3/setVec3 */
#include "sys_mem.h"           /* operator delete (the `__dl__FPv` global deleter) */

/* One 0x16-byte action record `fn_80182AB8` builds (the +0x00 type word, a VEC3 and three scalars).
 * size: 0x16 */
struct EmWorkItem {
    /* +0x00 */ u32 type_0x00;
    /* +0x04 */ VEC3 vec_0x04;
    /* +0x10 */ u8 field_0x10;
    /* +0x12 */ s16 field_0x12;
    /* +0x14 */ s16 field_0x14;
};

extern "C" {

/* `fn_802B0668` (the byte-table map lookup) comes from `include/unsplit/unknown.h`, which already
 * carries the band-interleaves-modules declaration. */

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions, declared up front so the later ones can call forward.
 * ------------------------------------------------------------------------------------------------ */
f32 fn_80181C88(f32 value, f32 center, f32 step);
void fn_80181CC0(_ENEMY_WORK* self);
void fn_80181E24(_ENEMY_WORK* self);
u32 fn_80182040(_ENEMY_WORK* self);
void fn_80182080(_ENEMY_WORK* self, u32 part);
u32 fn_801820DC(_ENEMY_WORK* self, u32 arg);
void fn_80182320(_ENEMY_WORK* self);
void fn_801823A0(_ENEMY_WORK* self, u32 value);
void fn_801823C0(EmUserData* self);
u32 fn_80182430(_ENEMY_WORK* self, u32 arg);
u32 fn_8018257C(_ENEMY_WORK* self);
u32 fn_801825A4(_ENEMY_WORK* self, u32 kind);
void fn_80182768(_ENEMY_WORK* self, u8* out_a, u8* out_b);
void fn_80182914(_ENEMY_WORK* self);
u32 fn_80182918(_ENEMY_WORK* self);
void fn_80182978(_ENEMY_WORK* self);
void fn_80182AB8(struct EmWorkItem* out, u32 a, s16 b, s16 c);
void* fn_80182B38(void* p, s16 arg);
void fn_80182B94(void);
void fn_80182C40(_ENEMY_WORK* self, u32 kind, void* out);
void fn_80182D44(_ENEMY_WORK* self, u32 arg);

/* ------------------------------------------------------------------------------------------------
 * Pool literals owned by the data pass (unresolved module -> declared, never defined; playbook 29).
 * ------------------------------------------------------------------------------------------------ */
extern f32 lbl_80797B10;
extern f32 lbl_80797B14;
extern f32 lbl_80797B18;
extern f32 lbl_80797B20;
extern f32 lbl_80797B2C;
extern f32 lbl_80797B34;
extern f32 lbl_80797B40;
extern f32 lbl_80797B58;
extern f32 lbl_80797B5C;
extern f32 lbl_80797B64;
extern f32 lbl_80797B9C;
extern f32 lbl_80797BB0;
extern f32 lbl_80797BB4;
extern f32 lbl_80797BE4;
extern f32 lbl_80797C14;
extern f32 lbl_80797C24;
extern f32 lbl_80797C34;
extern f32 lbl_80797C90;
extern f32 lbl_80797D08;
extern f32 lbl_80797E6C;
extern f32 lbl_80797E70;
extern f32 lbl_80797E78;
extern f32 lbl_80797E7C;
extern f32 lbl_80797E80;
extern f32 lbl_80797E88;
extern f32 lbl_80797E8C;
extern f32 lbl_80797E90;
extern f32 lbl_80797E94;
extern f32 lbl_80797E98;
/* The two argument labels `fn_8012A014` takes (`.data`, no module). */
extern u32 lbl_805A950C[];
extern u32 lbl_805A9518[];
/* The four 0xC-byte vectors `fn_80182B94` seeds (`.data`, no module). */
extern nw4r::math::VEC3 lbl_806A79D0[];

} /* extern "C" */

/* ------------------------------------------------------------------------------------------------
 * fn_80181C88 - step a scalar toward `center` by `step`, never overshooting it.  One float on each
 * side, so it is a plain helper, not a member.
 * ------------------------------------------------------------------------------------------------ */
f32 fn_80181C88(f32 value, f32 center, f32 step) {
    if (value < center) {
        value += step;
        if (value < center) {
            return value;
        }
    } else if (value > center) {
        if (value > center + step) {
            return value - step;
        }
    }
    return center;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80181CC0 - drive the work's colour scalar and the three K-colour bytes toward the mode's
 * targets: the scalar uses the mode-independent pool float (0x1E2 == 2 picks the second), then
 * fn_80182918's damage level picks the two byte targets.
 * ------------------------------------------------------------------------------------------------ */
void fn_80181CC0(_ENEMY_WORK* self) {
    f32 target = (self->field_0x1E2 == 2) ? lbl_80797B10 : lbl_80797B9C;
    self->color_0x328.field_0x328 = fn_80181C88(self->color_0x328.field_0x328, target, lbl_80797E6C);
    u8 level = (u8)fn_80182918(self);
    f32 byte_a;
    f32 byte_b;
    if (level == 1) {
        target = lbl_80797BB4;
        byte_a = lbl_80797B20;
        byte_b = lbl_80797C24;
    } else if (level == 2) {
        target = lbl_80797B9C;
        byte_a = lbl_80797B40;
        byte_b = lbl_80797B10;
    } else {
        target = lbl_80797B18;
        byte_a = target;
        byte_b = target;
    }
    self->color_0x328.field_0x32C =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32C, target, lbl_80797C14);
    self->color_0x328.field_0x32D =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32D, byte_a, lbl_80797C14);
    self->color_0x328.field_0x32E =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32E, byte_b, lbl_80797C14);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80181E24 - the MHchar K-colour material update: rebuild the four channel colours from the
 * work's scalar and byte colours, then set the fifth from the aim/special state.
 * ------------------------------------------------------------------------------------------------ */
void fn_80181E24(_ENEMY_WORK* self) {
    _GXColor color;
    fn_80181CC0(self);
    color.r = (u8)(s32)self->color_0x328.field_0x328;
    color.g = (u8)(s32)self->color_0x328.field_0x328;
    color.b = (u8)(s32)self->color_0x328.field_0x328;
    color.a = 0xFF;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR1, &color);
    if ((self->flags_0x836 & 1) != 0) {
        if (self->field_0x48F == 1) {
            color.r = 0xFF;
            color.g = 0xFF;
            color.b = 0xFF;
        } else {
            color.r = 100;
            color.g = 100;
            color.b = 100;
        }
    }
    ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
    color.r = self->color_0x328.field_0x32C;
    color.g = self->color_0x328.field_0x32D;
    color.b = self->color_0x328.field_0x32E;
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(5, GX_KCOLOR3, &color);
    if (fn_8012EC3C(self) == 1) {
        color.a = 0;
    } else if (fn_8012EC60(self) == 1) {
        color.a = (u8)((s32)(lbl_80797BB0 * (lbl_80797B14 *
                    (lbl_80797B34 + fn_8005024C((u16)(system_w.field_0x0c << 13))))) + 225);
    } else {
        color.a = (u8)((s32)(lbl_80797BB0 * (lbl_80797B14 *
                    (lbl_80797B34 + fn_8005024C((u16)(system_w.field_0x0c << 13))))) + 135);
    }
    ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR3, &color);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182040 - true for the dead mode (0x1E2 == 2) while the record has not latched its aim state.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_80182040(_ENEMY_WORK* self) {
    if (self->field_0x1E2 == 2 && fn_8012EC60(self) == 0) {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182080 - clamp the 0x1E4 counter: part 3 at full damage caps it at 99.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182080(_ENEMY_WORK* self, u32 part) {
    if ((part & 0xFF) == 3) {
        if ((u8)em_parts_damage_level_get(self, 3) == 3) {
            if (self->field_0x1E4 >= 100) {
                self->field_0x1E4 = 99;
            }
        }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801820DC - the area/action start predicate: true when the record's map/area state arms the
 * next action, either by setting the 0x1FC/0x1FE/0x1FF request or by running the 0x8012A014 test.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_801820DC(_ENEMY_WORK* self, u32 arg) {
    u8 mode = (u8)fn_802B0668(self->field_0x1E0);
    if (mode != 1 && mode != 3) {
        return 0;
    }
    u32 armed = 0;
    u32 probe;
    switch (mode) {
    case 1:
        probe = 8;
        break;
    case 3:
        probe = 6;
        break;
    default:
        probe = 0xFF;
        break;
    }
    if (probe != 0xFF) {
        u8 state = (u8)fn_80129DB8(self);
        switch (state) {
        case 1:
            armed = 1;
            break;
        case 2:
            return 1;
        default:
            break;
        }
    }
    if (armed == 0) {
        if (self->field_0x1FC == 1 && self->field_0x1FE == 8) {
            return 1;
        }
        if (self->value_0x452 >= self->field_0x450 || self->field_0x43D != 1) {
            if (fn_8012EC3C(self) == 1 && self->color_0x328.field_0x32F == 0) {
                if (mode == 1) {
                    if ((s32)self->area_no == 7) {
                        self->field_0x1FC = 1;
                        self->field_0x1FE = 8;
                        self->field_0x1FF = 12;
                        return 1;
                    }
                } else if (mode == 3) {
                    if ((s32)self->area_no == 4 || (s32)self->area_no == 8) {
                        self->field_0x1FC = 1;
                        self->field_0x1FE = 8;
                        self->field_0x1FF = 3;
                        return 1;
                    }
                }
            } else if (self->color_0x328.field_0x330 != 0) {
                u32 sel;
                switch (mode) {
                case 1:
                    sel = 8;
                    break;
                case 3:
                    sel = 3;
                    break;
                default:
                    sel = 0xFF;
                    break;
                }
                if (fn_8012A014(self, 23, sel, (u16)arg, lbl_805A950C, lbl_805A9518) == 1) {
                    return 1;
                }
            }
        }
    }
    if (fn_80129A70(self, (u16)arg) == 1) {
        return 1;
    }
    return fn_8012A204(self) == 1;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182320 - the area's follow-up arming: when the action has run out (not action 10, 0x81A <= 0)
 * pick the two motion ids the 0x1E2 mode names.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182320(_ENEMY_WORK* self) {
    fn_80128B80(self);
    if (self->action != 10 && self->field_0x81A <= 0) {
        if (self->field_0x1E2 == 0) {
            fn_80128AEC(self, 13, 12);
        } else if (self->field_0x1E2 == 2) {
            fn_80128AEC(self, 13, 13);
        }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801823A0 - set the part flag from a one-value selector.
 * ------------------------------------------------------------------------------------------------ */
void fn_801823A0(_ENEMY_WORK* self, u32 value) {
    if (value == 1) {
        self->part_0x740.field_0x740 = 1;
    } else {
        self->part_0x740.field_0x740 = 0;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801823C0 - reset the work's part state after the user-data teardown.
 * ------------------------------------------------------------------------------------------------ */
void fn_801823C0(EmUserData* self) {
    fn_8013A654((_ENEMY_WORK*)self, 5);
    self->work_0x04->part_0x740.field_0x740 = 1;
    self->work_0x04->part_0x740.field_0x742 = 0;
    self->work_0x04->part_0x740.field_0x744 = 0;
    self->work_0x04->part_0x740.field_0x746 = 0;
    self->work_0x04->part_0x740.field_0x748 = 0;
    self->work_0x04->part_0x740.field_0x74A = 0;
    self->work_0x04->part_0x740.field_0x74C = 0;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182430 - "is the work within the aimed target's attack range": builds the target's rotated
 * offset from the work position, adds it to the candidate record's position and compares the squared
 * XZ distance against the scaled range.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_80182430(_ENEMY_WORK* self, u32 arg) {
    VEC3 a;
    VEC3 b;
    VEC3 rel;
    VEC3 probe;
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    _ENEMY_WORK* target = fn_80131034(self, 23, 0);
    if (target == 0) {
        return 0;
    }
    if (fn_8012E5A8(target) != 1) {
        return 0;
    }
    if ((arg & 0xFF) == 0) {
        return 1;
    }
    setVec3(&rel, lbl_80797B18, lbl_80797B18, lbl_80797D08 * get_em_chg_scale(self));
    copyVec3(&b, &rel);
    rotVecY(&b, self->field_0x1C0);
    fn_80051378(&probe, &self->pos, &b);
    copyVec3(&a, &probe);
    f32 dist = calcDistanceSqXZ(&a, &target->pos);
    f32 range = lbl_80797BE4 * get_em_chg_scale(self);
    if (dist >= range * (lbl_80797BE4 * get_em_chg_scale(self))) {
        return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018257C - the "action 13, sub-state <= 5" predicate.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_8018257C(_ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * fn_801825A4 - the per-kind aim/approach query the action band dispatches over.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_801825A4(_ENEMY_WORK* self, u32 kind) {
    switch (kind & 0xFF) {
    case 0: {
        f32 delta = self->vec_0x36C.y - self->pos.y;
        if (delta >= lbl_80797B58) {
            return 2;
        }
        if (delta >= lbl_80797B2C) {
            return 1;
        }
        if (delta <= lbl_80797B5C) {
            return 4;
        }
        if (delta <= lbl_80797C90) {
            return 3;
        }
        return 0;
    }
    case 1:
        return fn_80182918(self);
    case 2:
        return self->color_0x328.field_0x32F;
    case 3: {
        _ENEMY_WORK* target = fn_80131034(self, 23, 0);
        if (target != 0) {
            return fn_8012E5A8(target) == 1;
        }
        return 0;
    }
    case 4:
        return self->color_0x328.field_0x332 < 900;
    case 5: {
        f32 scale = get_em_chg_scale(self);
        f32 limit = self->field_0x210 -
                    (lbl_80797B9C + fn_8013026C(self)) * scale;
        return self->pos.y < limit;
    }
    case 6:
        return self->color_0x328.field_0x330 != 0;
    case 7:
        if ((u8)em_parts_damage_level_get(self, 3) >= 3) {
            return 0;
        }
        return fn_80182918(self) != 0;
    default:
        return 0;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182768 - area/action -> motion pair: fills the two out-bytes and arms the motion ids the
 * map/area state names.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182768(_ENEMY_WORK* self, u8* out_a, u8* out_b) {
    switch ((u8)fn_802B0668(self->field_0x1E0)) {
    case 1:
        if (self->area_no == 7) {
            fn_80130478(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 14, 15, lbl_80797B18);
        } else if (self->area_no == 12) {
            fn_80130478(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 23, 24, lbl_80797B18);
        }
        break;
    case 3:
        if (self->area_no == 4) {
            fn_80130478(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 6, 7, lbl_80797B18);
        } else if (self->area_no == 8) {
            fn_80130478(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 6, 0, lbl_80797B18);
        }
        break;
    case 9:
    case 11:
        if (self->area_no == 1) {
            fn_80130478(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 0, 1, lbl_80797B18);
        }
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182914 - thin tail-call wrapper for fn_80182978.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182914(_ENEMY_WORK* self) {
    fn_80182978(self);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182918 - the 0x1E4 damage counter's level: 0 below 1, 1 below 100, and past that the part-3
 * damage level maps to 5, 4, 3, ... (retail's branchless `subfc`/`adde` run).
 * ------------------------------------------------------------------------------------------------ */
u32 fn_80182918(_ENEMY_WORK* self) {
    u8 health = self->field_0x1E4;
    if (health < 1) {
        return 0;
    }
    if (health < 100) {
        return 1;
    }
    u8 part = (u8)em_parts_damage_level_get(self, 3);
    return 5 - part + (part >= 3 ? -1 : 0);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182978 - the map/area aim/rotation setter: clears the aim vector then arms the rotation id
 * the map/area state names.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182978(_ENEMY_WORK* self) {
    f32 zero = lbl_80797B18;
    self->aim.x = zero;
    self->aim.y = zero;
    self->aim.z = zero;
    switch ((u8)fn_802B0668(self->field_0x1E0)) {
    case 1:
        if ((s32)self->area_no == 7 || (s32)self->area_no == 12) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256), &self->aim);
        }
        break;
    case 3:
        if ((s32)self->area_no == 3 || (s32)self->area_no == 6) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 8), &self->aim);
        } else if ((s32)self->area_no == 4) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 3), &self->aim);
        } else if ((s32)self->area_no == 8) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 4), &self->aim);
        }
        break;
    case 9:
    case 11:
        if ((s32)self->area_no == 1) {
            f32 v = lbl_80797B18;
            self->aim.x = v;
            self->aim.y = lbl_80797E78;
            self->aim.z = v;
        }
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182AB8 - build one 0x16-byte action record: the type, a fixed vector and the three scalars.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182AB8(struct EmWorkItem* out, u32 a, s16 b, s16 c) {
    VEC3 vec;
    setVec3(&vec, lbl_80797B18, lbl_80797E7C, lbl_80797B64);
    out->type_0x00 = 0x1A;
    copyVec3(&out->vec_0x04, &vec);
    out->field_0x10 = (u8)a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182B38 - the record's release step: unregister, then free when the caller asks (arg > 0);
 * returns the record so a caller can chain.
 * ------------------------------------------------------------------------------------------------ */
void* fn_80182B38(void* p, s16 arg) {
    if (p != 0) {
        fn_8013918C((_ENEMY_WORK*)p, 0);
        if (arg > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182B94 - install the four static vectors of the shared 0x806A79D0 table.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182B94(void) {
    VEC3 a;
    VEC3 b;
    VEC3 c;
    VEC3 d;
    setVec3(&a, lbl_80797B18, lbl_80797B18, lbl_80797E80);
    copyVec3(&lbl_806A79D0[0], &a);
    setVec3(&b, lbl_80797B18, lbl_80797B18, lbl_80797B18);
    copyVec3(&lbl_806A79D0[1], &b);
    setVec3(&c, lbl_80797C34, lbl_80797B18, lbl_80797B18);
    copyVec3(&lbl_806A79D0[2], &c);
    setVec3(&d, lbl_80797B18, lbl_80797B18, lbl_80797B18);
    copyVec3(&lbl_806A79D0[3], &d);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182C40 - rotate a per-kind offset into the work's position and write the resulting relative
 * vector the movement code steers by.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182C40(_ENEMY_WORK* self, u32 kind, void* out) {
    VEC3 v;
    VEC3 rel;
    VEC3_ctor(&v);
    v.x = lbl_80797E88;
    v.y = lbl_80797E88;
    v.z = lbl_80797E8C;
    s32 offset;
    switch (kind & 0xFF) {
    case 1:
        offset = 0x9555;
        v.z = lbl_80797E8C;
        break;
    case 2:
        offset = 0xAAAB;
        v.z = lbl_80797E94;
        break;
    case 10:
        offset = 0xA000;
        v.z = lbl_80797E98;
        break;
    case 11:
        offset = 0x4E39;
        v.z = lbl_80797E90;
        break;
    default:
        offset = 0x11C7;
        v.z = lbl_80797E90;
        break;
    }
    rotVecY(&v, (u16)(self->field_0x1C0 + offset));
    fn_80051378(&rel, &self->pos, &v);
    copyVec3((nw4r::math::VEC3*)out, &rel);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182D44 - forward the value to the motion setter only for team 0x10.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182D44(_ENEMY_WORK* self, u32 arg) {
    if (self->team == 0x10) {
        fn_80130CDC(self, (s16)arg);
    }
}
