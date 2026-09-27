/* enemy/fn_80182D5C.cpp - the enemy band's per-map/area step tables and their dispatchers.
 * .text 0x80182D5C..0x8018B3B8 (0x868C), 115 functions; extab 0x8000E9EC..0x8000ECC4;
 * extabindex 0x80029DCC..0x8002A210.
 *
 * Registration (proposal/80182D5C_fn_80182D5C.cpp).  The range is registered once, here, at its
 * final home.  Which class decided the name and module:
 *   * class 1 (a `__FILE__` string) fails: nothing in the region names a file.  The region's own
 *     `.data`/`.sdata2` pool entries are the shared constants 0.0/1.0/10.0/... (0x80797E88+, read
 *     out of `orig/RMHE08/sys/main.dol`), not a source-file name, and `dumpmap.py lookup` answers
 *     only `zz_` placeholders for this band.
 *   * class 3 (what the code does plus the neighbours' scheme) decides: every bracketing registered
 *     unit is `enemy/` (the unit below ends at 0x80178378, the next occupied band above is
 *     `auto_03_8018B3B8`), every callee the range names is an enemy-band function
 *     (`_ENEMY_WORK`, `em_frame_check__FP11_ENEMY_WORKUsff`, `get_enemy_data`), and the file keeps
 *     the map's own `fn_XXXXXXXX` stem because no better name is evidence-backed.
 *   * the seam is **unproven** (the brief's own warning): `tudiscover at 0x80182D5C` answers a
 *     1-function match set with weak cuts on both sides, no owned data and no anchors, and the
 *     brief records that the left edge is a `--max-bytes` cut.  The left neighbour is NOT this
 *     unit's predecessor: `enemy/fn_80178128.cpp` ends at 0x80178378 and the 0x80178378..0x80182D5C
 *     gap belongs to another proposal, so this unit starts where the run starts rather than
 *     extending that one.  The right edge (0x8018B3B8) is where the next function's symbol begins.
 *
 * C++ (`-lang=c++` through the lib's `cflags_main`) because the range reaches mangled callees -
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `em_die_ck__FP11_ENEMY_WORK` - through their real
 * signatures (rule 9), and `_ENEMY_WORK` carries `nw4r::math::VEC3` members.
 *
 * Types.  `_ENEMY_WORK` comes from its single home `include/enemy/ENEMY_WORK.h` (rule 1), not from
 * the older `include/enemy.h` copy.  Two bytes that header did not name yet were added there with
 * their offsets preserved (both are pure padding splits, so no other field moved):
 *   * `+0x1EC  u16 bits_0x1EC` - `fn_8018493C` does `lhz r0,0x1ec` then `clrlwi r3,r0,27`
 *     (`& 0x1F`) to derive its 0x96/0x5A frame countdown.
 *   * `+0x482  u8 field_0x482` - `fn_80184CE0`/`fn_80184C28` pick the approach float with it
 *     (`lbz r0,0x482; cmpwi r0,0; beq`); `include/enemy.h` already carried the same byte.
 * The pool constants this unit loads are declared `extern`, never defined (playbook 29): redefining
 * them would rebuild the pool instead of addressing the target's.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with `symedit` over
 * config/RMHE08/symbols.txt - every symbol defined here is a bare `.text` entry with no owner name -
 * and `python tools/symbols/dumpmap.py lookup 0x80182D5C` answers the `zz_` placeholder form, which
 * is not evidence).
 *
 * Status (official `build/RMHE08/report.json`, full `ninja` in this worktree, `main.dol: OK`).
 * 49 of the 115 functions are written and every one of them is above the 80 % bar; 36 are
 * byte-identical.  Unit: 20.966972 % fuzzy, 4272 / 34396 `.text` bytes matched, 36 / 115 functions
 * matched.  The 66 unwritten functions (27100 B) are named as the follow-up queue at the end of this
 * header.
 *
 * Residuals, by measurement (all the near-misses are codegen shapes, not comprehension):
 *   * ARGUMENT EVALUATION ORDER - `fn_80182D5C` 98.45, `fn_801844B8` 94.59, `fn_8018479C` 95.45,
 *     `fn_8018484C` 98.33, `fn_8018493C` 97.78, `fn_80185D60` 97.44.  The target evaluates the
 *     FLOAT argument of `fn_8012FCC4`/`fn_80134004` before the integer one (`lfs f1,pool` then
 *     `li r4,imm`); MWCC evaluates in declaration order, so that needs a `(self, f32, s32)` view of
 *     those two band symbols, while `include/unsplit/enemy.h` (and the landed consumers
 *     `enemy/fn_801550FC.cpp`, `enemy/fn_80147CE0.cpp`) carry `(self, s32, f32)`.  Both spellings
 *     are ABI-equivalent (one FPR slot and one GPR slot), which is why the body still links and
 *     measures: this is the declaration-order residual `enemy/fn_80147CE0.cpp` already recorded.
 *     Locally, naming the constant in a `f32` local reproduces the float-first order for the first
 *     call (`fn_80182D5C` 96.90 -> 98.45); the remaining rows are the second call's `mr r3` slot.
 *   * FUSED `subic.` - `fn_80183E9C` 97.75, `fn_8018493C` 97.78 (each 4 B short).  Retail keeps
 *     `subi r0,rX,1; stw r0,...; cmpwi r0,0; bgt`; this compiler fuses the decrement-with-compare
 *     into `subic. r0,rX,1; stw r0,...; bgt` and drops the `cmpwi`.  Written three ways (`--x <= 0`,
 *     `x--; if (x <= 0)`, `left = x - 1; x = left; if (left <= 0)`) and all three fuse; the mwcc
 *     scheduling shape the stopping rule names, recorded rather than chased.
 *   * `clrlwi` ON A u16 ARGUMENT - `fn_801846BC` 94.64, `fn_80185D60` 97.44.  Retail truncates the
 *     selected motion id at the call (`clrlwi r4,r4,16`); `(u16)` around a conditional whose arms
 *     are both small constants is folded away.  A `u32` local (`u32 motion = ... ; fn_8012F5B8(self,
 *     (u16)motion, ...)`) keeps the range unknown and is what `fn_801846BC` now uses - it moved the
 *     row rather than restoring it, so the remaining loss is that one instruction.  (The C view of
 *     `fn_8012F5B8` in `include/unsplit/enemy.h` is `(self, s32, s32, s32)`; a `u16` parameter would
 *     emit the truncation for free, but changing it would re-measure every landed consumer.)
 *   * REGISTER COLOURING, vtable store - `fn_80183440` 99.33.  Retail materialises `lbl_805AD340`
 *     into **r0** (`lis r3,@ha; addi r0,r3,@l; stw r0,0(r31)`), this build into r3.  Three spellings
 *     measured (`*(u32*)self = (u32)lbl;`, `*(void**)self = lbl;` and a struct field) and all colour
 *     r3; recorded.
 *   * `fn_801841B4` 99.90 / `fn_801842FC` 99.91 (216 B target / ours same size): every opcode and
 *     operand row pairs and my instruction-text comparison finds no difference beyond the format's
 *     absolute branch addresses; the report's remaining fraction is the branch-target row.  Recorded
 *     as measured; nothing to change in the source.
 *   * PAIRED-SINGLE EPILOGUE - `fn_801850F8` 97.78 (288 B both sides): retail restores the f31
 *     paired-single half with the indexed `li r0,0x18; psq_lx f31,r1,r0,0,qr0` (and saves it with
 *     `stfd` + `psq_st`), this build with the folded `psq_l f31,0x18(r1),0,qr0`; the whole body is
 *     instruction-identical.  Same one-instruction shape `enemy/fn_80178128.cpp` recorded, and the
 *     stopping rule's named unreachable class - recorded, not chased.
 *   * FOLDED u8 STORE MASK + FLOAT-FIRST ARGUMENT ORDER - `fn_80185B0C` 90.98.  Retail writes
 *     `srawi r0,r0,8; clrlwi r0,r0,24; stb r0,0x7(...)` where this build drops the mask (MWCC proves
 *     the shifted u16 is already a byte), and it evaluates `fn_80133E3C`'s two float arguments before
 *     its integer one (the same declaration-order residual as above; the band header's view is
 *     `(self, s32, f32, f32)`).  Both rows measured; the body is otherwise identical.
 *
 * Follow-up queue (the 66 unwritten functions, biggest first; sizes in bytes):
 *   fn_80188A30 (2500), fn_80186D08 (2008), fn_801884E4 (1356), fn_80189CAC (1316), fn_80183040
 *   (1024), fn_8018A1D0 (1016), fn_8018A5C8 (940), fn_801879E4 (904), fn_80184D98 (864),
 *   fn_8018AE7C (840), fn_8018814C (740), fn_80185218 (712), fn_801837B0 (676), fn_8018347C (668),
 *   fn_801897B0 (516), fn_801854E0 (472), fn_80187D6C (472), fn_80185938 (468), fn_80186B34 (468),
 *   fn_801894BC (432), fn_80185DFC (420), fn_80189A68 (376), fn_80187F44 (356), fn_8018B258 (352),
 *   fn_8018966C (324), fn_801878A4 (320), fn_80186020 (296), fn_8018777C (296), fn_80185C6C (244),
 *   fn_801875A4 (240), fn_801866F4 (236), fn_801867E0 (232), fn_80187694 (232), fn_801862CC (216),
 *   fn_801861F8 (212), fn_8018AB94 (208), fn_80186438 (200), fn_80186A6C (200), fn_801893F4 (200),
 *   fn_801874E0 (196), fn_8018A9A4 (196), fn_8018663C (184), fn_80188430 (180), fn_801899B4 (180),
 *   fn_80186594 (168), fn_801869C8 (164), fn_801880A8 (164), fn_80186148 (156), fn_80183718 (152),
 *   fn_801868C8 (152), fn_801863A4 (148), fn_80186500 (148), fn_8018ADE8 (148), fn_8018B1C4 (148),
 *   fn_8018AAD8 (140), fn_8018AC64 (136), fn_8018AD68 (128), fn_8018ACEC (124), fn_8018AA68 (112),
 *   fn_80186960 (104), fn_80189BE0 (80), fn_80189C30 (76), fn_80189C7C (48), fn_8018A974 (48),
 *   fn_8018AB64 (48), fn_801861E4 (20).
 *   `fn_80183040` is the one that is understood but not finished: it builds a 0xC-byte helper through
 *   `__nw__FUl` + `fn_80183440`, writes the 0x328..0x35F block (0xEAAC/0xEE3A/0x11C7 and their
 *   u16/u8 tails) and then dispatches on `team` 0x10/0x11/0x15; `fn_80183040`'s store block needs
 *   the `lis r3,1; subi r0,r3,imm` constant-materialisation shape before it can be written down.
 *   `fn_80183718` and `fn_801837B0` are this unit's own tail targets of `fn_80183AA0`/`fn_80184488`
 *   and are declared (not defined) above, so those two dispatchers already measure 100 %.
 *
 * Callees.  Everything the written bodies call is declared where it belongs (rule 2): the unsplit
 * enemy band in `include/unsplit/enemy.h`, the owner units in `include/enemy/fn_801251D0.h`,
 * `include/enemy/fn_8012BDF4.h`, `include/enemy/fn_80138074.h`, `include/enemy/fn_80147CE0.h`,
 * `include/ef/fn_80105314.h`, `include/mh3_pad.h`, `include/sys_mem.h`.  Four declarations were moved
 * into those owner headers by this unit and are filed as `shared-file` config requests:
 * `fn_80126324` + `fn_80128030` (owner `enemy/fn_801251D0.cpp`), `fn_8012E664` + `fn_8012E694`
 * (owner `enemy/fn_8012BDF4.cpp`), plus `fn_801337FC`, `f32 fn_8013026C(_ENEMY_WORK*)` and
 * `void fn_8012FE3C(_ENEMY_WORK*, f32)` in the band header.  The two callees with no
 * registered owner and no resolvable module band (`fn_802B0668`, `fn_80191598` - the lint's counted
 * "address band interleaves modules" gap) are declared locally, as are the pool literals.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "unsplit/enemy.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80147CE0.h"
#include "fn_8004CAD8.h"
#include "ef/fn_80105314.h"
#include "mh3_pad.h"
#include "sys_mem.h"
#include "enemy/fn_8012EC74.h" /* fn_8013026C/fn_8012FE3C/fn_801337FC/fn_80136D4C */

