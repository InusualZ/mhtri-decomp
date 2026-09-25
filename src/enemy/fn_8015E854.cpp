/* enemy/fn_8015E854.cpp - the enemy action/state unit that follows the em003 block,
 * 0x8015E854..0x80165FC8 (55 functions).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/dumpmap.py lookup 0x8015E854`: the shared runtime dump answers only
 * `zz_015e854_`, and `config/RMHE08/symbols.txt` carries nothing but the bare `fn_XXXXXXXX`
 * entries for the range - no `__FILE__`/class string names a file here).
 *
 * What it is.  A continuation of the same enemy-band action family as `enemy/fn_801550FC.cpp`:
 * three jump-table dispatchers keyed on the byte at +0x1E6 (`state_sub`)
 * - fn_8015F23C, fn_801620A4 and fn_80163298 - fan out to the range's per-action steps, and the
 * steps are the usual frame/timer gate + `em_frame_check`/`fn_8012F93C` shape over the shared
 * `_ENEMY_WORK` record.  The range drives the same enemy-band helpers
 * (`fn_80130478`, `fn_8012F5B8`, `fn_80134004`, `fn_80134114`, `fn_80134964`, `fn_80134B0C`,
 * `get_em_chg_scale`, `em_frame_check`, `em_after_frame_check`, `setVector3`, ...).
 *
 * The seam is unproven (docs/plan.md 8.3).  The range's own `tudiscover` run has no strong cut:
 * the set it returns is a single function (fn_8015E854) and both edges are weak.  It is NOT a
 * continuation of `enemy/fn_801550FC.cpp` (that unit's `.text` ends at 0x8015D860, 0xFF4 bytes
 * below this range's first function, and its own header records em008 taking over after it) and
 * it is not contiguous with it, so this registers a new unit at the proposal's range.  The
 * dispatcher fn_80163298 calls fn_8015E05C/fn_8015E804 below the range and fn_80162500 calls
 * fn_8015D860 (both in the still-unclaimed band 0x8015D860..0x8015E854), so the real translation
 * unit plausibly extends below this range; the boundary is recorded as provisional and those
 * neighbours are the follow-up queue.
 *
 * Language: C++ (every callee the range reaches is a C++ mangling - `em_frame_check__FP11_ENEMY_WORKUsff`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`, `get_em_chg_scale__FP11_ENEMY_WORK` - and the range
 * calls them through their real signatures, rule 9).  The flat `fn_*` symbols are `extern "C"`
 * so objdiff pairs them by name.
 *
 * Object: `_ENEMY_WORK`, included from `include/enemy.h`.
 *
 * Flags: this unit needs no deviation - the `enemy` lib's `cflags_main` (see the block's comment
 * in `configure.py`) measured every body below.  No `#pragma` is used.
 *
 * State of the reconstruction.  The 29 bodies below cover the range's state steps, its three
 * jump-table dispatchers (fn_8015F23C, fn_801620A4, fn_8015FCB0) and the small actor
 * predicates/wrappers, in address order; 28 of the 29 measure >= 80 % (the official report
 * metric), the exception is fn_80165C64 (65.7 %).  The remaining 26 functions are the follow-up
 * queue, below.
 *
 * Residuals, per shape (the measurement lives in the outbox, not here; playbook rows in
 * `docs/matching.md`):
 *   * fn_80165C64 (65.7 %, 96 B vs target 108 B): the target tests
 *     `(em_parts_damage_level_get(self, arg) & 1)` with an explicit
 *     `clrlwi r0,r3,24 / clrlwi r0,r0,31 / cmpwi / bne` pair, while every spelling tried
 *     (`== 0 return 1`, `!= 0 return 0`, a `u32`/`u8` temp, an if/else-if on the byte-narrowed
 *     argument) makes MWCC fold it to `clrlwi. r0,r3,31 / xori r3,r0,1` and reorder the two
 *     condition blocks; same source shape, 12 bytes shorter.  Recorded, not a flag problem.
 *   * fn_8015F23C (99.97 %) is byte-identical in size; only the table's unused 15..21 entries
 *     differ by one slot.  Residual only.
 *   * fn_8015F510 (92.6 %) saves f31 through the paired-single idiom the target uses
 *     (`stfd f31,.. ; psq_st f31,.. ; psq_lx f31`) where our command line emits `stfd`/`lfd`
 *     only - the stopping rule's shape (`docs/matching.md`: 785 such instructions, none
 *     matching).  Recorded; fn_8015ED94 has the same prologue and is unwritten.
 *   * the big per-motion effect/keyframe drivers fn_8015FD84 (0x18DC), fn_80163A48 (0x1F18),
 *     fn_80162500 (0xCC4) and fn_80162154 (0x2E8) call `em_frame_check` + `setVector3` +
 *     `fn_80304508`/`fn_801635A4` hundreds of times in straight-line keyframe streams; they need
 *     the keyframe tables (the `.data` records at 0x805A5E78/0x805A5EB4 and friends) recovered
 *     first.  The rest of the follow-up queue: fn_8015EB44, fn_8015ED94, fn_8015F0E8, fn_8015F820,
 *     fn_8015F8E4, fn_8015F9FC, fn_8015FAC0, fn_8015FB78, fn_8015FD1C, fn_80161660, fn_80162154,
 *     fn_8016243C, fn_801631C4, fn_80163298, fn_80163344, fn_801634F8, fn_801635A4, fn_801639C0,
 *     fn_80165964, fn_80165C14, fn_80165CD0.
 *   * fn_80161660 and fn_8015ED94 also need a VEC3 at +0x31C that `include/enemy.h` currently
 *     spells `pad_0x31C[4]` before its `v_0x320` VEC3 - a genuine two-view clash (this unit's
 *     actors read a vector at +0x31C; `enemy/fn_8014A1BC.c` reads `v_0x320` at +0x320), so the
 *     type needs a union before those bodies can be written (rule 1/3).  Left for the follow-up.
 */

