/* enemy/fn_801993E0.cpp - the enemy "em" action band between `enemy/fn_80191598.cpp` and the
 * unclaimed run at 0x801A4504.
 * .text 0x801993E0..0x8019ED34 (0x5954), 35 functions; extab 0x8000F084..0x8000F14C (25 records);
 * extabindex 0x8002A7B0..0x8002A8DC (25 records).
 *
 * Registration (proposal/801993E0_fn_801993E0.cpp).  The range is registered once, here, at its
 * final home.  Which class decided the name and module:
 *   * class 1 (a `__FILE__` string) fails.  The split's objects for this range (the per-symbol
 *     `auto_fn_8019xxxx_text.o` / `auto_03_8019xxxx_text.o`) have no undefined reference to any
 *     `__FILE__` literal: their whole undefined set is the `.sdata2` float pool, the `.data`
 *     jumptables and `.bss` globals, and `nm -u` over all thirty-two of them names no string
 *     symbol at all.  `python tools/symbols/dumpmap.py lookup 0x801993E0` answers the
 *     `zz_01993e0_` placeholder, which the brief states is not evidence (class 2 fails too).
 *   * class 3 decides the module: `enemy`.  The bracketing registered units are `enemy` (below:
 *     `enemy/fn_80191598.cpp` ends at 0x801926EC; the next occupied band above is the unclaimed
 *     0x801A4504 run), and every callee the range names out of its own band is an enemy-band
 *     function - `em_frame_check__FP11_ENEMY_WORKUsff`, `em_act_ck__FP11_ENEMY_WORKUcUc`,
 *     `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`, `get_em_chg_scale__FP11_ENEMY_WORK`,
 *     `get_move_work_adrs__FUc` - and every function it defines takes the `_ENEMY_WORK` record.
 *   * class 4 keeps the name: nothing supports a file name, so the map's own `fn_801993E0` stem is
 *     the file name.  The brief's `enemy_control.cpp` warning is NOT evidence that this range is
 *     that file: `attribute.py`'s `source_owner` returns "the latest accepted `__FILE__` name
 *     starting at or before", and `enemy_control.cpp` is the last accepted name before this run
 *     (`enemy/enemy_control.cpp` is registered at 0x801411B8..0x80147CE0, its `__FILE__` string at
 *     0x805A1BB8).  Nothing in this range references that string, and a unit cannot own two
 *     disjoint `.text` runs, so the range is its own TU.
 *
 * C++ (`-lang=c++` through the lib's `cflags_main`) because the range reaches mangled callees -
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff` - through their real
 * signatures (rule 9).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked over
 * config/RMHE08/symbols.txt - every symbol this unit defines is a bare `.text` entry with no owner
 * name - and `python tools/symbols/dumpmap.py lookup 0x801993E0` answers the `zz_` placeholder
 * form, which is not evidence).
 *
 * Sections.  `.text 0x801993E0..0x8019ED34`, `extab 0x8000F084..0x8000F14C` (25 records, 8 B
 * each), `extabindex 0x8002A7B0..0x8002A8DC` (25 x 12 B) and `.ctors 0x8056F340..0x8056F344`.
 * The `.ctors` word is this unit's: it is the 4-byte static-initializer entry that
 * `build/RMHE08/obj/auto_fn_8019E604_text.o` carries (the object dtk's split built for
 * `fn_8019E604`), and its value is 0x8019E604 - this range's own function.
 *
 * Status (official `build/RMHE08/report.json`, full `ninja` in this worktree, `main.dol: OK`).
 * 25 of the 35 functions are written and every written one is above the 80 % bar; 10 are
 * byte-identical.  Unit: 15.003324 % fuzzy, 944 / 22868 `.text` bytes matched, 10 / 35 functions
 * matched.  The ten unwritten functions (the follow-up queue, in address order) are
 * `fn_801994F4` (1336 B), `fn_80199BE0` (980 B), `fn_80199FB4` (544 B), `fn_8019A1D4` (12336 B),
 * `fn_8019D204` (1716 B), `fn_8019DBDC` (692 B), `fn_8019DE98` (556 B), `fn_8019E0C4` (724 B),
 * `fn_8019E604` (108 B) and `fn_8019EC38` (156 B).
 *
 * Residuals, by measurement (all the near-misses are codegen shapes, not comprehension):
 *   * `fn_802B0668` RETURN TRUNCATION - `fn_8019D8B8` 90.52, `fn_8019D9BC` 82.62, `fn_8019DAC0`
 *     96.00, `fn_8019E70C` 84.05.  The target keeps `clrlwi r0,r3,24` after every call to the
 *     map-id lookup (it is a byte table) and compares the byte; MWCC folds the truncation away
 *     when the compared value is a small constant.  Written with the `(u8)` cast at the call
 *     (`fn_8019E70C`) and without, and the instruction is dropped both ways: the owner's
 *     declaration is `u32 fn_802B0668(u32)` (this unit's own, since no registered unit owns
 *     0x802B0668 and `stylelint`'s rule-2 band answers `None` for it), so the truncation has to
 *     come from a `u8`-returning view of the callee, which would re-measure every landed
 *     consumer (`enemy/fn_80182D5C.cpp` declares the same `u32` form).
 *   * THE 0x354 BIT MAP'S BASE - `fn_8019E960` 94.63, `fn_8019E9AC` 88.82.  The target reaches
 *     the four mask bytes as `self + 0x328` plus a `0x2C` displacement; ours folds the whole
 *     `0x354` into the `addi`.  `mask_0x354` and the s16/byte view
 *     (`field_0x354`/`field_0x356`/`field_0x357`) are two members of one union in
 *     `include/enemy/ENEMY_WORK.h` so both spellings keep their offsets and names; MWCC only
 *     emits the split base when the source reaches the field through a *named* nested member,
 *     which would rename four fields four landed units read.  Recorded rather than restructured.
 *   * MWCC UNROLL FACTOR - `fn_8019EA80` 87.85 (300 B target / 280 B ours).  The 8-iteration
 *     free-slot scan is emitted as four steps under `mtctr 2` in the target (and keeps the
 *     indexed `stbx`), and fully unrolled with a strength-reduced store pointer here.  Three
 *     spellings measured (`u32`/`u8` count, index vs pointer store); the unroller's choice did
 *     not move.
 *   * SHARED `return 0` BLOCK - `fn_8019E840` 89.50 (200 B target / 180 B ours).  The target
 *     inlines the first two guards' `li r3,0; b <epilogue>` and merges the rest into one tail;
 *     only the nested-`if` spelling both reproduces that layout and *pairs* in objdiff - the flat
 *     `return 0` chain and the `||` chain each made `report generate` answer a null
 *     `fuzzy_match_percent` for this symbol (`recompile.py`: "no pairing"), so the nested form is
 *     the one recorded.
 *   * SINGLE-INSTRUCTION ROWS - `fn_8019E670` 97.31 (the `switch` discriminant: the target
 *     separates `clrlwi` from `cmpwi`, MWCC fuses them), `fn_8019EBAC` 96.43 (same size, one
 *     `cror` row), `fn_8019E5A8` 95.43 (the `(s16)flags > 0` `extsh`), `fn_8019E908` 86.56 (the
 *     target materialises the +0x1E4 byte once), `fn_8019DAC0` 96.00.
 *   * `fn_8019E398` 89.85, `fn_8019E49C` 92.11, `fn_8019EA04` 81.03 - register allocation on the
 *     two `||` blocks, the f30 save/restore of the radius comparison, and the hoisted
 *     `clrlwi r0,r27,16` of the slot loop.  Recorded as measured; nothing to change in the source.
 *
 * Dependencies on headers this registration had to touch (rule 2):
 *   * `include/enemy/fn_80191598.h` is NEW - `fn_80192370`/`fn_80192618` are owned by
 *     `enemy/fn_80191598.cpp` and had no owner header.
 *   * `include/enemy/fn_801251D0.h`'s `fn_80126278` was two-argument; its owner's call sites set
 *     r3 (the `_ENEMY_WORK`), r4 (the id) and r5 (the `VEC3*` fill target), so it is now
 *     three-argument.
 *   * `include/enemy/fn_8012EC74.h` gained the C++ spelling of `em_water_check`/`get_em_chg_scale`
 *     (rule 9: never call the mangling); `include/enemy/fn_8012BDF4.h` gained `em_act_ck`'s C++
 *     spelling for the same reason; `include/unsplit/enemy.h` gained the thirteen unowned
 *     0x80192F24.. entries the two dispatchers switch over (the 0x801926EC..0x801993E0 run has no
 *     registered owner yet, and both brackets are `enemy`).
 */

