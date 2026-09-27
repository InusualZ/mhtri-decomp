/* enemy/fn_801BD6C0.cpp - the enemy "em" motion/act-instruction band's continuation,
 * `.text` 0x801BD6C0..0x801CA004 (128 functions, 0xC944 B), extab 0x8000FACC..0x8000FDFC (102
 * records), extabindex 0x8002B71C..0x8002BBE4 (102 x 12 B).  Registered from
 * `proposal/801BD6C0_fn_801BD6C0.cpp`.
 *
 * Module `enemy`.  Both bracketing registered units are `enemy/*`: below is `enemy/fn_801B7020.cpp`
 * (`.text` ends exactly at 0x801BD6C0, its extab at 0x8000FACC and its extabindex at 0x8002B71C,
 * which is where this unit's runs begin) and above, after the still-unclaimed 0x801CA004..0x801D428C
 * run, `enemy/fn_801D428C.cpp` (its extab starts at 0x800100D4, the next record after this unit's
 * 0x8000FDFC).  Every callee out of the range is the enemy work API (`get_joint_wpos_em`,
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `em_after_frame_check__FP11_ENEMY_WORKUsff`, `fn_80130478`,
 * `fn_8012F93C`, `get_now_areano__Fv`).
 *
 * Seam.  `tudiscover at 0x801BD6C0` reports a weak left edge (the closure edge): the bounding cut is
 * the size cap the proposal records, not a translation-unit boundary - the region's edges are
 * `enemy/fn_801B7020.cpp`'s end and the as-yet-unregistered 0x801CA004..0x801D428C run, and this
 * unit's extab/extabindex runs end exactly where that run's would begin.  Registered separately from
 * `enemy/fn_801B7020.cpp` per the proposal's pinned seam; the header of that unit records the same
 * provisional boundary.  See the outbox.
 *
 * Name.  The symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `tools/symbols/dumpmap.py lookup 0x801BD6C0`: the runtime dump answers `zz_01bd6c0_` and the rest
 * `zz_`/nothing; no `__FILE__`-spelling string is referenced anywhere in the range - every data
 * reference is a pool float, an integer table or a `.bss` block), so the file keeps the map's stem
 * (brief section 2, class 4).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/dumpmap.py lookup` and a scan of the range's data references in the auto split)
 *
 * Language: C++ (re-derived from the range's own evidence: its callees carry argument-list manglings -
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`,
 * `setVector3__FPQ34nw4r4math4VEC3fff` - and the flat `fn_*` symbols are `extern "C"` so objdiff
 * pairs them by name).
 *
 * Status (this branch).  Twelve rows are written and measured one at a time with
 * `python tools/units/recompile.py enemy/fn_801BD6C0.cpp --measure <symbol>` - eleven are exactly
 * 100 % and `fn_801BD838` is 97.67 % (its 344 B object is the target's size to the byte).  The other
 * 116 rows of the range are a residual of the round (unwritten), not a claim.
 *
 * Residual - `fn_801BD838` 97.67 %.  Same 86 instructions and same 344 B; the only difference is the
 * order of the `li r4, 0` and `lfs f1, lbl_80798EA8@sda21` pair before the `fn_80134004(self, 0,
 * lbl_80798EA8)` call (retail loads f1 first, this build materialises the integer first).  Tried: a
 * `f32 scale = lbl_80798EA8;` local before the call (unchanged, 97.67 %).  The one other scheduling
 * difference the range first showed (`fn_801BD6C0`'s `li r4, 0x2D`) was closed by spelling the limit
 * as the ternary `arg1 == 1 ? 0x50 : 0x2D` instead of an `if` over an initialised local.
 *
 * `#pragma peephole off` is this unit's one scoped pragma (the same lever `enemy/fn_8015D860.cpp`
 * and `enemy/fn_801B7020.cpp` document, docs/plan.md 8.2): every row here measures at 100 % with the
 * `-O3` peephole pass off.  No lib flag is involved - the `enemy` lib's `cflags_main` measures every
 * body below.
 */

/* The two fields the shared `_ENEMY_WORK` view did not carry yet are declared there (rule 1): the
 * `_se_w*` handle at +0xB14 (`se_req_pos_ps`/`shell_se_req`).  The pool floats are declared locally,
 * as `src/draw_shape.cpp`/`src/ef/effect.cpp` do.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */

