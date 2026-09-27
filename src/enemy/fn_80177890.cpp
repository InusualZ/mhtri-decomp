/* enemy/fn_80177890.cpp - the enemy motion-state update set, `.text` 0x80177890..0x80178128.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py grep '80177890'` - every symbol in the range is a bare
 * `fn_XXXXXXXX = .text:0x...` entry with no real name).
 *
 * What it is.  The twelve functions are one enemy's per-motion update table plus the dispatcher that
 * selects them:
 *
 *   * `fn_80177BA4` switches on `_ENEMY_WORK::state_sub` (at +0x1E6) and tail-branches into the state
 *     updates; entries 0 and 1 are `fn_80177608`/`fn_80177774`, which belong to the neighbouring TU
 *     (`proposal/80177608_fn_80177608`), so the dispatcher is the seam's only evidence that the twelve
 *     are one unit.
 *   * `fn_80177890`, `fn_8017791C`, `fn_8017799C`, `fn_80177A2C`, `fn_80177AA8`, `fn_80177B24`,
 *     `fn_80177CC8` are single-step state machines: `case 0` bumps `state` (+0x5) and arms a motion
 *     through `fn_8012F504`/`fn_8012F5B8`; `case 1` polls a condition (`em_frame_check`, `fn_8012F93C`,
 *     `fn_80134B0C`) and calls `fn_80127F48` (the motion-change commit) when it returns 1.
 *   * `fn_80177BEC`, `fn_80177F30`, `fn_8017801C` take a second `u8` argument that selects which float
 *     pool constant `fn_80134004` should ease the enemy toward.
 *   * `fn_80177D54` is the compound one: three interleaved sub-steps (`state_0x006`, `state_0x007`)
 *     gated on the per-frame flags at +0xA0D / +0xA69, then the same `em_frame_check`/`fn_8012F93C` pair.
 *
 * Language.  The range's only call into another module is `em_frame_check(ENEMY_WORK*, u16, f32, f32)`,
 * which the map carries mangled (`em_frame_check__FP11_ENEMY_WORKUsff`); every other callee is a plain
 * `fn_XXXXXXXX` C symbol.  The unit is therefore C++ (the mangled callee is declared with its real
 * signature and mangled by this front-end, docs/plan.md 6.5 rule 9) and the file is `.cpp`.
 *
 * Types.  `_ENEMY_WORK` is the shared 0xB18 record; it lives in `include/enemy/ENEMY_WORK.h` so it is
 * defined once (rule 1).  Fields the twelve functions touch are named from their call sites; the rest is
 * `unused_0xNN` padding that keeps every measured offset in place.
 *
 * Data.  The pool floats (`lbl_80797B18`..) and the string block `lbl_8056FDD0` are owned elsewhere and
 * are only declared, never defined (playbook 29), so the load operands pair with the target's pool
 * references.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"

/* ------------------------------------------------------------------------------------------------ *
 * shared pool symbols (another unit owns the bytes)
 * ------------------------------------------------------------------------------------------------ */

extern f32 lbl_80797B18;
extern f32 lbl_80797B48;
extern f32 lbl_80797B4C;
extern f32 lbl_80797B50;
extern f32 lbl_80797B54;
extern f32 lbl_80797B58;
extern f32 lbl_80797B5C;
extern u8 lbl_8056FDD0[];

/* ------------------------------------------------------------------------------------------------ *
 * the enemy-band callees (unsplit: their addresses sit between the registered `enemy/` units, so the
 * declarations live here until the owning unit is registered; docs/plan.md 6.5 rule 2, the unsplit gap)
 *
 * The map carries every one of these as a plain C symbol (`fn_80177890`, `fn_80130478`, ...), so the
 * whole set - definitions included - is `extern "C"`: without it this C++ front-end would mangle them
 * (`fn_80177890__FP11_ENEMY_WORK`) and objdiff would pair nothing.
 * ------------------------------------------------------------------------------------------------ */

/* The one callee in another module.  Declared with its real signature (before the linkage block) so
 * this front-end mangles it to the map's `em_frame_check__FP11_ENEMY_WORKUsff`; rule 9 forbids writing
 * that mangled spelling as the callable identifier. */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