#include "types.h"
#include "nw4r/math.h"

#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h" /* fn_8012CF20, em_act_ck */
#include "enemy/fn_8012EC74.h" /* em_water_check, get_em_chg_scale */
#include "enemy/fn_80138074.h" /* fn_8013A654, fn_8013918C */
#include "enemy/fn_80191598.h" /* fn_80192370, fn_80192618 */
#include "fn_8004CAD8.h"       /* calcDistanceSqXZ */
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/enemy.h"

extern "C" {

/* ------------------------------------------------------------------------------------------------
 * The `.sdata2` float pool this range loads.  Its labels are unsplit (no registered unit claims a
 * `.sdata2` range beyond `main.cpp`'s 0x80795AA0..0x80795AD8) and `stylelint`'s rule-2 `module()`
 * answers `None` for the section (the bracketing band is one-sided), so these stay declared here as
 * the counted "address band interleaves modules" gap - the same shape the landed
 * `enemy/fn_80182D5C.cpp` uses for its own pool runs.  Never defined: the target addresses the
 * pool, and defining one would rebuild it.
 * ------------------------------------------------------------------------------------------------ */

extern f32 lbl_80798238;
extern f32 lbl_807983B0;
extern f32 lbl_807983F0;
extern f32 lbl_807983F4;
extern f32 lbl_807983F8;
extern f32 lbl_80798288;
extern f32 lbl_80798510;
extern f32 lbl_80798518;
extern f32 lbl_8079851C;
extern f32 lbl_80798520;
extern f32 lbl_80798524;
extern f32 lbl_80798528;
extern f32 lbl_8079852C;

/* ------------------------------------------------------------------------------------------------
 * Symbols with no registered owner whose address band names no module (the lint's counted
 * "address band interleaves modules" gap): the RSO-side helpers this band calls.  Declared here for
 * the same reason `enemy/fn_80182D5C.cpp` declares `fn_802B0668`.
 * ------------------------------------------------------------------------------------------------ */

/* 0x802B0668 - the map-id lookup: a byte table, `0xFF` meaning "no entry" (the argument comes back). */
u32 fn_802B0668(u32 kind);
/* 0x803B50A8 - r3 (a selector, 0 at every call site here) and no other argument; returns a status
 * compared against 1. */
u32 fn_803B50A8(u32 a);
/* 0x803B9994 - releases one handle of the enemy-control slot set (r3 = the handle). */
void fn_803B9994(s32 handle);
/* 0x803A8EE4 - the RSO-side random source; `fn_8019EA80` masks its low 16 bits. */
u32 fn_803A8EE4(void);

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions: declared up front so the dispatchers can call them.
 * ------------------------------------------------------------------------------------------------ */

void fn_801993E0(struct _ENEMY_WORK* self);
void fn_80199468(struct _ENEMY_WORK* self);
void fn_801994F4(struct _ENEMY_WORK* self);
void fn_80199A2C(struct _ENEMY_WORK* self);
void fn_80199ADC(struct _ENEMY_WORK* self);
void fn_8019E398(struct _ENEMY_WORK* self);
u32 fn_8019E670(struct _ENEMY_WORK* self, u32 mode, u32 lo, u32 hi);
void fn_8019E948(struct _ENEMY_WORK* self);

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions.
 * ------------------------------------------------------------------------------------------------ */

/* 0x801993E0 - the state-advance entry of the enemy's "em" action: state 0 arms the motion, state 1
 * waits for `fn_8012F93C` to report the current action finished and then runs the band's
 * `fn_80128A14(self, 13, 5)` completion. */
void fn_801993E0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F504(self, 49, 20, 0, 1);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128A14(self, 13, 5);
        }
        break;
    }
}

