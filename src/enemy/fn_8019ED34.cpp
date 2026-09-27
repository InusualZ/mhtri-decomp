/* enemy/fn_8019ED34.cpp - the enemy motion/action band between `enemy/fn_801993E0.cpp` and
 * `enemy/fn_801A4504.cpp`.
 * .text 0x8019ED34..0x801A4504 (0x57D0), 65 functions; extab 0x8000F14C..0x8000F304 (55 records);
 * extabindex 0x8002A8DC..0x8002AB70 (55 records).
 *
 * Registration (proposal/8019ED34_fn_8019ED34.cpp).  The range is registered once, here, at its
 * final home.  Which class decided the name and module:
 *   * class 1 (a `__FILE__` string) fails.  No object in the range has an undefined reference to a
 *     `__FILE__` literal: its undefined set is the `.sdata2` float pool, the `.data` jumptables and
 *     `.bss` globals only.  `python tools/symbols/dumpmap.py lookup 0x8019ED34` answers
 *     `FUN_8019ed34`/`zz_019ed34_`, which the brief states is not evidence (class 2 fails too).
 *   * class 3 decides the module: `enemy`.  The bracketing registered units are `enemy` (below:
 *     `enemy/fn_801993E0.cpp` ends at 0x8019ED34; above: `enemy/fn_801A4504.cpp` starts at
 *     0x801A4504), and every callee out of the range is an enemy-band function
 *     (`em_frame_check__FP11_ENEMY_WORKUsff`, `fn_801251D0`, `em_mot_end_ck`, `em_move_mode_set`,
 *     `em_get_mot_no__FP11_ENEMY_WORK`, `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`,
 *     `em_act_ck__FP11_ENEMY_WORKUcUc`) and every state machine switches on `_ENEMY_WORK::state`.
 *   * class 4 keeps the name: nothing supports a file name, so the map's own `fn_8019ED34` stem is
 *     the file name (the sibling units use the same scheme).
 * Seam: `tudiscover at 0x8019ED34` reports a STRONG left boundary there (`.sdata2` pool-run jump
 * lbl_80798524 -> lbl_80798530), so his range starts its own TU.  The right edge 0x801A4504 is the
 * discovery cap (a weak closure edge), not a proven boundary; the range is worked as one unit and
 * the extent settles as its functions match (invariant 8.3).
 *
 * C++ (`-lang=c++` through the lib's `cflags_main`) because the range reaches mangled callees -
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff` - through their real
 * signatures (rule 9).  Every plain `fn_XXXXXXXX` definition here is `extern "C"` so its map name is
 * emitted.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup` - every address answers the `zz_XXXXXXXX_`/`FUN_XXXXXXXX`
 * placeholder, and no `__FILE__` string is referenced anywhere in the range).
 *
 * Status.  34 of the 65 functions are written and every written one is above the 90 % bar
 * (`python tools/units/recompile.py enemy/fn_8019ED34.cpp --measure <symbol>`); 23 are
 * byte-identical.  Full `ninja` is not runnable in this worktree (no `orig/RMHE08/sys/main.dol`),
 * so the scores are `recompile.py`'s `report generate` metric against MAIN's retired split objects -
 * the same number the registered unit reports.  The 31 unwritten functions (the follow-up queue, in
 * address order, size in bytes) are `fn_8019ED34` (464), `fn_8019EF04` (60), `fn_8019EF44` (312),
 * `fn_8019F378` (500), `fn_801A0210` (932), `fn_801A0684` (356), `fn_801A07E8` (224),
 * `fn_801A08C8` (192), `fn_801A0988` (328), `fn_801A0AD0` (376), `fn_801A0C48` (932),
 * `fn_801A10AC` (260), `fn_801A11B0` (280), `fn_801A1894` (208), `fn_801A1964` (232),
 * `fn_801A1A4C` (364), `fn_801A1BB8` (680), `fn_801A1E60` (244), `fn_801A1F54` (192),
 * `fn_801A2014` (828), `fn_801A2350` (284), `fn_801A246C` (584), `fn_801A26B4` (532),
 * `fn_801A28C8` (1252), `fn_801A2E30` (676), `fn_801A30E8` (1252), `fn_801A35CC` (1672),
 * `fn_801A3C54` (360), `fn_801A3E90` (328), `fn_801A3FD8` (576), `fn_801A437C` (392).
 *
 * Residuals, by measurement (all the near-misses are codegen shapes, not comprehension):
 *   * TIMER DECREMENT (`fn_8019F56C` 95.29, `fn_801A1454` 96.19, `fn_801A16D4` 94.30,
 *     `fn_801A1384` 94.90) - the target stores the decremented `timer_0x020` and then tests it with
 *     a separate `cmpwi`; MWCC folds `--x; if (x <= 0)` into the record-form `addic.` and drops the
 *     `cmpwi` (4 bytes shorter).  Three spellings were measured (`--x`, a named local, and
 *     `x = x - 1` separately); all emit `addic.`, and the exact `addi`+`cmpwi` shape needs a
 *     source-level idiom MWCC did not accept here.  Recorded rather than restructured.
 *   * REGISTER CHOICE (`fn_8019F07C` 99.09) - the `lbl_805AF9CC`/`lbl_805AFC08` stores materialise
 *     the address into r4 and store r4; the target uses r0 (`addi r0,r4,off; stw r0,...`).  One
 *     instruction over the whole function; the pool/register allocation is otherwise identical.
 *   * SINGLE WORD (`fn_801A05E0` 95.12, `fn_801A4218` 97.82, `fn_801A3DEC` 100, `fn_8019F768`/
 *     `fn_8019F9BC` 98.66, `fn_8019FC10`/`fn_8019FE80` 98.72) - one register or a `cror` pair
 *     differs in the ten-window `em_frame_check` pass and the status-bit branch; the control flow
 *     and every call are the same.  Nothing to change without moving the landed owners' headers.
 *   * `fn_801A2E30` (the 22-case motion table) is deliberately NOT written yet: its call sites pass
 *     a fourth argument (r6) to `fn_801251D0`, whose owner header declares three; matching it needs
 *     a 4-argument spelling of an owned symbol (rule 2/9), which is a residual for the owner's
 *     header, not this unit's source.
 *
 * Sections claimed: `.text` 0x8019ED34..0x801A4504, extab 0x8000F14C..0x8000F304, extabindex
 * 0x8002A8DC..0x8002AB70.  No `.ctors` word belongs to the range: only `auto_fn_8019E604_text.o`
 * in the enemy band carries one, and it belongs to `enemy/fn_801993E0.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"

#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck */
#include "enemy/fn_8012EC74.h" /* em_get_mot_no, em_after_frame_check, get_joint_wmat_em */
#include "enemy/fn_801993E0.h"
#include "fn_8004CAD8.h" /* MTX34_ctor, mulVecMatAddTrans */
#include "unsplit/enemy.h"