extern "C" {

/* ------------------------------------------------------------------------------------------------
 * Callees with no registered owner (the lint's counted "band interleaves modules" gap).
 * ------------------------------------------------------------------------------------------------ */

/* 0x802B0668 - a byte table lookup (`lbzx` into `lbl_805CED40`, `0xFF` meaning "no entry", in which
 * case the argument is returned unchanged).  The three landed consumers declare three different
 * return types for it; the call here masks the result to a byte itself, so the wider view is the
 * one this range's target was built with (it carries the `clrlwi r0,r3,24`). */
u32 fn_802B0668(u32 kind);

/* 0x80191598 - two `u8*` outputs of a `self` (the target passes `&self->action` / `&self->state_sub`
 * and reads the two bytes back). */
void fn_80191598(_ENEMY_WORK* self, u8* out_a, u8* out_b);

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions: declared up front so the dispatchers can tail-call them.
 * ------------------------------------------------------------------------------------------------ */

void fn_80183718(_ENEMY_WORK* self);
void fn_801837B0(_ENEMY_WORK* self);
void fn_801841B4(_ENEMY_WORK* self);
void fn_80184280(_ENEMY_WORK* self);
void fn_801842FC(_ENEMY_WORK* self);
void fn_801843D4(_ENEMY_WORK* self);
void fn_8018454C(_ENEMY_WORK* self, u32 arg);
void fn_8018460C(_ENEMY_WORK* self, u32 arg);
void fn_801846BC(_ENEMY_WORK* self, u32 arg1, u32 arg2);
void fn_8018479C(_ENEMY_WORK* self);
void fn_8018484C(_ENEMY_WORK* self, u32 arg1, u32 arg2);
void fn_8018493C(_ENEMY_WORK* self, u32 arg);
void fn_801844B8(_ENEMY_WORK* self);
void fn_80184A5C(_ENEMY_WORK* self);
void fn_80184B0C(_ENEMY_WORK* self);
void fn_80184B78(_ENEMY_WORK* self);
void fn_80184434(_ENEMY_WORK* self);
void fn_80184458(_ENEMY_WORK* self);

/* ------------------------------------------------------------------------------------------------
 * Pool literals owned by the data pass (module unresolved -> declared, never defined; playbook 29).
 * ------------------------------------------------------------------------------------------------ */

extern f32 lbl_80797E88;
extern f32 lbl_80797E9C;
extern f32 lbl_80797EA0;
extern f32 lbl_80797EA4;
extern f32 lbl_80797EA8;
extern f32 lbl_80797EAC;
extern f32 lbl_80797EB0;
extern f32 lbl_80797EB4;
extern f32 lbl_80797EB8;
extern f32 lbl_80797EC0;
extern f32 lbl_80797EC4;
extern f32 lbl_80797EC8;
extern f32 lbl_80797ECC;
/* The two action tables `fn_8018454C` picks between (`arg` 1 selects the second). */
extern u32 lbl_8056FF50[];
extern u32 lbl_8056FF90[];
/* The action table `fn_80184CE0` arms. */
extern u32 lbl_80570010[];
extern u32 lbl_8056FFD0[];
extern f32 lbl_80797ED0;
extern f32 lbl_80797ED4;
extern f32 lbl_80797ED8;
extern f32 lbl_80797EDC;
extern f32 lbl_80797EE0;
extern f32 lbl_80797EE4;
extern f32 lbl_80797EE8;
extern f32 lbl_80797EEC;
extern f32 lbl_80797F00;
extern f32 lbl_80797F04;
/* The vtable word the 0xC-byte helper's constructors install at +0 (map: `.data`, no module). */
extern u32 lbl_805AD340[];

}