/* 0x80199468 - the same shape with this action's own arming pair (mode 46, duration 6, the 1000 ms
 * `fn_80130CDC` timer) and `fn_80128030` as the completion. */
void fn_80199468(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F504(self, 46, 6, 0, 1);
        fn_80130CDC(self, 1000);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80199A2C - the motion step of the action `fn_80199ADC` dispatches case 7 to: state 0 arms mode
 * 31 with the band's `fn_80146058`/`fn_8014610C` pair and zeroes the stored height, state 1 waits for
 * `fn_8012F93C` and then runs `fn_80128030`. */
void fn_80199A2C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 31, 0, 0);
        fn_80146058(self, lbl_807983F0, lbl_807983F4, lbl_807983F8);
        fn_8014610C(self, lbl_80798238, lbl_807983B0, lbl_80798238);
        fn_801303EC(self, lbl_80798238);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80199ADC - the action-id dispatcher: `self->state_sub` (0x1E6) selects this range's own
 * per-action update, whose first two entries the registered `enemy/fn_80191598.cpp` owns. */
void fn_80199ADC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80198F28(self);
        break;
    case 1:
        fn_80198FE8(self);
        break;
    case 2:
        fn_801990E0(self);
        break;
    case 3:
        fn_801991E4(self);
        break;
    case 4:
        fn_801993E0(self);
        break;
    case 5:
        fn_80199468(self);
        break;
    case 6:
        fn_801994F4(self);
        break;
    case 7:
        fn_80199A2C(self);
        break;
    }
}