extern "C" {

/* ------------------------------------------------------------------------------------------------
 * The `.sdata2` float pool this range loads.  Its labels are unsplit (no registered unit claims a
 * `.sdata2` range) and `stylelint`'s rule-2 `module()` answers `None` for the section, so these stay
 * declared here as the counted "address band interleaves modules" gap - the same shape
 * `enemy/fn_801993E0.cpp` uses for its own pool run.  Never defined: the target addresses the pool.
 * ------------------------------------------------------------------------------------------------ */

extern f32 lbl_80798528;
extern f32 lbl_8079852C;
extern f32 lbl_80798538;
extern f32 lbl_80798540;
extern f32 lbl_80798544;
extern f32 lbl_80798548;
extern f32 lbl_8079854C;
extern f32 lbl_80798550;
extern f32 lbl_80798554;
extern f32 lbl_80798558;
extern f32 lbl_8079855C;
extern f32 lbl_80798560;
extern f32 lbl_80798564;
extern f32 lbl_80798568;
extern f32 lbl_8079856C;
extern f32 lbl_80798570;
extern f32 lbl_80798574;
extern f32 lbl_80798578;
extern f32 lbl_8079857C;
extern f32 lbl_80798580;
extern f32 lbl_80798584;
extern f32 lbl_80798588;
extern f32 lbl_8079858C;
extern f32 lbl_80798590;
extern f32 lbl_80798594;
extern f32 lbl_80798598;
extern f32 lbl_8079859C;
extern f32 lbl_807985A0;
extern f32 lbl_807985A4;
extern f32 lbl_807985A8;
extern f32 lbl_807985D4;
extern f32 lbl_807985D8;
extern f32 lbl_807985DC;
extern f32 lbl_807985E0;
extern f32 lbl_807985E4;
extern f32 lbl_807985E8;
extern f32 lbl_807985EC;
extern f32 lbl_807985F0;
extern f32 lbl_807987B4;

/* ------------------------------------------------------------------------------------------------
 * `.data` tables with no registered owner (the same counted rule-2 gap).  `fn_8019F07C` stores two
 * of them into the record's dispatch words; the rest are the tail-call dispatcher's motion tables.
 * ------------------------------------------------------------------------------------------------ */

extern u8 lbl_805AE7C0[];
extern u8 lbl_805AF268[];
extern u8 lbl_805AF9CC[];
extern u8 lbl_805AFC08[];
extern u8 lbl_805B0478[];
extern u8 lbl_80570150[];

/* The status-bit helpers `enemy/fn_8011D448.cpp` defines (`bits_0x824`) - declared locally, the same
 * counted gap its other consumers (`enemy/fn_80137604.cpp`) use: the owner registered no header. */
void fn_8011E5EC(struct _ENEMY_WORK* self);
void fn_8011E620(struct _ENEMY_WORK* self, u32 mask);
void fn_8011E630(struct _ENEMY_WORK* self, u32 mask);

/* ------------------------------------------------------------------------------------------------
 * Symbols with no registered owner whose address band names no module (the lint's counted
 * "address band interleaves modules" gap): the model/effect helpers this band calls.
 * ------------------------------------------------------------------------------------------------ */

/* 0x802B08DC - r3 is the `_ENEMY_WORK` (set by the caller), f1 the alpha/blend scalar; no return. */
void fn_802B08DC(f32 a);

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions: declared up front so the dispatchers can call them.  A declaration for
 * a function this unit does not define yet is legal while the bodies are still being written.
 * ------------------------------------------------------------------------------------------------ */

void fn_8019EF04(void* p);
void fn_8019F2C0(struct _ENEMY_WORK* self);
void fn_8019F768(struct _ENEMY_WORK* self);
void fn_8019F9BC(struct _ENEMY_WORK* self);
void fn_8019FC10(struct _ENEMY_WORK* self);
void fn_8019FE80(struct _ENEMY_WORK* self);
void fn_801A00F0(struct _ENEMY_WORK* self);
void fn_801A0210(struct _ENEMY_WORK* self, u32 a);
void fn_801A0684(struct _ENEMY_WORK* self, u32 a);
void fn_8019F378(struct _ENEMY_WORK* self, u32 a);
void fn_8019F56C(struct _ENEMY_WORK* self);
void fn_8019F5F4(struct _ENEMY_WORK* self);
void fn_8019F690(struct _ENEMY_WORK* self);
void fn_8019F70C(struct _ENEMY_WORK* self);
void fn_801A01C8(struct _ENEMY_WORK* self);
void fn_801A05B4(struct _ENEMY_WORK* self);
void fn_801A07E8(struct _ENEMY_WORK* self, u32 a);
void fn_801A08C8(struct _ENEMY_WORK* self);
void fn_801A0988(struct _ENEMY_WORK* self, u32 a);
void fn_801A0AD0(struct _ENEMY_WORK* self, u32 a);
void fn_801A0C48(struct _ENEMY_WORK* self, u32 a);
void fn_801A0FEC(struct _ENEMY_WORK* self);
void fn_801A10AC(struct _ENEMY_WORK* self);
void fn_801A11B0(struct _ENEMY_WORK* self);
void fn_801A12C8(struct _ENEMY_WORK* self);
void fn_801A1384(struct _ENEMY_WORK* self, u32 a);
void fn_801A1454(struct _ENEMY_WORK* self);
void fn_801A14FC(struct _ENEMY_WORK* self);
void fn_801A1624(struct _ENEMY_WORK* self);
void fn_801A16D4(struct _ENEMY_WORK* self, u32 a);
void fn_801A17B8(struct _ENEMY_WORK* self);
void fn_801A1894(struct _ENEMY_WORK* self);
void fn_801A1964(struct _ENEMY_WORK* self);
void fn_801A1A4C(struct _ENEMY_WORK* self, u32 a);
void fn_801A1BB8(struct _ENEMY_WORK* self);
void fn_801A1E60(struct _ENEMY_WORK* self);
void fn_801A1F54(struct _ENEMY_WORK* self);
void fn_801A2014(struct _ENEMY_WORK* self);
void fn_801A2350(struct _ENEMY_WORK* self);
void fn_801A246C(struct _ENEMY_WORK* self, u32 a);
void fn_801A26B4(struct _ENEMY_WORK* self, u32 a);
void fn_801A28C8(struct _ENEMY_WORK* self, u32 a);
void fn_801A2DAC(struct _ENEMY_WORK* self);
void fn_801A2E30(struct _ENEMY_WORK* self);
void fn_801A30D4(struct _ENEMY_WORK* self);
void fn_801A30E8(struct _ENEMY_WORK* self);
void fn_801A35CC(struct _ENEMY_WORK* self);
void fn_801A3C54(struct _ENEMY_WORK* self);
void fn_801A3DBC(struct _ENEMY_WORK* self);

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions.
 * ------------------------------------------------------------------------------------------------ */

/* 0x8019EF40 - an empty virtual body (the class's no-op slot). */
void fn_8019EF40(void) {}

/* 0x8019F12C - the band's motion arming tail shared by several states: arm mode 4, set the
 * `fn_80128AAC` window (6, 9) and refresh through `fn_80133BB4`. */
void fn_8019F12C(struct _ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    fn_80128AAC(self, 6, 9);
    fn_80133BB4(self);
}

/* 0x8019F174 - the same tail arming mode 0 with the `fn_80128AAC` window (1, 0). */
void fn_8019F174(struct _ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 1, 0);
    fn_80133BB4(self);
}