#include "nw4r/math.h"

#include "enemy/ENEMY_WORK.h"

#include "unsplit/enemy.h"
#include "unsplit/unknown.h"

#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"

#include "sound/fn_800D7F54.h"

/* The SE request layer's owner header (rule 2): `se_req_pos_ps` is declared there, once, in
 * `sound/se.h`.  A local copy in `sound/fn_800D7F54.h` clashed with `ef/fn_80105314.cpp`'s own
 * `void se_req_pos_ps(...)` spelling (`(10505) illegal overloading`). */
#include "sound/se.h"

/* `VEC3_ctor` (the 3-float record writer) - via the ef band, because `sound/se.h` and the
 * owner's `mh3_pad.h` both declare `copyVec3`/`setVec3` with signatures that clash
 * (`(10197) illegal function overloading`), the same conflict `ai/fn_802CC794.cpp` records.  The
 * ef band carries the identical `Vec3*` spelling (`Vec3` is `nw4r::math::VEC3`). */
#include "unsplit/ef.h"

#include "ef/eft009.h"

#include "draw_shape.h"
#include "fn_8004CAD8.h"

#pragma peephole off

/* The `.sdata2` pool floats the range's state steps compare and arm with (the same local-declaration
 * convention `src/draw_shape.cpp`/`src/ef/effect.cpp` use). */
extern f32 lbl_80798E40;
extern f32 lbl_80798E54;
extern f32 lbl_80798E58;
extern f32 lbl_80798E60;
extern f32 lbl_80798E7C;
extern f32 lbl_80798E88;
extern f32 lbl_80798E90;
extern f32 lbl_80798E9C;
extern f32 lbl_80798EA4;
extern f32 lbl_80798EA8;
extern f32 lbl_80798EAC;
extern f32 lbl_80798EB0;
extern f32 lbl_80798EB4;
extern f32 lbl_80798EB8;
extern f32 lbl_80798EBC;
extern f32 lbl_80798EC0;
extern f32 lbl_80798EC4;
extern f32 lbl_80798EC8;
extern f32 lbl_80798ECC;
extern f32 lbl_80798ED0;
extern f32 lbl_80798ED4;
extern f32 lbl_80798ED8;
extern f32 lbl_80798EDC;
extern f32 lbl_80798EE0;
extern f32 lbl_80798EE4;
extern f32 lbl_80798EE8;
extern f32 lbl_80798EEC;
extern f32 lbl_80798EF0;
extern f32 lbl_80798EF4;
extern f32 lbl_80798EF8;