/* 0x80199B24 - the per-action `action` (0x1E5) dispatcher of the run above, plus the common tail
 * every action shares: the +0x1E2 gate that runs the pair `fn_8012CF20`/`fn_80131E74`, then this
 * unit's own `fn_8019E398`. */
void fn_80199B24(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_80192F24(self);
        break;
    case 1:
        fn_80193394(self);
        break;
    case 2:
        fn_801938D8(self);
        break;
    case 5:
        fn_801953BC(self);
        break;
    case 6:
        fn_80196618(self);
        break;
    case 7:
        fn_801987E4(self);
        break;
    case 10:
        fn_80198910(self);
        break;
    case 11:
        fn_80198E00(self);
        break;
    case 12:
        fn_80198F14(self);
        break;
    case 13:
        fn_80199ADC(self);
        break;
    }
    if (self->field_0x1E2 == 1) {
        fn_8012CF20(self);
        fn_80131E74(self);
    }
    fn_8019E398(self);
}

/* 0x8019DB9C - the "action 2 still running" gate: true only in mode 4 of the enemy-control state
 * machine while `fn_8012EC60` reports not-yet-armed. */
u32 fn_8019DB9C(struct _ENEMY_WORK* self) {
    if (self->field_0x1E2 == 4 && fn_8012EC60(self) == 0) {
        return 1;
    }
    return 0;
}

/* 0x8019DE90 - the one-line tail call the band's teardown uses: `fn_8013A654(self, 1)`. */
void fn_8019DE90(struct _ENEMY_WORK* self) {
    fn_8013A654(self, 1);
}