/* 0x8019F1BC - the two-state motion step: state 0 arms `em_mot_set_ck(self, 13, 30, 0)` plus the
 * `lbl_80798538` timer, state 1 waits for `em_mot_end_ck` and then runs `em_action_finish`. */
void fn_8019F1BC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 13, 30, 0);
        fn_801303EC(self, lbl_80798538);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F244 - the same shape arming `em_mot_set_ck(self, 1, 30, 0)` and completing through
 * `fn_801280F4`. */
void fn_8019F244(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019F2C0 - the per-sub-state dispatcher of the motion above: sub-states 0/1/2 arm through
 * `fn_8019F1BC`, sub-state 7 runs `fn_8019F244`. */
void fn_8019F2C0(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8019F1BC(self);
        break;
    case 1:
        fn_8019F1BC(self);
        break;
    case 2:
        fn_8019F1BC(self);
        break;
    case 7:
        fn_8019F244(self);
        break;
    }
}

/* 0x8019F2FC - state 0 arms `em_mot_set_ck(self, 21, 30, 0)`, state 1 completes through
 * `fn_8019F174`. */
void fn_8019F2FC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 21, 30, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8019F174(self);
        }
        break;
    }
}

/* 0x8019F56C - state 0 arms mode 13 (window 30) and the 150-frame `timer_0x020`; state 1 runs the
 * timer down and completes through `em_action_finish`. */