#include "types.h"
#include "enemy.h"

extern "C" {
#include "unsplit/enemy.h"
}
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "fn_8004CAD8.h"

/* the C++-mangled callees, through their real signatures (rule 9) */
extern s32 calcVecAng2(VEC3*, VEC3*);
extern u32 em_frame_check(_ENEMY_WORK*, u16, f32, f32);
extern u32 em_parts_damage_level_get(_ENEMY_WORK*, u8);
extern f32 get_em_chg_scale(_ENEMY_WORK*);
extern void rotVecY(VEC3*, u32);
extern void setVector3(VEC3*, f32, f32, f32);

/* the unit's own range's flat symbols (the band below it is declared in `unsplit/enemy.h`) */
extern "C" {
extern void fn_8015EA24(_ENEMY_WORK*);
extern void fn_8015EAC8(_ENEMY_WORK*);
extern void fn_8015EB44(_ENEMY_WORK*);
extern void fn_8015ED00(_ENEMY_WORK*);
extern void fn_8015ED94(_ENEMY_WORK*);
extern void fn_8015EED4(_ENEMY_WORK*);
extern void fn_8015EFAC(_ENEMY_WORK*);
extern void fn_8015F0E8(_ENEMY_WORK*);
extern void fn_8015F6C4(_ENEMY_WORK*, u8);
extern void fn_8015FD84(_ENEMY_WORK*, u8);
extern void fn_80161660(_ENEMY_WORK*);
extern void fn_80161784(_ENEMY_WORK*, u8);
extern void fn_801618A4(_ENEMY_WORK*);
extern void fn_8016198C(_ENEMY_WORK*, u8);
extern void fn_80161A34(_ENEMY_WORK*, u8);
extern void fn_80161D24(_ENEMY_WORK*);
extern void fn_80161DCC(_ENEMY_WORK*, u8);
extern void fn_80161E94(_ENEMY_WORK*, u8);
extern void fn_8015F820(_ENEMY_WORK*);
extern void fn_8015F8E4(_ENEMY_WORK*);
extern void fn_8015F9FC(_ENEMY_WORK*);
extern void fn_8015FAC0(_ENEMY_WORK*);
extern void fn_8015FB78(_ENEMY_WORK*);
extern void fn_80162500(_ENEMY_WORK*);
extern void fn_801631C4(_ENEMY_WORK*);
extern void fn_8015E854(_ENEMY_WORK* self, u8 arg1, u8 arg2);
extern void fn_801624E4(_ENEMY_WORK* self);
extern void fn_801624EC(_ENEMY_WORK* self);
extern void fn_80163274(_ENEMY_WORK* self);
extern void fn_80165960(void);
extern s32 fn_80165BC0(void);
extern void fn_80165BC8(_ENEMY_WORK*, u8*, u8*);
extern s32 fn_80165C20(_ENEMY_WORK*, u8);
extern s32 fn_80165C64(_ENEMY_WORK*, u32);
}