/* ------------------------------------------------------------------------------------------------ */
/* 0x801BD6C0 - step 4 of the "em" action: a two-state move     */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801BD6C0(_ENEMY_WORK* self, u8 arg1)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F62C(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        self->timer_0x020 = 0;
        break;
    case 1:
        if ((self->timer_0x020 & 0x1F) == 0) {
            u32 type;

            fn_80136B50(self, -1, 5);
            get_joint_wpos_em(self, 3, &pos);
            if ((self->field_0x228 & 6) != 0) {
                type = 0x54;
                pos.y = lbl_80798EA4 + self->field_0x210;
            } else {
                type = 0x53;
                pos.y = lbl_80798EA4 + self->field_0x20C;
            }
            fn_801048B4(self, 3, type, 0, lbl_80798E58);
            if (self->area_no == get_now_areano()) {
                se_req_pos_ps(self->se_0xB14, 0x27, 2, &pos);
            }
        }
        {
            s32 limit = arg1 == 1 ? 0x50 : 0x2D;

            if (++self->timer_0x020 >= limit) {
                fn_801280F4(self);
            }
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x801BD838 - step 8 of the "em" action: a seated wait        */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801BD838(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        fn_80130478(self, 4);
        fn_8012F5B8(self, 8, 0, 0);
        fn_8012F8C8(self, lbl_80798E90);
        fn_8012933C(self, 0, 0x21, 2);
        fn_80134004(self, 0, lbl_80798EA8);
        fn_801303EC(self, fn_8013032C(self));
        self->timer_0x020 = 0x14;
        break;
    case 1:
        if (self->area_no == get_now_areano()) {
            get_joint_wpos_em(self, 3, &pos);
            shell_se_req(self->se_0xB14, &pos, 8, self->field_0x01A);
        }
        if (self->state_0x006 == 0 && fn_80134114(self, 0, 0x40) == 1) {
            self->state_0x006 = 1;
        }
        if (--self->timer_0x020 <= 0 && self->state_0x006 == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* the per-action state steps the dispatchers below drive                                              */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801BDA28 - action 0x2F: a two-state motion hand-off (one 0..1 branch). */
extern "C" void fn_801BDA28(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x2F, 4, 0);
        fn_80129668(self, 0, 1);
        fn_80129668(self, 1, 2);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801BDEF8 - action 0xD1: the same shape with the 6/0x18 motion set. */
extern "C" void fn_801BDEF8(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xD1, 6, 0);
        fn_80129668(self, 0, 6);
        fn_80129668(self, 1, 0x18);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801BDF94 - action 0xD2: the same shape with the single 0..7 motion set. */
extern "C" void fn_801BDF94(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xD2, 6, 0);
        fn_80129668(self, 0, 7);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801BE508 - the seated en/disable step (0xCE): three window timers and the finish gate. */
extern "C" void fn_801BE508(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F5B8(self, 0xCE, 0, 0);
        fn_80129668(self, 0, 0x16);
        fn_801303EC(self, fn_8013032C(self));
        fn_80130CDC(self, -0xA);
        fn_8012CF20(self);
        fn_80131E74(self);
        fn_80136D14(self);
        fn_80131E00(self);
        break;
    case 1:
        fn_8012CF20(self);
        fn_80131E74(self);
        if (em_frame_check(self, 2, lbl_80798E54, lbl_80798E40) == 1) {
            fn_80131E00(self);
        }
        if (em_frame_check(self, 2, lbl_80798ED8, lbl_80798E40) == 1) {
            fn_80136D14(self);
        }
        if (em_frame_check(self, 0, lbl_80798E58, lbl_80798E40) == 1) {
            fn_80130478(self, 0);
            fn_801303EC(self, lbl_80798E40);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801BE198 - action 0xD4: the three-state seated step. */
extern "C" void fn_801BE198(_ENEMY_WORK* self, u8 arg1)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xD4, 6, 0);
        fn_80130CDC(self, -6);
        fn_8012933C(self, 0, 9, 8);
        fn_8012933C(self, 1, 0x29, 0x18);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798ECC, lbl_80798E40) == 1) {
            fn_8012933C(self, 0, 0xA, 0x10);
        }
        if (arg1 == 1) {
            if (em_frame_check(self, 0, lbl_80798ED0, lbl_80798E40) == 1) {
                self->state++;
                fn_8012F5B8(self, 0xD9, 0, 0);
                fn_8012933C(self, 0, 0x1A, 8);
                fn_8012933C(self, 1, 0x2A, 0x18);
                return;
            }
        } else {
            if (fn_8012F93C(self) == 1) {
                fn_80127F48(self);
            }
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_80798E7C, lbl_80798E40) == 1) {
            fn_8012933C(self, 0, 0x1B, 0x10);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801BEFD4 - a 4-byte `b fn_801BE508` tail entry. */
extern "C" void fn_801BEFD4(_ENEMY_WORK* self)
{
    fn_801BE508(self);
}

/* 0x801BEFD8 - the same entry guarded by `state_sub == 0`. */
extern "C" void fn_801BEFD8(_ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        fn_801BEFD4(self);
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* the 0x801C25xx predicate helpers the action band reads                                              */
/* ------------------------------------------------------------------------------------------------ */

/* 0x801C2508 - a 4-byte `blr`. */
extern "C" void fn_801C2508(void)
{
}

/* 0x801C250C - `(arg1 == 0) && (self->+0x328 == 0)` (the target reads the byte, not a word: it
 * loads +0x328 with `lbz`). */
extern "C" s32 fn_801C250C(_ENEMY_WORK* self, u8 arg1)
{
    if (arg1 == 0 && self->init_0x328.slots_0x328[0] == 0) {
        return 1;
    }
    return 0;
}

/* 0x801C2A58 - flips the halfword rotation and clears the third angle. */
extern "C" void fn_801C2A58(_ENEMY_WORK* self)
{
    self->field_0x1C0 = (s32)(u16)(self->field_0x1C0 + 0x8000);
    self->field_0x1C4 = 0;
}