void fn_8019F56C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 13, 30, 0);
        self->timer_0x020 = 150;
        break;
    case 1: {
        s32 left = self->timer_0x020 - 1;
        self->timer_0x020 = left;
        if (left <= 0) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* 0x8019F5F4 - state 0 arms `em_mot_set(self, 26, 20, 0)` plus the two `fn_80129668` windows
 * (0,20)/(1,21), state 1 completes through `fn_8019F174`. */
void fn_8019F5F4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 26, 20, 0);
        fn_80129668(self, 0, 20);
        fn_80129668(self, 1, 21);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8019F174(self);
        }
        break;
    }
}

/* 0x8019F690 - state 0 arms `em_mot_set(self, 27, 20, 0)`, state 1 completes through
 * `em_action_finish`. */
void fn_8019F690(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 27, 20, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F70C - the per-sub-state dispatcher of the motion above. */
void fn_8019F70C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8019F2FC(self);
        break;
    case 1:
        fn_8019F378(self, 1);
        break;
    case 2:
        fn_8019F378(self, 0);
        break;
    case 3:
        fn_8019F56C(self);
        break;
    case 4:
        fn_8019F5F4(self);
        break;
    case 5:
        fn_8019F690(self);
        break;
    }
}

/* 0x8019F768 - the first of the four `fn_801A01C8` motion steps: state 0 arms
 * `em_mot_set(self, 20, 100, 0)` and the alpha store, state 1 is a ten-window joint pass
 * (`em_frame_check` -> `fn_8012933C`/`fn_80129724`) ending in the `fn_80134114` completion. */