/* rodata pools this unit references (owned by the split, not defined here) */
extern u8 lbl_8056FB50[];
extern u8 lbl_8056FB90[];
extern u8 lbl_8056FBD0[];
extern f32 lbl_80797330;
extern f32 lbl_8079733C;
extern f32 lbl_80797360;
extern f32 lbl_80797364;
extern f32 lbl_80797368;
extern f32 lbl_8079736C;
extern f32 lbl_80797380;
extern f32 lbl_80797384;
extern f32 lbl_80797390;
extern f32 lbl_80797394;
extern f32 lbl_80797398;
extern f32 lbl_8079739C;
extern f32 lbl_807973A0;
extern f32 lbl_807973A4;
extern f32 lbl_807973A8;
extern f32 lbl_807973AC;
extern f32 lbl_807973B0;
extern f32 lbl_807973B4;
extern f32 lbl_807973B8;
extern f32 lbl_80797340;
extern f32 lbl_807973BC;
extern f32 lbl_807973C0;
extern f32 lbl_807973D8;
extern f32 lbl_807973DC;
extern f32 lbl_807973E0;
extern f32 lbl_807973E4;
extern f32 lbl_807973E8;

/* the range's own flat symbols the written bodies reach */
extern "C" {
}

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_8015E854(_ENEMY_WORK* self, u8 arg1, u8 arg2) {
    u8 temp_r4;
    u8 temp_a1;
    u8 temp_a2;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xA, 8, 0);
        temp_a1 = arg1;
        switch ((s32) temp_a1) {
        case 0:
            fn_80134004(self, 0, lbl_80797360);
            return;
        case 1:
            fn_80134004(self, 0, lbl_80797360);
            return;
        case 2:
            fn_80134004(self, 0, lbl_80797364);
            return;
        case 3:
            fn_80134004(self, 0, lbl_80797330);
            return;
        case 4:
            fn_80134004(self, 0, lbl_80797368);
            return;
        case 5:
            fn_80134004(self, 0, lbl_8079736C);
            return;
        case 6:
            fn_80134004(self, 0, lbl_80797364);
            return;
        }
        return;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1U) {
            if (fn_8012F948(self) == 0) {
                temp_a2 = arg2;
                switch ((s32) temp_a2) {
                case 0:
                    self->state_0x05 = self->state_0x05 + 1;
                    fn_8012F5B8(self, 0xB, 4, 0);
                    return;
                case 1:
                    fn_80127F48(self);
                    return;
                }
            }
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_8015EA24(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_80134964(self, lbl_8056FB50, 0, 1, 0);
        if (self->field_0x482 == 1U) {
            fn_8012F8C8(self, lbl_8079733C);
        }
        return;
    case 1:
        if (fn_80134B0C(self, lbl_8056FB50) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_8015EAC8(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x21, 4, 0);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_8015ED00(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x10, 0xA, 0);
        fn_80134004(self, 0, lbl_80797330);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_8016198C(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r5;
    f32 temp_f1;

    temp_r5 = self->state_0x05;
    switch ((s32) temp_r5) {
    case 0:
        self->state_0x05 = (u8) (temp_r5 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x16, 4, 0);
        self->field_0x20 = 0;
        return;
    case 1:
        if ((u8) arg1 == 0) {
            temp_f1 = lbl_807973E8;
        } else {
            temp_f1 = lbl_807973B0;
        }
        if (em_frame_check(self, 1, temp_f1, lbl_80797330) == 1U) {
            fn_8015DDB8(self);
        }
        return;
    }
}

extern "C" void fn_80162028(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x16, 4, 0x2C);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_8015DDB8(self);
        }
        return;
    }
}