/* 0x8019E580 - "action 13 in its first five sub-states". */
u32 fn_8019E580(struct _ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* 0x8019E5A8 - the deleting destructor of that record: reset through the owner's `fn_8013918C`,
 * then `operator delete` when the caller passed a positive flag.  Returns its argument, exactly as
 * the target's `mr r3,r30` epilogue does. */
void* fn_8019E5A8(void* p, s16 flags) {
    if (p != 0) {
        fn_8013918C((struct _ENEMY_WORK*)p, 0);
        if (flags > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* 0x8019E670 - the band's range test on the record's byte at +0x961: mode 0 `==`, 1 `>=`, 2 `<=`,
 * 3 the inclusive window. */
u32 fn_8019E670(struct _ENEMY_WORK* self, u32 mode, u32 lo, u32 hi) {
    switch ((u8)mode) {
    case 0:
        if (self->stack_0x961[0] == (u8)lo) {
            return 1;
        }
        break;
    case 1:
        if (self->stack_0x961[0] >= (u8)lo) {
            return 1;
        }
        break;
    case 2:
        if (self->stack_0x961[0] <= (u8)lo) {
            return 1;
        }
        break;
    case 3:
        if (self->stack_0x961[0] >= (u8)lo && self->stack_0x961[0] <= (u8)hi) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x8019E908 - arm/step the +0x1E4 countdown: mode 0 steps it while bit 0 is set, mode 1 sets bit 0. */
void fn_8019E908(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 0) {
        if ((self->field_0x1E4 & 1) == 0) {
            return;
        }
        self->field_0x1E4--;
    } else if (mode == 1) {
        self->field_0x1E4 |= 1;
    }
}

/* 0x8019E948 - clear the four-byte action bit set at +0x354 (the target's four `stb`s are the
 * compiler's inlined `memset`). */
void fn_8019E948(struct _ENEMY_WORK* self) {
    self->mask_0x354[0] = 0;
    self->mask_0x354[1] = 0;
    self->mask_0x354[2] = 0;
    self->mask_0x354[3] = 0;
}

/* 0x8019E960 - set bit `id` of that set: slot `id / 8` clamped to the last of the four bytes. */
void fn_8019E960(struct _ENEMY_WORK* self, s32 id) {
    s32 slot = id / 8;
    if (slot >= 4) {
        slot = 3;
    }
    self->mask_0x354[slot] |= 1 << (id % 8);
}

/* 0x8019E9AC - the matching test. */
s32 fn_8019E9AC(struct _ENEMY_WORK* self, s32 id) {
    s32 slot = id / 8;
    if (slot >= 4) {
        slot = 3;
    }
    if (self->mask_0x354[slot] & (1 << (id % 8))) {
        return 1;
    }
    return 0;
}

/* 0x8019D8B8 - the per-mode "this action may run" predicate the band's dispatchers gate on. */
u32 fn_8019D8B8(struct _ENEMY_WORK* self, u8 mode) {
    switch (mode) {
    case 0:
        if (em_water_check(self) == 1) {
            return 1;
        }
        break;
    case 1:
        if ((self->field_0x35C & 1) == 0) {
            return 1;
        }
        break;
    case 2:
        if ((self->field_0x35C & 2) == 0) {
            return 1;
        }
        break;
    case 3:
        if ((self->field_0x228 & 3) != 0) {
            return 1;
        }
        break;
    case 4:
        if (self->vec_0x36C.z - self->pos.y >= lbl_80798288) {
            return 1;
        }
        break;
    case 5:
        if (fn_80192370(self, 16) == 0) {
            return 1;
        }
        break;
    case 6:
        if (self->field_0x360 <= 0) {
            return 1;
        }
        break;
    case 7:
        if (self->field_0x362 <= 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x8019DAC0 - the area dispatch of the "already in mode" state: the map lookup picks the area
 * group, this area decides whether the state is armed. */
void fn_8019DAC0(struct _ENEMY_WORK* self) {
    u32 state = 0;
    if (fn_802B0668(self->field_0x1E0) == 3) {
        switch (self->area_no) {
        case 1:
            state = 1;
            break;
        case 2:
            state = 2;
            break;
        case 3:
            if (self->field_0x9F6 == 2) {
                state = 1;
            }
            break;
        }
    }
    if (state == 1) {
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
    } else if (state == 2) {
        fn_80130478(self, 2);
        fn_8012F5B8(self, 40, 0, 0);
    }
}

/* 0x8019D9BC - the "load the action's joint position" init: arm mode 4, publish the two record
 * bytes and ask `fn_80126278` for the area's joint, then run the band's common tail. */
void fn_8019D9BC(struct _ENEMY_WORK* self, u8* out_a, u8* out_b) {
    fn_80130478(self, 4);
    *out_a = 12;
    *out_b = 0;
    switch (fn_802B0668(self->field_0x1E0)) {
    case 3:
        switch (self->area_no) {
        case 3:
            fn_80126278(self, (u16)(((self->area_no & 0xF) << 8) | 6), &self->pos);
            break;
        case 8:
            fn_80126278(self, (u16)(((self->area_no & 0xF) << 8) | 9), &self->pos);
            break;
        }
        break;
    case 9:
    case 11:
        if (self->area_no == 1) {
            fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        }
        break;
    }
    fn_80192618(self);
}

/* 0x8019E398 - the two +0x35C completion flags: each is set while its action pair matches and
 * cleared once the enemy has moved on to another action. */
void fn_8019E398(struct _ENEMY_WORK* self) {
    if (em_act_ck(self, 6, 0) == 1 || em_act_ck(self, 6, 1) == 1) {
        if ((self->field_0x35C & 1) == 0) {
            self->field_0x35C |= 1;
        }
    } else if (self->action != 0 && (self->field_0x35C & 1) != 0) {
        self->field_0x35C &= ~1;
    }
    if (em_act_ck(self, 5, 17) == 1 || em_act_ck(self, 5, 18) == 1) {
        if ((self->field_0x35C & 2) == 0) {
            self->field_0x35C |= 2;
        }
    } else if (self->action != 0 && (self->field_0x35C & 2) != 0) {
        self->field_0x35C &= ~2;
    }
}

/* 0x8019E70C - the "which sub-phase of the shore action" query: 255 when the record is not the one
 * this table describes, else the phase the byte at +0x961 falls into. */
u32 fn_8019E70C(struct _ENEMY_WORK* self) {
    if (self == 0) {
        return 255;
    }
    if (self->team != 25) {
        return 255;
    }
    if (fn_802B0668(self->field_0x1E0) != 6) {
        return 255;
    }
    if (self->area_no != 1) {
        return 255;
    }
    if (fn_803B50A8(0) == 1) {
        return 255;
    }
    if (fn_8019E670(self, 3, 0, 3)) {
        return 0;
    }
    if (fn_8019E670(self, 3, 4, 19)) {
        return 1;
    }
    if (fn_8019E670(self, 3, 20, 59)) {
        return 3;
    }
    if (fn_8019E670(self, 3, 60, 70)) {
        return 4;
    }
    if (fn_8019E670(self, 3, 71, 91)) {
        return 2;
    }
    return 255;
}

/* 0x8019E840 - the matching yes/no form of the same table. */
u32 fn_8019E840(struct _ENEMY_WORK* self) {
    if (self != 0) {
        if (self->team == 25) {
            if (fn_802B0668(self->field_0x1E0) == 6) {
                if (self->area_no == 1) {
                    if (fn_803B50A8(0) != 1) {
                        if (fn_8019E670(self, 3, 30, 59) == 1) {
                            return 1;
                        }
                        if (fn_8019E670(self, 3, 90, 92) == 1) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* 0x8019EBAC - step the rotation word at +0x1C0 by 4 (clamped to the 0x4000..0x4004 band) and keep
 * the height at +0x190 inside its two pool bounds, snapping to the middle value otherwise. */
void fn_8019EBAC(struct _ENEMY_WORK* self) {
    u32 angle = self->field_0x1C0 & 0xFFFF;
    self->field_0x1C0 = angle;
    if (angle >= 16388) {
        self->field_0x1C0 = angle - 4;
    } else if (angle > 16380) {
        self->field_0x1C0 = 16384;
    } else {
        self->field_0x1C0 = angle + 4;
    }
    if (self->pos.z >= lbl_80798518) {
        self->pos.z -= lbl_8079851C;
        return;
    }
    if (self->pos.z <= lbl_80798520) {
        self->pos.z += lbl_8079851C;
        return;
    }
    self->pos.z = lbl_80798524;
}

/* 0x8019ECD4 - arm the four enemy-control slots at +0x328 and seed the record's remaining state,
 * then clear the bit set. */
void fn_8019ECD4(struct _ENEMY_WORK* self) {
    self->handles_0x328[0] = -1;
    self->states_0x338[0] = 255;
    self->handles_0x328[1] = -1;
    self->states_0x338[1] = 255;
    self->handles_0x328[2] = -1;
    self->states_0x338[2] = 255;
    self->handles_0x328[3] = -1;
    self->states_0x338[3] = 255;
    self->field_0x33C = 0;
    self->field_0x340 = 0;
    self->field_0x344 = 0;
    self->field_0x348 = lbl_80798528;
    self->field_0x352 = 1;
    self->field_0x34C = lbl_8079852C;
    self->field_0x350 = 0;
    self->field_0x353 = 0;
    self->field_0x358 = 0;
    fn_8019E948(self);
}

/* 0x8019EA04 - release every live enemy-control handle and re-arm the slot states. */
void fn_8019EA04(struct _ENEMY_WORK* self) {
    for (u16 i = 0; i < 4; i++) {
        if (self->handles_0x328[i] != -1) {
            fn_803B9994(self->handles_0x328[i]);
            self->handles_0x328[i] = -1;
        }
        self->states_0x338[i] = 255;
    }
    self->field_0x761 = 0;
}

/* 0x8019EA80 - pick one of the free slot indices at random: the low 8 bits of +0x761 mark the used
 * slots, the RSO random source is folded down to the free count. */
u32 fn_8019EA80(struct _ENEMY_WORK* self, u16 idx) {
    u8 avail[8];
    u8 n = 0;
    u8 mask = self->field_0x761;
    u8 i;
    for (i = 0; i < 8; i++) {
        if ((mask & (1 << i)) == 0) {
            avail[n] = i;
            n++;
        }
    }
    if (n <= 4) {
        return 255;
    }
    s32 value = ((fn_803A8EE4() & 0xFFFF) + 0x157E7) >> (idx & 0xFFFF);
    value = (value * 13) & 0xFFFF;
    return avail[value % (s32)n];
}

/* 0x8019E49C - "is the record we are hunting within reach": true when the paired record exists and
 * the xz distance is inside the scaled radius (or when the caller asked for the trivial answer). */
u32 fn_8019E49C(struct _ENEMY_WORK* self, u32 flag) {
    struct _ENEMY_WORK* other = fn_80131034(self, 23, 0);
    if (other != 0 && fn_8012E5A8(other) == 1) {
        if ((u8)flag == 0) {
            return 1;
        }
        f32 dist = calcDistanceSqXZ(&self->pos, &other->pos);
        f32 range = get_em_chg_scale(self) * lbl_80798510;
        if (dist < range * (get_em_chg_scale(self) * lbl_80798510)) {
            return 1;
        }
    }
    return 0;
}

} /* extern "C" */