void fn_8019F768(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 100, 0);
        fn_80134004(self, 0, lbl_80798538);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 14, 3);
        }
        if (em_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798568, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, lbl_8079856C, lbl_80798538) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798570, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, lbl_80798574, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 12, 3);
        }
        if (em_frame_check(self, 0, lbl_80798578, lbl_80798538) == 1) {
            fn_80129724(self, 1);
        }
        if (em_frame_check(self, 0, lbl_8079857C, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 13, 3);
        }
        if (em_frame_check(self, 0, lbl_80798580, lbl_80798538) == 1) {
            fn_80129724(self, 1);
        }
        if (em_frame_check(self, 0, lbl_80798584, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 13, 3);
        }
        if (fn_80134114(self, 0, 1) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019FC10 - the same ten-window pass as `fn_8019F768`, completing through `em_mot_end_ck` first
 * and `fn_80134114` second. */
void fn_8019FC10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 100, 0);
        fn_80134004(self, 0, lbl_80798538);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 14, 3);
        }
        if (em_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798568, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, lbl_8079856C, lbl_80798538) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798570, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, lbl_80798574, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 12, 3);
        }
        if (em_frame_check(self, 0, lbl_80798578, lbl_80798538) == 1) {
            fn_80129724(self, 1);
        }
        if (em_frame_check(self, 0, lbl_8079857C, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 13, 3);
        }
        if (em_frame_check(self, 0, lbl_80798580, lbl_80798538) == 1) {
            fn_80129724(self, 1);
        }
        if (em_frame_check(self, 0, lbl_80798584, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 13, 3);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        } else if (fn_80134114(self, 0, 1) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F9BC - the second motion step: the same ten-window pass against the other pooled floats
 * (`lbl_80798588`..) arming mode 22, completing through `fn_8019F174`. */
void fn_8019F9BC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 22, 100, 0);
        fn_80134004(self, 0, lbl_80798538);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798588, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 18, 3);
        }
        if (em_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798568, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798590, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, lbl_80798594, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 16, 3);
        }
        if (em_frame_check(self, 0, lbl_80798598, lbl_80798538) == 1) {
            fn_80129724(self, 1);
        }
        if (em_frame_check(self, 0, lbl_8079859C, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 17, 3);
        }
        if (em_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1) {
            fn_80129724(self, 1);
        }
        if (em_frame_check(self, 0, lbl_807985A0, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 17, 3);
        }
        if (fn_80134114(self, 0, 1) == 1) {
            fn_8019F174(self);
        }
        break;
    }
}

/* 0x8019FE80 - the second motion step with the `fn_8019FC10` completion pair. */
void fn_8019FE80(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 22, 100, 0);
        fn_80134004(self, 0, lbl_80798538);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798588, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 18, 3);
        }
        if (em_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798568, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798590, lbl_80798538) == 1) {
            fn_8012933C(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, lbl_80798594, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 16, 3);
        }
        if (em_frame_check(self, 0, lbl_80798598, lbl_80798538) == 1) {
            fn_80129724(self, 1);
        }
        if (em_frame_check(self, 0, lbl_8079859C, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 17, 3);
        }
        if (em_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1) {
            fn_80129724(self, 1);
        }
        if (em_frame_check(self, 0, lbl_807985A0, lbl_80798538) == 1) {
            fn_8012933C(self, 1, 17, 3);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_8019F174(self);
        } else if (fn_80134114(self, 0, 1) == 1) {
            fn_8019F174(self);
        }
        break;
    }
}

/* 0x801A00F0 - the fourth of the `fn_801A01C8` band's motion steps: state 0 arms the
 * `em_mot_set(self, 26, 20, 0)` pair with the two `fn_80129668` windows, state 1 arms
 * `fn_8012F504(self, 27, 20, 0, 1)` and advances, state 2 completes through `em_action_finish`. */