extern "C" void fn_801624E4(_ENEMY_WORK* self) {
    fn_8015F6C4(self, 0);
}

extern "C" void fn_801624EC(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_801624E4(self);
    }
}

extern "C" void fn_80163274(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_80162500(self);
        return;
    case 1:
        fn_801631C4(self);
        return;
    }
}

extern "C" void fn_80165960(void) {
}

extern "C" s32 fn_80165BC0(void) {
    return 1;
}

extern "C" void fn_80165BC8(_ENEMY_WORK* self, u8* out_state, u8* out_flags) {
    fn_80130478(self, 4);
    *out_state = 0xC;
    *out_flags = 0;
}

extern "C" s32 fn_80165C20(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r4 = arg1;

    if ((u8) (temp_r4 - 3) <= 1U) {
        if (fn_8012EC60() == 1U) {
            return 1;
        }
    }
    return 0;
}

extern "C" s32 fn_80165C64(_ENEMY_WORK* self, u32 arg1) {
    u8 temp_r4;
    u8 temp_r0;
    u32 temp_r3;

    temp_r4 = (u8) arg1;
    if (temp_r4 <= 2U) {
        temp_r3 = em_parts_damage_level_get(self, temp_r4);
        temp_r0 = (u8) temp_r3;
        if ((temp_r0 & 1) == 0) {
            return 1;
        }
    } else if ((u32) (temp_r4 - 3) <= 1U) {
        if (fn_8012EC3C(self) == 1U) {
            return 1;
        }
    }
    return 0;
}