#ifdef __cplusplus
extern "C" {
#endif

void fn_80130478(_ENEMY_WORK* self, u32 a);
void fn_8012F504(_ENEMY_WORK* self, s32 a, s32 b, s32 c, s32 d);
void fn_8012F5B8(_ENEMY_WORK* self, s32 a, s32 b, s32 c);
void fn_8012F810(_ENEMY_WORK* self);
u32 fn_8012F93C(_ENEMY_WORK* self);
void fn_80127F48(_ENEMY_WORK* self);
void fn_80128A14(_ENEMY_WORK* self, s32 a, s32 b);
void fn_8012933C(_ENEMY_WORK* self, s32 a, s32 b, s32 c);
void fn_80133C50(_ENEMY_WORK* self, u32 a);
void fn_80134004(_ENEMY_WORK* self, f32 a, s32 b);
u32 fn_80134114(_ENEMY_WORK* self, s32 a, s32 b);
void fn_80134964(_ENEMY_WORK* self, void* tbl, s32 a, s32 b, s32 c);
u32 fn_80134B0C(_ENEMY_WORK* self, void* tbl);
void fn_801823A0(_ENEMY_WORK* self, s32 a);

/* the two neighbouring-TU entries the dispatcher tail-branches into */
void fn_80177608(_ENEMY_WORK* self);
void fn_80177774(_ENEMY_WORK* self);

/* ------------------------------------------------------------------------------------------------ *
 * the motion-state updates
 * ------------------------------------------------------------------------------------------------ */

void fn_80177890(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F504(self, 0x14, 0x14, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80797B48, lbl_80797B18) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_8017791C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F504(self, 7, 0xa, 0, 1);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* `fn_8017799C` keeps the countdown's `subi`; with the unit peephole on it fuses into `subic.` and
 * drops the retail `cmpwi`, so the pragma is scoped to this one function (a per-unit deviation). */
#pragma peephole off
void fn_8017799C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 6, 0xa, 0);
        self->timer_0x020 = 0x12c;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            fn_80128A14(self, 1, 6);
        }
        break;
    }
}
#pragma peephole on

void fn_80177A2C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x82, 0xa, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80177AA8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xa, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80177B24(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F504(self, 0x1d, 0x14, 0, 1);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The dispatcher: `state_sub` selects the active motion update.  Cases 0/1 are the neighbouring TU's
 * entries, so the switch is dense 0..7 and MWCC lowers it to the `.data` jump table the target carries. */
void fn_80177BA4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80177608(self);
        break;
    case 1:
        fn_80177774(self);
        break;
    case 2:
        fn_80177890(self);
        break;
    case 3:
        fn_8017791C(self);
        break;
    case 4:
        fn_8017799C(self);
        break;
    case 5:
        fn_80177A2C(self);
        break;
    case 6:
        fn_80177AA8(self);
        break;
    case 7:
        fn_80177B24(self);
        break;
    }
}

void fn_80177BEC(_ENEMY_WORK* self, s32 arg) {
    fn_801823A0(self, 1);

    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F810(self);
        fn_8012F504(self, 0x15, 0x28, 0, 1);
        switch ((u8)arg) {
        default:
            fn_80134004(self, lbl_80797B18, 0);
            break;
        case 1:
            fn_80134004(self, lbl_80797B4C, 0);
            break;
        }
        break;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80177CC8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_80134964(self, lbl_8056FDD0, 0, 0, 0);
        break;
    case 1:
        if (fn_80134B0C(self, lbl_8056FDD0) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80177D54(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1b, 6, 0);
        fn_8012933C(self, 0, 0x2e, 8);
        fn_8012933C(self, 1, 0x32, 0x18);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                fn_8012933C(self, 0, 0x2f, 0x18);
            }
            break;
        case 1:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                fn_8012933C(self, 0, 0x30, 0x18);
            }
            break;
        case 2:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                fn_8012933C(self, 0, 0x31, 0x18);
            }
            break;
        }

        switch (self->state_0x007) {
        case 0:
            if (self->field_0xA69 == 0) {
                self->state_0x007++;
                fn_8012933C(self, 1, 0x33, 0x18);
            }
            break;
        case 1:
            if (self->field_0xA69 == 0) {
                self->state_0x007++;
                fn_8012933C(self, 1, 0x34, 0x18);
            }
            break;
        }

        if (em_frame_check(self, 1, lbl_80797B50, lbl_80797B54) == 1) {
            fn_80133C50(self, 0x100);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80177F30(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 2, 0xa, 0);
        switch ((u8)arg) {
        default:
            fn_80134004(self, lbl_80797B18, 0);
            break;
        case 1:
            fn_80134004(self, lbl_80797B18, 0);
            if (self->value_0x378 > lbl_80797B58) {
                self->value_0x378 = lbl_80797B58;
            }
            /* falls through to case 2's easing */
        case 2:
            fn_80134004(self, lbl_80797B5C, 0);
            break;
        }
        break;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_8017801C(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 9, 0xa, 0);
        switch ((u8)arg) {
        default:
            fn_80134004(self, lbl_80797B18, 0);
            break;
        case 1:
            fn_80134004(self, lbl_80797B4C, 0);
            break;
        case 2:
            fn_80134004(self, lbl_80797B18, 0);
            if (self->value_0x378 > lbl_80797B58) {
                self->value_0x378 = lbl_80797B58;
            }
            break;
        case 3:
            fn_80134004(self, lbl_80797B5C, 0);
            break;
        }
        break;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

#ifdef __cplusplus
}
#endif