void fn_801A00F0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 26, 20, 0);
        fn_80129668(self, 0, 20);
        fn_80129668(self, 1, 21);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            fn_8012F504(self, 27, 20, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801A01C8 - the five-way motion dispatcher of the `fn_801A3DEC` action 2. */
void fn_801A01C8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8019F768(self);
        break;
    case 1:
        fn_8019F9BC(self);
        break;
    case 2:
        fn_8019FC10(self);
        break;
    case 3:
        fn_8019FE80(self);
        break;
    case 4:
        fn_801A00F0(self);
        break;
    }
}

/* 0x801A05B4 - action 3's two-way sub-state selector into `fn_801A0210` (0/1). */
void fn_801A05B4(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801A0210(self, 0);
        break;
    case 1:
        fn_801A0210(self, 1);
        break;
    }
}

/* 0x801A05E0 - action 6's first motion: state 0 arms `fn_8012F5C4(self, 1, 30, 0, 1)` plus the two
 * alpha/timer stores, state 1 waits on `fn_80134114(self, 0, 0)` and completes through
 * `fn_801280F4`. */
void fn_801A05E0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        fn_8012F5C4(self, 1, 30, 0, 1);
        fn_801303EC(self, lbl_807985A4);
        fn_80134004(self, 0, lbl_80798538);
        break;
    case 1:
        if (fn_80134114(self, 0, 0) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019F07C - selects the record's two dispatch words (0x8C8/0x8CC) from the area/motion, then
 * asks `fn_8019E840` which of the two pooled alpha values `fn_802B08DC` should blend. */
void fn_8019F07C(struct _ENEMY_WORK* self) {
    if (self->area_no == 1) {
        self->field_0x8C8 = fn_801260BC(self);
        self->field_0x8CC = fn_801260E0(self);
    } else if (self->action == 11) {
        self->field_0x8C8 = (u32)lbl_805AF9CC;
        self->field_0x8CC = (u32)lbl_805AFC08;
    } else {
        self->field_0x8C8 = (u32)lbl_805AE7C0;
        self->field_0x8CC = (u32)lbl_805AF268;
    }
    if (fn_8019E840(self) == 1) {
        fn_802B08DC(lbl_8079852C);
    } else {
        fn_802B08DC(lbl_80798528);
    }
}

/* 0x8019F70C's motion table above is documented here because the two cover one 0x254-byte band each
 * in the same order; `fn_8019F70C` holds the dispatcher that reaches both. */

/* 0x801A2DAC - the state_sub dispatcher of the `state`-band's second half: 0..14, with the double
 * entries 2/12, 8/11, 9/13, 10/14 passing the 0/1 selector to the four two-argument motion steps. */
void fn_801A2DAC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801A1894(self);
        break;
    case 1:
        fn_801A1964(self);
        break;
    case 2:
        fn_801A1A4C(self, 0);
        break;
    case 3:
        fn_801A1BB8(self);
        break;
    case 4:
        fn_801A1E60(self);
        break;
    case 5:
        fn_801A1F54(self);
        break;
    case 6:
        fn_801A2014(self);
        break;
    case 7:
        fn_801A2350(self);
        break;
    case 8:
        fn_801A246C(self, 0);
        break;
    case 9:
        fn_801A26B4(self, 0);
        break;
    case 10:
        fn_801A28C8(self, 0);
        break;
    case 11:
        fn_801A246C(self, 1);
        break;
    case 12:
        fn_801A1A4C(self, 1);
        break;
    case 13:
        fn_801A26B4(self, 1);
        break;
    case 14:
        fn_801A28C8(self, 1);
        break;
    }
}

/* 0x801A17B8 - action 6's `state_sub` dispatcher (27 entries): the first slot runs this band's
 * `fn_801A05E0`, the rest dispatch into the motion-mode table by (function, mode). */