extern "C" void fn_8015EED4(_ENEMY_WORK* self) {
    u8 temp_r4;
    f32 temp_f1;
    f32 t1;
    f32 t2;
    f32 t3;
    f32 t4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x22, 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80797330, lbl_80797390) == 1U) {
            temp_f1 = fn_8012F8E4(self);
            t1 = lbl_8079739C * temp_f1;
            t2 = t1 * lbl_80797398;
            t3 = t2 / lbl_807973A0;
            t4 = lbl_80797394 + t3;
            fn_80133C50(self, (u16) (s32) t4);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_8015EFAC(_ENEMY_WORK* self) {
    f32* temp_r31;
    u8 temp_r3;
    f32 temp_f0;
    f32 temp_f1;

    temp_r31 = fn_80126454(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {
    case 0:
        self->state_0x05 = (u8) (temp_r3 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xA, 6, 0);
        temp_f1 = fn_80050EF4(&self->pos, &self->target);
        temp_f1 = lbl_807973A4 * temp_f1;
        temp_f0 = -temp_r31[1];
        if (temp_f1 > temp_f0) {
            temp_f1 = temp_f0;
        }
        fn_80134004(self, 0, temp_f1);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1U) {
            if (fn_8012F948(self) == 0) {
                self->state_0x05 = self->state_0x05 + 1;
                fn_8012F5B8(self, 0xB, 4, 0);
                fn_80134004(self, 0, lbl_80797330);
            }
        }
        return;
    case 2:
        fn_80134114(self, 0, 0x80);
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_801618A4(_ENEMY_WORK* self) {
    u8 temp_r4;
    f32 temp_f1;
    f32 t1;
    f32 t2;
    f32 t3;
    f32 t4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x13, 4, 0);
        fn_80129668(self, 0, 0xC);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80797330, lbl_80797384) == 1U) {
            temp_f1 = fn_8012F8E4(self);
            t1 = lbl_807973E4 * temp_f1;
            t2 = t1 * lbl_80797398;
            t3 = t2 / lbl_807973A0;
            t4 = lbl_80797394 + t3;
            fn_80133C50(self, (u16) (s32) t4);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_80161D24(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x18, 2, 0);
        fn_80129668(self, 0, 2);
        fn_80129668(self, 1, 0x10);
        fn_80130CDC(self, -0x1E);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_80161DCC(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1C, 2, 0);
        if ((u8) arg1 == 0) {
            fn_80129668(self, 0, 3);
        } else {
            fn_8012933C(self, 0, 0x14, 0x40);
        }
        fn_80130CDC(self, -0x1E);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_8015F2D8(_ENEMY_WORK* self) {
    u8 temp_r3;

    fn_8012CF20(self);
    fn_80131E74(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {
    case 0:
        self->state_0x05 = (u8) (temp_r3 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x26, 4, 0);
        fn_80129668(self, 0, 0xE);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_807973B4, lbl_80797330) == 1U) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F39C(_ENEMY_WORK* self) {
    u8 temp_r3;
    s32 temp_r0;

    fn_80131E74(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {
    case 0:
        self->state_0x05 = (u8) (temp_r3 + 1);
        fn_80130478(self, 4);
        fn_8012F62C(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        self->field_0x20 = 0x5A;
        return;
    case 1:
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 == 0) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F450(_ENEMY_WORK* self) {
    u8 temp_r3;

    fn_80131E74(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {
    case 0:
        self->state_0x05 = (u8) (temp_r3 + 1);
        fn_80130478(self, 4);
        fn_8012F5B8(self, 6, 0, 0);
        fn_80134004(self, 0, lbl_80797380);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (fn_80134114(self, 0, 0x100) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F510(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r3;
    f32 f31;

    f31 = lbl_80797330;
    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {
    case 0:
        self->state_0x05 = (u8) (temp_r3 + 1);
        fn_80130478(self, 4);
        fn_8012F5B8(self, 0xA, 0, 0);
        if ((u8) arg1 == 0) {
            f31 = lbl_807973B8;
        }
        fn_80134004(self, 0, f31);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (fn_80134114(self, 0, 0x100) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F60C(_ENEMY_WORK* self) {
    u8 temp_r3;

    fn_80131E74(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {
    case 0:
        self->state_0x05 = (u8) (temp_r3 + 1);
        fn_80130478(self, 4);
        fn_80134964(self, lbl_8056FB50, 0, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (fn_80134B0C(self, lbl_8056FB50) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F6C4(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r3;

    fn_8012CF20(self);
    fn_80131E74(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {
    case 0:
        self->state_0x05 = (u8) (temp_r3 + 1);
        fn_80130478(self, 4);
        if ((u8) arg1 == 0) {
            fn_8012F5B8(self, 0x27, 2, 0x24);
            fn_801303EC(self, lbl_807973BC);
            fn_80129668(self, 0, 0xF);
        } else {
            fn_8012F5B8(self, 0x27, 2, 0);
            fn_801303EC(self, lbl_80797330);
            fn_80129668(self, 0, 0x11);
        }
        fn_80136D14(self);
        return;
    case 1:
        if (em_frame_check(self, 2, lbl_807973C0, lbl_80797330) == 1U) {
            fn_80136D14(self);
        }
        if ((u8) arg1 == 0) {
            if (self->field_0x1ac >= lbl_80797330) {
                fn_801303EC(self, lbl_80797330);
            } else {
                fn_801303FC(self, lbl_80797340);
            }
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

extern "C" void fn_80161784(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r5;

    temp_r5 = self->state_0x05;
    switch ((s32) temp_r5) {
    case 0:
        self->state_0x05 = (u8) (temp_r5 + 1);
        fn_80130478(self, 0);
        if ((u8) arg1 == 0) {
            fn_8012F5B8(self, 0x14, 4, 0);
            fn_80129668(self, 0, 5);
            fn_80129668(self, 1, 6);
        } else {
            fn_8012F5B8(self, 0x1A, 4, 0);
            fn_80129668(self, 0, 7);
            fn_80129668(self, 1, 8);
        }
        return;
    case 1:
        if ((u8) arg1 == 0) {
            fn_80133E3C(self, 0x4000, lbl_80797330, lbl_80797340);
        } else {
            fn_80133E3C(self, -0x4000, lbl_80797330, lbl_80797340);
        }
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}


extern "C" void fn_8015FCB0(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015F2D8(self);
        return;
    case 1:
        fn_8015F39C(self);
        return;
    case 2:
        fn_8015F450(self);
        return;
    case 3:
        fn_8015F510(self, 0);
        return;
    case 4:
        fn_8015F60C(self);
        return;
    case 5:
        fn_8015F6C4(self, 0);
        return;
    case 6:
        fn_8015F510(self, 1);
        return;
    case 7:
        fn_8015F820(self);
        return;
    case 8:
        fn_8015F8E4(self);
        return;
    case 9:
        fn_8015F6C4(self, 1);
        return;
    case 10:
        fn_8015F9FC(self);
        return;
    case 11:
        fn_8015FAC0(self);
        return;
    case 12:
        fn_8015FB78(self);
        return;
    case 13:
        return;
    }
}

extern "C" void fn_8015F23C(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015E854(self, 0, 0);
        return;
    case 1:
        fn_8015E854(self, 1, 1);
        return;
    case 2:
        fn_8015E854(self, 2, 1);
        return;
    case 3:
        fn_8015E854(self, 3, 1);
        return;
    case 4:
        fn_8015E854(self, 4, 1);
        return;
    case 5:
        fn_8015E854(self, 5, 1);
        return;
    case 6:
        fn_8015EA24(self);
        return;
    case 7:
        fn_8015EAC8(self);
        return;
    case 8:
        fn_8015EB44(self);
        return;
    case 9:
        fn_8015ED00(self);
        return;
    case 10:
        fn_8015ED94(self);
        return;
    case 11:
        fn_8015EED4(self);
        return;
    case 12:
        fn_8015E854(self, 6, 1);
        return;
    case 13:
        fn_8015EFAC(self);
        return;
    case 14:
        fn_8015F0E8(self);
        return;
    }
}

extern "C" void fn_801620A4(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015FD84(self, 0);
        return;
    case 1:
        fn_80161660(self);
        return;
    case 2:
        fn_80161784(self, 0);
        return;
    case 3:
        fn_8015FD84(self, 1);
        return;
    case 4:
        fn_80161DCC(self, 1);
        return;
    case 5:
        fn_801618A4(self);
        return;
    case 6:
        fn_8016198C(self, 0);
        return;
    case 7:
        fn_80161A34(self, 0);
        return;
    case 8:
        fn_80161D24(self);
        return;
    case 9:
        fn_80161DCC(self, 0);
        return;
    case 10:
        fn_80161784(self, 1);
        return;
    case 11:
        fn_80161E94(self, 0);
        return;
    case 12:
        fn_8016198C(self, 1);
        return;
    case 13:
        fn_80161A34(self, 1);
        return;
    case 14:
        fn_8015FD84(self, 2);
        return;
    case 15:
        fn_8015FD84(self, 3);
        return;
    case 16:
        fn_80161A34(self, 2);
        return;
    case 17:
        fn_80161E94(self, 1);
        return;
    case 18:
        fn_80162028(self);
        return;
    }
}