/* ------------------------------------------------------------------------------------------------
 * fn_80182D5C - per-(map,area) motion start: two timed motion sets, then the area's pair.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80182D5C(_ENEMY_WORK* self) {
    {
        f32 t = lbl_80797E88;
        fn_8012FCC4(self, 0, t);
    }
    {
        f32 t = lbl_80797E9C;
        fn_8012FCC4(self, 0x1E, t);
    }

    switch ((u8)fn_802B0668(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 5:
            fn_80126324(self, 6, 7, lbl_80797EA0);
            break;
        case 6:
            fn_80126324(self, 9, 10, lbl_80797EA4);
            break;
        case 7:
            fn_80126324(self, 0xD, 0xE, lbl_80797EA0);
            break;
        case 8:
            fn_80126324(self, 8, 9, lbl_80797EA0);
            break;
        case 0xC:
            fn_80126324(self, 0x19, 0x1A, lbl_80797EA0);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 1:
            fn_80126324(self, 6, 7, lbl_80797EA8);
            break;
        case 2:
            fn_80126324(self, 7, 8, lbl_80797EA0);
            break;
        case 3:
            fn_80126324(self, 9, 10, lbl_80797EAC);
            break;
        case 4:
            fn_80126324(self, 5, 6, lbl_80797EA0);
            break;
        case 5:
            fn_80126324(self, 7, 8, lbl_80797EA0);
            break;
        case 6:
            fn_80126324(self, 0xD, 0xE, lbl_80797EAC);
            break;
        case 7:
            fn_80126324(self, 0, 2, lbl_80797EA0);
            break;
        case 8:
            fn_80126324(self, 6, 7, lbl_80797EA0);
            break;
        case 10:
            fn_80126324(self, 0, 1, lbl_80797EAC);
            break;
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182F60 - whether this map/area pair has a motion set at all.
 * ------------------------------------------------------------------------------------------------ */