void fn_801A17B8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801A05E0(self);
        break;
    case 1:
        fn_801A0684(self, 0);
        break;
    case 2:
        fn_801A0684(self, 1);
        break;
    case 3:
        fn_801A0684(self, 2);
        break;
    case 4:
        fn_801A0684(self, 0);
        break;
    case 5:
        fn_801A0684(self, 0);
        break;
    case 6:
        fn_801A07E8(self, 0);
        break;
    case 7:
        fn_801A08C8(self);
        break;
    case 8:
        fn_801A0988(self, 0);
        break;
    case 9:
        fn_801A0AD0(self, 0);
        break;
    case 10:
        fn_801A0C48(self, 0);
        break;
    case 11:
        fn_801A0FEC(self);
        break;
    case 12:
        fn_801A10AC(self);
        break;
    case 13:
        fn_801A11B0(self);
        break;
    case 14:
        fn_801A12C8(self);
        break;
    case 15:
        fn_801A1384(self, 0);
        break;
    case 16:
        fn_801A1454(self);
        break;
    case 17:
        fn_801A0988(self, 1);
        break;
    case 18:
        fn_801A14FC(self);
        break;
    case 19:
        fn_801A1624(self);
        break;
    case 20:
        fn_801A16D4(self, 0);
        break;
    case 21:
        fn_801A16D4(self, 1);
        break;
    case 22:
        fn_801A0AD0(self, 1);
        break;
    case 23:
        fn_801A1384(self, 1);
        break;
    case 24:
        fn_801A0AD0(self, 2);
        break;
    case 25:
        fn_801A07E8(self, 1);
        break;
    case 26:
        fn_801A0AD0(self, 3);
        break;
    }
}

/* 0x801A30D4 - the tail thunk into `fn_801251D8(self, &lbl_805B0478, 0, 0)`. */
void fn_801A30D4(struct _ENEMY_WORK* self) {
    fn_801251D8(self, lbl_805B0478, 0, 0);
}

/* 0x801A3DBC - the action dispatcher: 2 -> `fn_801A30E8`, 3 -> `fn_801A35CC`, 4 -> `fn_801A3C54`. */
void fn_801A3DBC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_801A30E8(self);
        break;
    case 3:
        fn_801A35CC(self);
        break;
    case 4:
        fn_801A3C54(self);
        break;
    }
}

/* 0x801A3DEC - the action-id dispatcher (0x1E5) plus the block's shared tail: the `field_0x1E2 == 4`
 * gate runs `fn_80136D14`. */
void fn_801A3DEC(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_8019F2C0(self);
        break;
    case 1:
        fn_8019F70C(self);
        break;
    case 2:
        fn_801A01C8(self);
        break;
    case 3:
        fn_801A05B4(self);
        break;
    case 6:
        fn_801A17B8(self);
        break;
    case 7:
        fn_801A2DAC(self);
        break;
    case 10:
        fn_801A2E30(self);
        break;
    case 11:
        fn_801A30D4(self);
        break;
    case 13:
        fn_801A3DBC(self);
        break;
    }
    if (self->field_0x1E2 == 4) {
        fn_80136D14(self);
    }
}

/* 0x801A0FEC - action 6's sub-state 11: the shared `em_busy_set`/`fn_80131E74` gate then a
 * two-state timer whose state 1 completes through `fn_8019F12C`. */
void fn_801A0FEC(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        fn_80134964(self, lbl_80570150, 0, 16, 0);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (fn_80134B0C(self, lbl_80570150) == 1) {
            fn_801303EC(self, fn_8013032C(self));
            fn_8019F12C(self);
        }
        break;
    }
}

/* 0x801A12C8 - action 6's sub-state 14: state 0 arms mode 8, the `em_mot_speed_set` blend and
 * `field_0x318 = lbl_807985D8 * fn_8012F8E4(self)`; state 1 runs `fn_801355C8(self, &field_0x1BC)`
 * and completes through `fn_801280F4`. */
void fn_801A12C8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 8, 60, 0);
        em_mot_speed_set(self, lbl_807985D4);
        fn_801303EC(self, lbl_807985A4);
        fn_801353F8(self);
        self->field_0x318 = lbl_807985D8 * fn_8012F8E4(self);
        break;
    case 1:
        fn_801355C8(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x801A1384 - action 6's sub-state 15/23: state 0 arms mode 1 and seeds `timer_0x020` from the
 * (mode) selector (240/150), state 1 runs the timer down and completes through `fn_801280F4`. */
void fn_801A1384(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        fn_801303EC(self, lbl_807985A4);
        switch ((u8)mode) {
        case 0:
            self->timer_0x020 = 240;
            break;
        case 1:
            self->timer_0x020 = 150;
            break;
        default:
            self->timer_0x020 = 150;
            break;
        }
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x801A1454 - action 6's sub-state 16: state 0 arms mode 1 and the 150-frame timer, state 1 runs
 * it down and completes through `fn_8019F12C`. */
void fn_801A1454(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        fn_801303EC(self, fn_8013032C(self));
        self->timer_0x020 = 150;
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            fn_801303EC(self, fn_8013032C(self));
            fn_8019F12C(self);
        }
        break;
    }
}

/* 0x801A14FC - action 6's sub-state 18: the shared `em_busy_set`/`fn_80131E74` gate, then state 0
 * arms mode 9 and the `field_0x318` scale, state 1 drives the two `em_frame_check` windows and
 * completes through `fn_8019F12C`. */
void fn_801A14FC(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        fn_8012F504(self, 9, 30, 0, 0);
        fn_801303EC(self, lbl_807985A4);
        fn_801353F8(self);
        self->field_0x318 = lbl_807985DC * fn_8012F8E4(self);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_807985E0, lbl_80798538) == 1) {
            fn_801303FC(self, lbl_807985E4 * fn_8012F8E4(self));
        }
        if (em_frame_check(self, 1, lbl_807985A8, lbl_80798538) == 1) {
            fn_801354F4(self, &self->field_0x1BC);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_801303EC(self, fn_8013032C(self));
            fn_8019F12C(self);
        }
        break;
    }
}

/* 0x801A1624 - action 6's sub-state 19: state 0 arms mode 1, state 1 waits for the 64-bit
 * `fn_80133C50` predicate and advances, state 2 completes through `fn_801280F4`. */
void fn_801A1624(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        fn_801303EC(self, lbl_807985A4);
        break;
    case 1:
        if (fn_80133C50(self, 64) == 1) {
            self->state++;
        }
        break;
    case 2:
        if (fn_8012F948(self) == 0) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x801A16D4 - action 6's sub-state 20/21: state 0 arms mode 1, state 1 drives the `field_0x1AC`
 * height against the (mode)-selected blend window and completes either through `fn_8019F12C` (mode
 * 0) or `fn_801280F4`. */
void fn_801A16D4(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        fn_8012F5C4(self, 1, 30, 0, 0);
        fn_801353F8(self);
        break;
    case 1:
        if ((u8)mode == 0) {
            fn_801303FC(self, lbl_807985E8);
            if (self->field_0x1AC <= lbl_807985EC) {
                fn_801303EC(self, fn_8013032C(self));
                fn_8019F12C(self);
            }
        } else {
            fn_801303FC(self, lbl_807985F0);
            if (self->field_0x1AC >= lbl_807985A4) {
                fn_801303EC(self, fn_8013032C(self));
                fn_801280F4(self);
            }
        }
        break;
    }
}

/* 0x801A4218 - action 7's status-bit pass: the (7,6) window clears the bits and re-arms through
 * `fn_8011E620(0x40000)` when `em_get_mot_no` reports 7 and the frame window matches; the (7,7)
 * window re-arms through `0x80000`; otherwise `fn_8011E5EC` seeds and area 1 gets the `0x40000`
 * clear. */
void fn_801A4218(struct _ENEMY_WORK* self) {
    if (em_act_ck(self, 7, 6) == 1) {
        fn_8011E630(self, -1);
        if ((u16)em_get_mot_no(self) == 7) {
            if (em_after_frame_check(self, 2, lbl_807987B4, lbl_80798538) == 1) {
                fn_8011E620(self, 0x40000);
            }
        }
    } else if (em_act_ck(self, 7, 7) == 1) {
        fn_8011E630(self, -1);
        fn_8011E620(self, 0x80000);
    } else {
        fn_8011E5EC(self);
        if (self->area_no == 1) {
            fn_8011E630(self, 0x40000);
        }
    }
}

/* 0x801A42F4 - transforms `tmp` through the joint's world matrix `joint`, then copies it into
 * `out`.  The four-argument call sites are the distance probes in `fn_801A28C8`. */
void fn_801A42F4(struct _ENEMY_WORK* self, u32 joint, Vec3* out, Vec3* tmp) {
    nw4r::math::MTX34 m;
    MTX34_ctor(&m);
    get_joint_wmat_em(self, joint, &m);
    mulVecMatAddTrans(tmp, &m);
    out->x = tmp->x;
    out->y = tmp->y;
    out->z = tmp->z;
}

} /* extern "C" */