extern "C" u32 fn_80182F60(_ENEMY_WORK* self) {
    switch ((u8)fn_802B0668(self->field_0x1E0)) {
    case 1:
        if ((u32)(self->area_no - 5) <= 1U || (s32)self->area_no == 8) {
            return 1;
        }
        break;
    case 3:
        if ((s32)self->area_no == 1 || (s32)self->area_no == 5 || (s32)self->area_no == 7
            || (s32)self->area_no == 10) {
            return 1;
        }
        break;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182FF8 - arm the motion that this map/area pair starts with.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80182FF8(_ENEMY_WORK* self) {
    fn_80130478(self, 4);
    fn_80128AAC(self, 6, 5);
    fn_80133BB4(self);
}


/* ------------------------------------------------------------------------------------------------
 * fn_80183440 - the 0xC-byte helper's second constructor: base first, then this class's vtable.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void* fn_80183440(void* self) {
    fn_80147E2C(self);
    *(void**)self = lbl_805AD340;
    return self;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183A54 - the map-0x15 (teardown) step: run it once `em_die_ck` and the state check agree.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80183A54(_ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        if (fn_801337FC(self) == 1) {
            fn_8012E664(self);
        }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183AA0 - per-team step dispatch (`team` 0x10/0x11/0x15).
 * ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------
 * fn_80183AD0 / fn_80183B4C / fn_80183BC8 / fn_80183C44 / fn_80183CC4 - the armed-motion steps.
 * Each is the same two-phase action: phase 0 latches the state byte, arms the action and hands the
 * motion pair to the action setter; phase 1 waits for `fn_8012F93C` and runs the matching finish
 * call.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80183AD0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F62C(self, 1, 4, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

extern "C" void fn_80183B4C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F62C(self, 0x14, 4, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

extern "C" void fn_80183BC8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F62C(self, 0x1D, 4, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

extern "C" void fn_80183C44(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F5C4(self, 0x28, 0x14, 0, 3);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80183CC4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F5C4(self, 0x36, 0x14, 0, 3);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80183D44(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 4);
        fn_8012F62C(self, 0xCA, 6, 0);
        fn_801303EC(self, lbl_80797E88);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183AA0 - per-`team` step dispatch into the five armed-motion steps above (tail calls).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80183AA0(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_80183718(self);
        return;
    case 0x11:
        fn_801837B0(self);
        return;
    case 0x15:
        fn_80183A54(self);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183DCC - per-`state_sub` step dispatch into the same five steps (tail calls).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80183DCC(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80183AD0(self);
        return;
    case 1:
        fn_80183B4C(self);
        return;
    case 2:
        fn_80183BC8(self);
        return;
    case 4:
        fn_80183C44(self);
        return;
    case 5:
        fn_80183CC4(self);
        return;
    case 7:
        fn_80183D44(self);
        return;
    }
}


/* ------------------------------------------------------------------------------------------------
 * fn_80183E20 - the armed-motion step with the (0x1A, 0xA) pair.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80183E20(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1A, 0xA, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183E9C - the four-phase armed-motion step: arm, run the sub-action and its 0x708-frame timer,
 * then count it down and finish.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80183E9C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 8, 6, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            u8 state = self->state;
            self->state = state + 1;
            fn_8012F5B8(self, 4, 0, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2: {
        fn_8013221C(self, lbl_80797EC0, 1, 0xF);
        s32 left = self->timer_0x020 - 1;
        self->timer_0x020 = left;
        if (left <= 0) {
            u8 state = self->state;
            self->state = state + 1;
            fn_8012F5B8(self, 5, 4, 0);
            fn_80132264(self);
        }
        break;
    }
    case 3:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183FB8 / fn_80184038 / fn_801840B4 / fn_80184138 - more armed-motion steps, same two phases.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80183FB8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F504(self, 0x14, 0x14, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

extern "C" void fn_80184038(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 7, 4, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

extern "C" void fn_801840B4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 6, 2, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128A14(self, 1, 7);
        }
        break;
    }
}

extern "C" void fn_80184138(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1D, 4, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801841B4 - the armed-motion step with two frame checks and its finish step.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801841B4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xC9, 4, 0);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797EC4, lbl_80797E88) == 1) {
            fn_80136D14(self);
            if (em_frame_check(self, 1, lbl_80797EA4, lbl_80797E88) == 1) {
                fn_80131DB4(self);
                fn_80131DF4(self);
            }
        }
        if (fn_8012F93C(self) == 1) {
            fn_8012E694(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184280 / fn_801842FC - the last two armed-motion steps of the second family.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80184280(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xB, 6, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

extern "C" void fn_801842FC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xC9, 6, 0);
        fn_801303EC(self, lbl_80797E88);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797EC8, lbl_80797E88) == 1) {
            fn_80136D14(self);
            if (em_frame_check(self, 1, lbl_80797ECC, lbl_80797E88) == 1) {
                fn_80131DB4(self);
                fn_80131DF4(self);
            }
        }
        if (fn_8012F93C(self) == 1) {
            fn_8012E694(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801843D4 / fn_80184434 / fn_80184458 - the three per-`state_sub` dispatchers of the second
 * step family (tail calls).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801843D4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80183E20(self);
        return;
    case 1:
        fn_80183E9C(self);
        return;
    case 2:
        fn_80183FB8(self);
        return;
    case 3:
        fn_80184038(self);
        return;
    case 4:
        fn_801840B4(self);
        return;
    case 5:
        fn_80184138(self);
        return;
    case 7:
        fn_80184280(self);
        return;
    }
}

extern "C" void fn_80184434(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_80183FB8(self);
        return;
    case 6:
        fn_801841B4(self);
        return;
    }
}

extern "C" void fn_80184458(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_80183FB8(self);
        return;
    case 3:
        fn_80184038(self);
        return;
    case 8:
        fn_801842FC(self);
        return;
    }
}


/* ------------------------------------------------------------------------------------------------
 * fn_80184488 - per-`team` dispatch into the second step family (tail calls).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80184488(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_801843D4(self);
        return;
    case 0x11:
        fn_80184434(self);
        return;
    case 0x15:
        fn_80184458(self);
        return;
    }
}


/* ------------------------------------------------------------------------------------------------
 * fn_801844B8 - the armed-motion step that hands a float to `fn_80134004` and waits on
 * `fn_80134114`.  The call's argument order is the band header's ABI-equivalent `(u32, f32)` view
 * (see the header's residual note).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801844B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x15, 4, 0);
        fn_80134004(self, 0, lbl_80797E88);
        break;
    }
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018454C - the same step, but the argument picks one of two action tables.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8018454C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_80134964(self, (u8)arg == 1 ? lbl_8056FF90 : lbl_8056FF50, 0, 0, 0);
        break;
    }
    case 1:
        if (fn_80134B0C(self, (u8)arg == 1 ? lbl_8056FF90 : lbl_8056FF50) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018460C - the armed-motion step whose phase 1 gates `fn_80133C50` on an argument and a frame
 * check.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8018460C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1B, 6, 0);
        break;
    }
    case 1:
        if ((u8)arg == 1 && em_frame_check(self, 3, lbl_80797ED0, lbl_80797ED4) == 1) {
            fn_80133C50(self, 0x100);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801846BC - the armed-motion step whose argument picks the motion pair and whether the target
 * distance is clamped.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801846BC(_ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        u32 motion = (u8)arg1 == 1 ? 9 : 2;
        fn_8012F5B8(self, (u16)motion, 0xA, 0);
        fn_80134004(self, 0, lbl_80797E88);
        if ((u8)arg2 == 1 && self->value_0x378 > lbl_80797ED8) {
            self->value_0x378 = lbl_80797ED8;
        }
        break;
    }
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018479C - the armed-motion step shared by two sub-states (`fn_8012CF20`/`fn_80131E74` first).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8018479C(_ENEMY_WORK* self) {
    fn_8012CF20(self);
    fn_80131E74(self);

    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 2, 0xA, 0);
        fn_80134004(self, 0, lbl_80797E88);
        break;
    }
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80128A14(self, 5, 5);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018484C - the armed-motion step whose argument picks the motion pair, the blend float and the
 * `fn_80134114` mask.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8018484C(_ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x59, 0xA, 0);
        fn_8012933C(self, 0, 0xD, 2);

        f32 blend;
        switch ((u8)arg2) {
        default:
            blend = lbl_80797E88;
            break;
        case 1:
            blend = lbl_80797EDC;
            break;
        case 2:
            blend = lbl_80797EE0;
            break;
        }
        fn_80134004(self, 0, blend);
        break;
    }
    case 1:
        if (fn_80134114(self, 0, (u16)((u8)arg1 == 1 ? 0xC0 : 0x40)) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018493C - the armed-motion step with the 0x96/0x5A countdown derived from `bits_0x1EC`.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_8018493C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x59, 4, 0);

        u32 ticks = self->bits_0x1EC & 0x1F;
        self->timer_0x020 = ticks + 0x96;

        f32 blend;
        switch ((u8)arg) {
        default:
            blend = lbl_80797E88;
            break;
        case 1:
            blend = lbl_80797EE4;
            break;
        case 2:
            blend = lbl_80797E88;
            break;
        case 3:
            blend = lbl_80797EE4;
            self->timer_0x020 = ticks + 0x5A;
            break;
        }
        fn_80134004(self, 0, blend);
        break;
    }
    case 1: {
        u32 done = 0;
        if ((u8)arg != 2) {
            s32 left = self->timer_0x020 - 1;
            self->timer_0x020 = left;
            if (left <= 0) {
                done = 1;
            }
        }
        if (fn_80134114(self, 0, 0x40) == 1 || done == 1) {
            fn_80127F48(self);
        }
        break;
    }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184A5C / fn_80184B0C / fn_80184B78 - the three per-`state_sub` dispatchers of the third step
 * family (tail calls; the dense ones are jump tables).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80184A5C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801844B8(self);
        return;
    case 1:
        fn_8018454C(self, 0);
        return;
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 4:
        fn_801846BC(self, 1, 0);
        return;
    case 5:
        fn_8018479C(self);
        return;
    case 6:
        fn_8018454C(self, 0);
        return;
    case 7:
        fn_8018484C(self, 0, 1);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 9:
        fn_8018484C(self, 1, 0);
        return;
    case 10:
        fn_801846BC(self, 0, 1);
        return;
    case 11:
        fn_801846BC(self, 1, 1);
        return;
    case 13:
        fn_8018484C(self, 0, 0);
        return;
    case 16:
        fn_8018484C(self, 0, 2);
        return;
    }
}

extern "C" void fn_80184B0C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 12:
        fn_8018493C(self, 1);
        return;
    case 14:
        fn_8018493C(self, 0);
        return;
    case 15:
        fn_8018493C(self, 2);
        return;
    case 17:
        fn_8018454C(self, 1);
        return;
    case 18:
        fn_8018493C(self, 3);
        return;
    }
}

extern "C" void fn_80184B78(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 12:
        fn_8018493C(self, 1);
        return;
    case 14:
        fn_8018493C(self, 0);
        return;
    case 15:
        fn_8018493C(self, 2);
        return;
    case 17:
        fn_8018454C(self, 1);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184CE0 - the armed-motion step with the `field_0x482`-selected approach float.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80184CE0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_80570010, 0, 1, 0);
        break;
    }
    case 1:
        if (fn_80134B0C(self, lbl_80570010) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797EE8);
        } else {
            fn_80136D4C(self, lbl_80797EEC);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184BF8 - per-`team` dispatch into the third step family (tail calls).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80184BF8(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_80184A5C(self);
        return;
    case 0x11:
        fn_80184B0C(self);
        return;
    case 0x15:
        fn_80184B78(self);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184C28 - fn_80184CE0's sibling with the other action table.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80184C28(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_8056FFD0, 0, 1, 0);
        break;
    }
    case 1:
        if (fn_80134B0C(self, lbl_8056FFD0) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797EE8);
        } else {
            fn_80136D4C(self, lbl_80797EEC);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801856B8 / fn_80185738 / fn_801857B8 / fn_80185838 / fn_801858B8 - the fifth family's armed
 * motion steps: arm the motion with the action setter, then finish on `fn_8012F93C`.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801856B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x2E, 0x14, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80185738(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x35, 0x14, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801857B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x32, 0x14, 0, 3);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80185838(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x31, 0x14, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801858B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F5C4(self, 0x28, 0x14, 0, 3);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80185BEC / fn_80185FA0 - two more of the same family.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80185BEC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x3A, 0xA, 0, 1);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80185FA0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x36, 0x14, 0, 3);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80185D60 - the team-selected motion step (`fn_8012CF20` runs first).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80185D60(_ENEMY_WORK* self) {
    fn_8012CF20(self);

    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 0);
        fn_8012F5B8(self, (u16)(self->team == 0x10 ? 0x3D : 0x3B), 0, 0);
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80185B0C - the motion step that stores the target angle byte: the difference between the body
 * angle and `field_0x1C0`, quantised into `state_0x007` (0 / 0x80 / the >>8 byte).
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_80185B0C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x2D, 0xA, 0);

        u32 diff = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (diff > 0x8000) {
            if (diff > 0xC000) {
                self->state_0x007 = 0;
            } else {
                self->state_0x007 = 0x80;
            }
        } else {
            self->state_0x007 = (u8)((s32)diff >> 8);
        }
        break;
    }
    case 1:
        fn_80133E3C(self, (s32)(self->state_0x007 << 8), lbl_80797EB4, lbl_80797ECC);
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801850F8 - the motion step that runs the angle-driven approach: two `em_frame_check` gates and
 * a clamped blend of `fn_8012F8EC`'s value.
 * ------------------------------------------------------------------------------------------------ */

extern "C" void fn_801850F8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x39, 0xA, 0, 1);
        break;
    }
    case 1:
        fn_80136D4C(self, lbl_80797EEC);

        if (em_frame_check(self, 1, lbl_80797F00, lbl_80797E88) == 1) {
            f32 blend = lbl_80797EEC * (fn_8012F8EC(self) - lbl_80797EB4);
            if (blend > lbl_80797EA4) {
                blend = lbl_80797EA4;
            }
            fn_8012FE3C(self, blend + fn_8013026C(self));
        }

        if (em_frame_check(self, 1, lbl_80797F04, lbl_80797E88) == 1) {
            fn_80133C50(self, 0x30);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}
