/* enemy/fn_801B7020.cpp - the enemy band's motion/act-instruction group.
 *
 * `.text` 0x801B7020..0x801BD6C0 (123 functions, 0x66A0 B), extab 0x8000F7EC..0x8000FACC (92
 * records), extabindex 0x8002B2CC..0x8002B71C (92 x 12 B), one `.ctors` word at
 * 0x8056F34C..0x8056F350.  Registered from `proposal/801B7020_fn_801B7020.cpp`.
 *
 * Module `enemy`.  Both bracketing registered units are `enemy/*` (`enemy/fn_80191598.cpp` below at
 * 0x80191598..0x801926EC, `lobby/lobby_scene.c` only after the whole 0x801926EC..0x801EC9E0 band), and
 * every callee out of the range is the enemy work API (`em_die_ck__FP11_ENEMY_WORK`,
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `get_em_scale__FP11_ENEMY_WORK`, `get_move_work_adrs`,
 * `em_parts_damage_level_get`).  Language C++: the range reaches mangled symbols whose mangling only a
 * class/namespace declaration reproduces (`setMatColor__6MHcharFUl12_GXChannelID8_GXColorb`,
 * `rotVecY__FPQ34nw4r4math4VEC3Ul`, `shell_se_req__FP5_se_wPQ34nw4r4math4VEC3UcUl`).
 *
 * Seam.  The proposal's own edges: below is the still-unclaimed 0x801926EC..0x801B7020 hole (whose
 * last function, the 4-byte `fn_801B701C`, is not this unit's) and above is `proposal/801BD6C0`.  The
 * unit's extabindex run holds exactly the 92 records of this range's framed functions, bracketed by
 * the records of 0x801B6FB0 (below) and 0x801BD6C0 (above), so the extab/extabindex edges are exact.
 *
 * Name.  The map has only `fn_XXXXXXXX` for this range - the runtime dump answers `zz_01b7020_` for
 * 0x801B7020 and nothing at all for the later rows (`tools/symbols/dumpmap.py lookup`) - and no
 * `__FILE__` string is referenced anywhere in the range, so the file keeps the map's stem.
 *
 * Status (this branch).  The first 24 rows (0x801B7020..0x801B7F74, 0x1A0C B) are written; 21 of them
 * are byte-identical under the official `report generate` metric and 3 carry a residual (below).  The
 * remaining 99 rows are a residual of the round, not a claim: they are unwritten, so the unit's own
 * score is 14.89 % fuzzy over 26272 B (3192 B matched).  Every written row was measured one at a time
 * with `python tools/units/recompile.py enemy/fn_801B7020 --measure <symbol>` and re-measured in the
 * tree-wide report.
 *
 * Source shapes worth keeping (each measured):
 *   * `#pragma peephole off` (one scoped pragma, docs/plan.md 8.2): the build's -O3 peephole fuses the
 *     `clrlwi`/`rlwinm` + `cmpwi` pairs the byte gates need into their record forms (`clrlwi.` /
 *     `rlwinm.`).  `fn_801B7048`'s `joint_flags & 6` test and `fn_801B73A0`'s `bits_0x1EC & 7` test
 *     only match with it off; every already-identical row is unchanged by it.
 *   * `self->state++` and not `self->state = self->state + 1`: the u8 store of a plain `+ 1` expression
 *     drags a `clrlwi r0,r0,24` in front of the `stb`, which retail does not have (it costs one
 *     instruction in 17 rows).
 *   * a `switch` is the idiom the sparse dispatches are written in: it makes MWCC emit the `cmpwi`
 *     chain retail has (`fn_801B73A0`'s value chain, `fn_801B7494`'s state dispatch,
 *     `fn_801B7590`/`fn_801B78B0`'s sub-state dispatch - `fn_801B7590` scores 71.53 % as grouped
 *     `case 0: case 1: case 2:` labels, because MWCC then picks a range test instead of the chain).
 *   * the mode gates that retail compares with `cmpwi` (a *signed* int compare) need the operand
 *     spelled `(s32)(u8)mode`: a plain `u8` promotes to unsigned and yields `cmplwi`
 *     (`fn_801B71F4` 98.88 -> 100.00, `fn_801B7A68` 97.20 -> 97.85).
 *   * `fn_801B70A4`'s +0x08..+0x14 copy is word-wise in retail (`lwz`/`stw` pairs, not `lfs`/`stfs`):
 *     a three-u32 struct assignment reproduces the shape, a `memcpy` call does not (84.83 %).
 *
 * Residuals of the three rows that are not byte-identical:
 *   * `fn_801B70A4` 92.93 - everything matches but the register pair of the 12-byte copy: retail
 *     loads two words into r3/r0 and stores them (the inlined-copy shape), this build loads and stores
 *     one word at a time through r0.  Tried: a `memcpy(&out, &rot, 0xC)` call (84.83), a
 *     float-member assignment (the shape the target does *not* use), and a 3x u32 struct assignment
 *     (best).
 *   * `fn_801B7A68` 97.85 - the pool-reference spelling of the five constants and one row of the
 *     `fn_801B7E34`-style vector block; the instruction sequence itself matches.
 *   * `fn_801B7BDC` 99.84 - one `fmuls` operand order (`f1 * f0` in retail, `f0 * f1` here) after the
 *     `-5.0f` local is narrowed.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py
 * lookup on the range's inventory: all 123 names are bare `.text` entries in
 * config/RMHE08/symbols.txt, the whole range's data references are pool floats/integers and `.bss`
 * blocks, and the runtime dump has only `zz_XXXXXXXX_` placeholders for them)
 *
 * Types.  This unit reads `_ENEMY_WORK` bytes that the shared views name differently: it keeps a f32
 * counter at +0x328 (the shared `include/enemy/ENEMY_WORK.h` carries a s16 `field_0x328` there for
 * `fn_80170804`) and it drives a +0x328..+0x358 cluster, +0x310/+0x320 vectors and the +0x210/+0x228
 * words that no shared header names yet.  The unit therefore carries its own view (`EmProgWork`),
 * exactly as `enemy/fn_80191598.cpp` (`EmActWork`), `enemy/fn_8013ACC4.cpp` (`EmWork`) and
 * `enemy/fn_8013F764.cpp` (`EmcWork`) do; the outbox asks for the measured fields to be folded into
 * the shared header.
 */
#include "types.h"

#include "nw4r/math.h"

/* Retail keeps the unfused `clrlwi`+`cmpwi` and `rlwinm`+`cmpwi` pairs the -O3 peephole pass folds
 * into their record forms (`clrlwi.` / `rlwinm.`): `fn_801B7048`'s `joint_flags` test and
 * `fn_801B73A0`'s `bits_0x1EC` test both need it.  The same per-unit lever `enemy/fn_80191598.cpp`
 * documents (docs/plan.md 8.2; the playbook's row 39). */
#pragma peephole off

#include "mh3_pad.h"

#include "Runtime.PPCEABI.H/memcpy.h"

#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"

#include "enemy/enemy_control.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"

#include "unsplit/enemy.h"

/* The work record's own tag: this unit reaches it only through `EmProgWork` and casts at the callee
 * boundary (a pointer cast, never arithmetic - docs/plan.md 6.5 rule 6). */
struct _ENEMY_WORK;

/* The C++-mangled callees, declared at C++ scope so the front-end reproduces the map's mangling
 * (docs/plan.md 6.5 rule 9: a caller never spells it).  `rotVecY` belongs in the header of the unit
 * that owns it (0x80051064, `fn_8004CAD8.cpp`) but cannot go there: `src/enemy/fn_801550FC.cpp`
 * carries a C-linkage declaration of the same name, so a C++-scope declaration in that shared header
 * fails that unit with `(10505) illegal overloading`.  The outbox carries the conflict. */
void rotVecY(nw4r::math::VEC3* v, u32 angle);

/* ------------------------------------------------------------------------------------------------ */
/* this range's view of the records it reads                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* Three 32-bit words, the shape the +0x08..+0x14 copy of `fn_801B70A4` is made in (the target copies
 * it with `lwz`/`stw` pairs, not with the `lfs`/`stfs` pairs a float-member assignment produces).
 * size: 0x0C */
struct EmVecWords {
    /* +0x00 */ u32 x;
    /* +0x04 */ u32 y;
    /* +0x08 */ u32 z;
};

/* The 0x20-byte out record `fn_80125F54` prepares and `fn_801421E4` fills.  Both of this range's
 * readers measure it the same way: a byte at +0x03 (the mode `fn_801B7118` switches on), the 3-float
 * vector the initialiser zeroes at +0x08, and a u32 at +0x18 that `fn_801B71F4` narrows to u16.
 * size: 0x20 */
struct EmSelRec {
    /* +0x00 */ u8 unused_0x00[0x03];
    /* +0x03 */ u8 mode;          /* 0 or 2: the random threshold `fn_801B7118` picks */
    /* +0x04 */ u8 unused_0x04[0x04];
    /* +0x08 */ nw4r::math::VEC3 vec_0x08; /* zeroed by `fn_80125F54` through `fn_80043EA8` */
    /* +0x14 */ u8 unused_0x14[0x04];
    /* +0x18 */ u32 value_0x18;   /* the id `fn_801B71F4` latches into the work's +0x32E */
};

/* The enemy work record, as this range's accesses measure it.  `state` (+0x05) is the per-motion step
 * every dispatcher in the range switches on, `timer_0x020` the countdown they run down, and the
 * +0x328..+0x358 cluster the per-action aim/timer state.
 * size: 0xB18 */
struct EmProgWork {
    /* +0x000 */ u8 active;              /* nonzero while the record is in use */
    /* +0x001 */ u8 unused_0x001[0x005 - 0x001];
    /* +0x005 */ u8 state;               /* the per-motion step (0..5) the dispatchers switch on */
    /* +0x006 */ u8 field_0x006;
    /* +0x007 */ u8 field_0x007;
    /* +0x008 */ u8 unused_0x008[0x00A - 0x008];
    /* +0x00A */ u8 field_0x00A;         /* bit 1: the random-motion flag `fn_801B7118` feeds */
    /* +0x00B */ u8 unused_0x00B[0x00F - 0x00B];
    /* +0x00F */ u8 field_0x00F;
    /* +0x010 */ u8 field_0x010;
    /* +0x011 */ u8 unused_0x011[0x01A - 0x011];
    /* +0x01A */ u16 field_0x01A;        /* the id `fn_801421E4` is handed */
    /* +0x01C */ u8 unused_0x01C[0x020 - 0x01C];
    /* +0x020 */ s32 timer_0x020;        /* the step countdown (`bgt` keeps a positive one running) */
    /* +0x024 */ u8 unused_0x024[0x188 - 0x024];
    /* +0x188 */ nw4r::math::VEC3 pos;   /* +0x18C is the height the range clamps */
    /* +0x194 */ u8 unused_0x194[0x1BC - 0x194];
    /* +0x1BC */ u32 field_0x1BC;        /* rotation angle (wraps at 0x10000) */
    /* +0x1C0 */ u32 field_0x1C0;        /* the Y rotation `rotVecY` is handed */
    /* +0x1C4 */ u32 field_0x1C4;
    /* +0x1C8 */ u8 unused_0x1C8[0x1CC - 0x1C8];
    /* +0x1CC */ f32 value_0x1CC;        /* armed with 1.0f by the charge branch */
    /* +0x1D0 */ u8 unused_0x1D0[0x1E1 - 0x1D0];
    /* +0x1E1 */ u8 area_no;
    /* +0x1E2 */ u8 field_0x1E2;
    /* +0x1E3 */ u8 unused_0x1E3[0x1E6 - 0x1E3];
    /* +0x1E6 */ u8 state_sub;           /* the sub-state several dispatchers switch on */
    /* +0x1E7 */ u8 unused_0x1E7[0x1EC - 0x1E7];
    /* +0x1EC */ u16 bits_0x1EC;         /* low 3 bits gate `fn_801B73A0` */
    /* +0x1EE */ u8 unused_0x1EE[0x210 - 0x1EE];
    /* +0x210 */ f32 value_0x210;        /* the base of the walk-in height clamp */
    /* +0x214 */ u8 unused_0x214[0x228 - 0x214];
    /* +0x228 */ u16 joint_flags;        /* bits 2-3 gate the walk-in clamp */
    /* +0x22A */ u8 unused_0x22A[0x310 - 0x22A];
    /* +0x310 */ nw4r::math::VEC3 vec_0x310; /* the run-in vector; .y is armed with -5.0f */
    /* +0x31C */ u8 unused_0x31C[0x320 - 0x31C];
    /* +0x320 */ f32 value_0x320;        /* armed with -1.0f beside it */
    /* +0x324 */ u8 unused_0x324[0x328 - 0x324];
    /* +0x328 */ f32 value_0x328;        /* the walk-in/charge distance the motion mode picks */
    /* +0x32C */ u8 field_0x32C;         /* cleared on entry by `fn_801B71F4` */
    /* +0x32D */ u8 unused_0x32D[0x32E - 0x32D];
    /* +0x32E */ u16 field_0x32E;        /* latched from the selection record */
    /* +0x330 */ u16 field_0x330;        /* armed with 0xFF, or 150 for the random motion */
    /* +0x332 */ u8 unused_0x332[0x36C - 0x332];
    /* +0x36C */ nw4r::math::VEC3 target; /* the height the clamp measures against (+0x370) */
    /* +0x378 */ u8 unused_0x378[0x38B - 0x378];
    /* +0x38B */ u8 field_0x38B;         /* cleared once `fn_801B701C` reports done */
    /* +0x38C */ u8 unused_0x38C[0x43B - 0x38C];
    /* +0x43B */ u8 field_0x43B;         /* the action/mode byte `fn_8013072C` compares */
    /* +0x43C */ u8 unused_0x43C[0x440 - 0x43C];
    /* +0x440 */ s16 field_0x440;        /* the 150-frame timer `fn_801B73F0` tests */
    /* +0x442 */ u8 unused_0x442[0x834 - 0x442];
    /* +0x834 */ u8 field_0x834;
    /* +0x835 */ u8 unused_0x835[0xB14 - 0x835];
    /* +0xB14 */ u32 field_0xB14;
};

/* This unit's own rows that an earlier row calls - they are defined further down, in address order. */
extern "C" void fn_801B78F8(EmProgWork* self);

/* The two foreign callees this unit cannot reach through a header, each for a documented reason:
 *   * `fn_800B0B90` is `Vec* fn_800B0B90(Vec* self, Vec* b)` in the owner's own source
 *     (`src/ef/fn_800AEE48.cpp:330` - it subtracts `b` from `self` in place), but that owner's header
 *     includes `include/ef.h`, whose `fn_80043EA8`/`fn_80041E8C` spellings clash with
 *     `include/mh3_pad.h`'s on the very same C-linkage symbols (MWCC `(10197) illegal function
 *     overloading`), so the header is not includable here.  The map symbol is the plain
 *     `fn_800B0B90`, hence C linkage.
 *   * `lbl_805B2188` is the 0x60-byte `.data` table `fn_80135644` is handed; no registered unit
 *     claims that `.data` range, so it has no owner header - the same in-file spelling
 *     `src/Pl/pl_act.cpp` uses for its data labels.  Both are `shared-file` requests in the outbox. */
extern "C" {
void fn_800B0B90(nw4r::math::VEC3* dst, const nw4r::math::VEC3* src);
/* 0x80050850 (normalise in place: its body moves r3/r4 into r29/r30 and calls `fn_800508A8` and
 * `fn_800508AC` with one each) and 0x80051EE0 (r3 `out`, r4 `in`, f1 the scale it saves in f31 before
 * zeroing `out` through `fn_80043EA8`).  Their owner is `fn_8004CAD8.cpp`, but that unit's header
 * cannot carry them: five other units declare the same two C-linkage names with four different
 * spellings (`src/ef/eft001.cpp`, `src/ef/eft007.cpp`, `src/ef/eft019.cpp`,
 * `src/ef/fn_801173AC.cpp`, `src/g3d/fn_80075DCC.cpp`, the last with a four-argument form), so any
 * header declaration breaks at least one of them with MWCC `(10197)/(10115)`.  The outbox carries
 * the fold-in as a `shared-file` request. */
void fn_80050850(nw4r::math::VEC3* v, nw4r::math::VEC3* in);
void fn_80051EE0(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 scale);
extern u8 lbl_805B2188[];
}

/* ------------------------------------------------------------------------------------------------ */
/* the range                                                                                          */
/* ------------------------------------------------------------------------------------------------ */

/* The walk-in altitude clamp: while the work's height (`pos.y`) is more than 1500 above its target
 * height (`target.y`), drop it by 20 per frame. */
extern "C" void fn_801B7020(EmProgWork* self) {
    f32 height = self->pos.y;
    if (height > 1500.0f + self->target.y) {
        self->pos.y = height - 20.0f;
    }
}

/* The second half of the same clamp: for the part flags that walk in (bits 2-3 of `joint_flags`), the
 * height is at least `value_0x210` plus 30 units of the model scale. */
extern "C" void fn_801B7048(EmProgWork* self) {
    if (self->joint_flags & 0x6) {
        f32 step = 30.0f * get_em_scale((struct _ENEMY_WORK*)self);
        f32 height;
        height = self->value_0x210 + step;
        if (self->pos.y < height) {
            self->pos.y = height;
        }
    }
}

/* Rotate a 45-degree-up vector by the work's Y rotation and hand it to the aim setter. */
extern "C" void fn_801B70A4(EmProgWork* self) {
    nw4r::math::VEC3 rot;
    EmVecWords out;

    fn_80043EA8(&rot);
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 45.0f;
    rotVecY(&rot, self->field_0x1C0);
    out = *(EmVecWords*)&rot;
    fn_80130350((struct _ENEMY_WORK*)self, &out);
}

/* Pick the random-motion kind for the work's `field_0x01A` entry: 0 = no record, 1/2 = the kind the
 * selection record's mode chooses (0 → the 10 % branch, 2 → the 60 % branch). */
extern "C" s32 fn_801B7118(u16 id) {
    EmSelRec rec;
    s32 roll;

    fn_80125F54(&rec);
    if (fn_800CF280() == 0) {
        return 0;
    }
    if (fn_801421E4((u16)id, &rec) == 0) {
        return 0;
    }
    roll = (u16)ran_suu(0) % 100;
    if (rec.mode == 2) {
        return (roll < 60) + 1;
    }
    if (rec.mode == 0) {
        return (roll < 10) + 1;
    }
    return 0;
}

/* Enter / step the motion `mode` (r4): latch the selection record, then either branch on the random
 * kinds (1 and 4) or restart the motion set (`fn_80130248`/`fn_801305C4`). */
extern "C" void fn_801B71F4(EmProgWork* self, u8 mode) {
    EmSelRec rec;
    s32 pick;

    fn_80125F54(&rec);
    self->field_0x32C = 0;
    if (fn_801421E4(self->field_0x01A, &rec) == 1) {
        self->field_0x32E = (u16)rec.value_0x18;
    } else {
        self->field_0x32E = 0;
    }
    self->field_0x330 = 0xFF;
    if ((s32)(u8)mode == 1 || (s32)(u8)mode == 4) {
        pick = (s8)fn_801B7118(self->field_0x01A);
        if (pick == 1) {
            self->field_0x00A &= 0xFD;
        } else if (pick == 2) {
            self->field_0x00A |= 0x02;
        }
    } else {
        fn_80130248((struct _ENEMY_WORK*)self);
        fn_801305C4((struct _ENEMY_WORK*)self);
        if (self->field_0x00A & 1) {
            fn_80128A8C((struct _ENEMY_WORK*)self, 3, 10);
        } else {
            fn_80128A8C((struct _ENEMY_WORK*)self, 0, 3);
        }
        if (self->field_0x00F == 0) {
            pick = (s8)fn_801B7118(self->field_0x01A);
            if (pick == 1) {
                self->field_0x00A &= 0xFD;
            } else if (pick == 2) {
                self->field_0x00A |= 0x02;
            }
        }
    }
    if (self->field_0x00A & 2) {
        self->value_0x328 = 3000.0f;
        self->value_0x1CC = 1.0f;
        fn_80131FA0((struct _ENEMY_WORK*)self, 80);
    } else if (self->field_0x00A & 1) {
        self->value_0x328 = 1200.0f;
        self->field_0x330 = 150;
    } else {
        self->value_0x328 = 1500.0f;
    }
}

/* Retarget the program instruction (r4 points at the `{ code, ... }` record, r5 at the value): only
 * the code 11 row with an empty 3-bit `bits_0x1EC` field runs, and it moves the byte 0 → 29 and
 * 5 → 30. */
extern "C" void fn_801B73A0(EmProgWork* self, u8* record, u8* value) {
    if (*record != 11) {
        return;
    }
    if (self->bits_0x1EC & 7) {
        return;
    }
    switch (*value) {
    case 0:
        *value = 29;
        break;
    case 5:
        *value = 30;
        break;
    }
}

/* An empty row of the instruction table (its whole body is the return). */
extern "C" void fn_801B73EC(void) {
}

/* The per-frame death/mode check: clear the "already reported" byte once the program reports done,
 * then shut the action down when the work is dead or its 150-frame timer has run out. */
extern "C" void fn_801B73F0(EmProgWork* self) {
    if (fn_801B701C((struct _ENEMY_WORK*)self) == 0 && self->field_0x38B == 1) {
        self->field_0x38B = 0;
    }
    if (em_die_ck((struct _ENEMY_WORK*)self)) {
        return;
    }
    if (self->field_0x834 == 1) {
        fn_8013072C((struct _ENEMY_WORK*)self, 2, 0);
    }
    if (self->field_0x43B == 2 && self->field_0x440 > 150) {
        fn_8013072C((struct _ENEMY_WORK*)self, 0, 0);
        self->field_0x834 = 0;
    }
}

/* The first two-step motion of the set: step 1 arms the motion set (`fn_80130478` with mode 0 and the
 * `fn_8012F62C` window 4/4/0), step 2 hands over to `fn_80127F48` once `fn_8012F93C` reports done. */
extern "C" void fn_801B7494(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478((struct _ENEMY_WORK*)self, 0);
        fn_8012F62C((struct _ENEMY_WORK*)self, 4, 4, 0);
        break;
    case 1:
        if (fn_8012F93C((struct _ENEMY_WORK*)self) == 1) {
            fn_80127F48((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same two-step shape with the motion set restarted (`fn_80130248`/`fn_801305C4`) and the
 * 1/4/0 window, handing over to `fn_80127FE4`. */
extern "C" void fn_801B7510(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248((struct _ENEMY_WORK*)self);
        fn_801305C4((struct _ENEMY_WORK*)self);
        fn_8012F62C((struct _ENEMY_WORK*)self, 1, 4, 0);
        break;
    case 1:
        if (fn_8012F93C((struct _ENEMY_WORK*)self) == 1) {
            fn_80127FE4((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The sub-state dispatcher of the first set: sub-states 0..2 are `fn_801B7494`, sub-state 3 is
 * `fn_801B7510`. */
extern "C" void fn_801B7590(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        fn_801B7494(self);
        break;
    case 1:
        fn_801B7494(self);
        break;
    case 2:
        fn_801B7494(self);
        break;
    case 3:
        fn_801B7510(self);
        break;
    }
}

/* A second copy of the `fn_801B7494` motion (the target's step bodies are byte-identical). */
extern "C" void fn_801B75CC(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478((struct _ENEMY_WORK*)self, 0);
        fn_8012F62C((struct _ENEMY_WORK*)self, 4, 4, 0);
        break;
    case 1:
        if (fn_8012F93C((struct _ENEMY_WORK*)self) == 1) {
            fn_80127F48((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same shape with the 12/2/0 window. */
extern "C" void fn_801B7648(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478((struct _ENEMY_WORK*)self, 0);
        fn_8012F62C((struct _ENEMY_WORK*)self, 12, 2, 0);
        break;
    case 1:
        if (fn_8012F93C((struct _ENEMY_WORK*)self) == 1) {
            fn_80127F48((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The three-step run-in: step 1 restarts the motion set and arms the 101/2/0 window plus the
 * distance/speed pair, step 2 waits for `fn_80130008`, step 3 ends the motion. */
extern "C" void fn_801B76C4(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248((struct _ENEMY_WORK*)self);
        fn_801305C4((struct _ENEMY_WORK*)self);
        fn_8012F5B8((struct _ENEMY_WORK*)self, 101, 2, 0);
        fn_80134E8C((struct _ENEMY_WORK*)self);
        fn_801353F8((struct _ENEMY_WORK*)self);
        self->vec_0x310.y = -5.0f;
        self->value_0x320 = -1.0f;
        break;
    case 1:
        CancelFade((struct _ENEMY_WORK*)self);
        fn_80130248((struct _ENEMY_WORK*)self);
        if (fn_80130008((struct _ENEMY_WORK*)self) == 1) {
            self->state++;
            fn_80130478((struct _ENEMY_WORK*)self, 0);
            fn_8012F5B8((struct _ENEMY_WORK*)self, 102, 2, 0);
        }
        break;
    case 2:
        if (fn_8012F93C((struct _ENEMY_WORK*)self) == 1) {
            fn_80127F48((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same two-step shape with the 103/2/0 window. */
extern "C" void fn_801B77B8(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478((struct _ENEMY_WORK*)self, 0);
        fn_8012F62C((struct _ENEMY_WORK*)self, 103, 2, 0);
        break;
    case 1:
        if (fn_8012F93C((struct _ENEMY_WORK*)self) == 1) {
            fn_80127F48((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same two-step shape with the 107/4/0 window through `fn_8012F5B8`. */
extern "C" void fn_801B7834(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478((struct _ENEMY_WORK*)self, 0);
        fn_8012F5B8((struct _ENEMY_WORK*)self, 107, 4, 0);
        break;
    case 1:
        if (fn_8012F93C((struct _ENEMY_WORK*)self) == 1) {
            fn_80127F48((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The sub-state dispatcher of the second set: sub-states 0..4 are the five motion bodies above. */
extern "C" void fn_801B78B0(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        fn_801B75CC(self);
        break;
    case 1:
        fn_801B7648(self);
        break;
    case 2:
        fn_801B76C4(self);
        break;
    case 3:
        fn_801B77B8(self);
        break;
    case 4:
        fn_801B7834(self);
        break;
    }
}

/* The one-shot gate of the third set: sub-state 0 runs `fn_801B78F8`. */
extern "C" void fn_801B7A54(EmProgWork* self) {
    if (self->state_sub == 0) {
        fn_801B78F8(self);
    }
}

/* The three-step run-in of the third set: step 1 arms the 5/4/0 window, step 2 holds the run-in
 * vector and step 3 times the last 10 frames out. */
extern "C" void fn_801B78F8(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478((struct _ENEMY_WORK*)self, 0);
        fn_8012F5B8((struct _ENEMY_WORK*)self, 5, 4, 0);
        break;
    case 1:
        if (em_frame_check((struct _ENEMY_WORK*)self, 1, 6.0f, 3000.0f) == 1) {
            self->state++;
            fn_80130248((struct _ENEMY_WORK*)self);
            fn_801305C4((struct _ENEMY_WORK*)self);
            fn_801353F8((struct _ENEMY_WORK*)self);
            self->vec_0x310.y = fn_80135644((struct _ENEMY_WORK*)self, lbl_805B2188);
            fn_80135418((struct _ENEMY_WORK*)self);
        }
        break;
    case 2:
        fn_801353F8((struct _ENEMY_WORK*)self);
        self->vec_0x310.y = fn_80135644((struct _ENEMY_WORK*)self, lbl_805B2188);
        fn_80135418((struct _ENEMY_WORK*)self);
        if (em_frame_check((struct _ENEMY_WORK*)self, 1, 16.0f, 3000.0f) == 1) {
            self->state++;
            fn_8012F5B8((struct _ENEMY_WORK*)self, 1, 10, 0);
            self->timer_0x020 = 10;
            fn_801353F8((struct _ENEMY_WORK*)self);
        }
        break;
    case 3:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            fn_80127FE4((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The charge run-in: the motion set is restarted, the speed comes from the mode-dependent constant
 * (`fn_80134004`, 8 frames), and the vector to the target is normalised and scaled into the run-in
 * slot. */
extern "C" void fn_801B7A68(EmProgWork* self, u8 mode) {
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 out;

    fn_80043EA8(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248((struct _ENEMY_WORK*)self);
        fn_801305C4((struct _ENEMY_WORK*)self);
        fn_8012F5B8((struct _ENEMY_WORK*)self, 2, 20, 0);
        fn_80134004((struct _ENEMY_WORK*)self, 8, (s32)(u8)mode == 1 ? -450.0f : -80.0f);
        fn_801353F8((struct _ENEMY_WORK*)self);
        fn_80041E40(&vec, &self->target);
        vec.y += 60.0f;
        fn_800B0B90(&vec, &self->pos);
        if (fn_80050EDC((const f32*)&vec) > 0.001f) {
            fn_80050850(&vec, &vec);
            fn_80051EE0(&out, &vec, 8.0f);
            fn_80041E40(&self->vec_0x310, &out);
        }
        self->timer_0x020 = 240;
        break;
    case 1:
        fn_80133C50((struct _ENEMY_WORK*)self, 0x1000);
        if (fn_80134114((struct _ENEMY_WORK*)self, 0, 0) == 1 || self->timer_0x020 <= 0) {
            fn_80127FE4((struct _ENEMY_WORK*)self);
        } else {
            fn_80135418((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
    fn_801B7048(self);
}

/* The held run-in: the vector is kept for 100 frames (10 in the short mode) and re-aimed every frame. */
extern "C" void fn_801B7BDC(EmProgWork* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248((struct _ENEMY_WORK*)self);
        fn_801305C4((struct _ENEMY_WORK*)self);
        fn_8012F5B8((struct _ENEMY_WORK*)self, 3, 20, 0);
        fn_801353F8((struct _ENEMY_WORK*)self);
        f32 begin = -5.0f;
        self->vec_0x310.z = begin;
        self->timer_0x020 = 100;
        if (mode == 1) {
            self->vec_0x310.z = begin * 8.0f;
            self->timer_0x020 = 10;
        }
        rotVecY(&self->vec_0x310, self->field_0x1C0);
        break;
    case 1:
        fn_80133C50((struct _ENEMY_WORK*)self, 0x1000);
        if (self->timer_0x020 <= 0) {
            fn_80127FE4((struct _ENEMY_WORK*)self);
        } else {
            fn_80135418((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
    fn_801B7048(self);
}

/* The three-step run-in of the second set (the 1/4/0 window, then the 6/4/0 one). */
extern "C" void fn_801B7CD4(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248((struct _ENEMY_WORK*)self);
        fn_801305C4((struct _ENEMY_WORK*)self);
        fn_8012F5B8((struct _ENEMY_WORK*)self, 1, 4, 0);
        fn_80134E8C((struct _ENEMY_WORK*)self);
        break;
    case 1:
        fn_80134F18((struct _ENEMY_WORK*)self);
        fn_80130248((struct _ENEMY_WORK*)self);
        if (fn_80130008((struct _ENEMY_WORK*)self) == 1) {
            self->state++;
            fn_80130478((struct _ENEMY_WORK*)self, 0);
            fn_8012F5B8((struct _ENEMY_WORK*)self, 6, 4, 0);
        }
        break;
    case 2:
        if (fn_8012F93C((struct _ENEMY_WORK*)self) == 1) {
            fn_80127F48((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* A two-step run-in that ends as soon as `fn_80133C50` reports the end of the motion. */
extern "C" void fn_801B7DB0(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248((struct _ENEMY_WORK*)self);
        fn_801305C4((struct _ENEMY_WORK*)self);
        fn_8012F62C((struct _ENEMY_WORK*)self, 1, 4, 0);
        break;
    case 1:
        if (fn_80133C50((struct _ENEMY_WORK*)self, 0x400) == 1) {
            fn_80127FE4((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same run-in vector as `fn_801B7A68` with a shorter 150-frame hold and no speed constant. */
extern "C" void fn_801B7E34(EmProgWork* self, u8 mode) {
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 out;

    fn_80043EA8(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248((struct _ENEMY_WORK*)self);
        fn_801305C4((struct _ENEMY_WORK*)self);
        fn_8012F5B8((struct _ENEMY_WORK*)self, 2, 20, 0);
        fn_801353F8((struct _ENEMY_WORK*)self);
        fn_80041E40(&vec, &self->target);
        vec.y += 60.0f;
        fn_800B0B90(&vec, &self->pos);
        if (fn_80050EDC((const f32*)&vec) > 0.001f) {
            fn_80050850(&vec, &vec);
            fn_80051EE0(&out, &vec, 10.0f);
            fn_80041E40(&self->vec_0x310, &out);
        }
        self->timer_0x020 = 150;
        break;
    case 1:
        if (mode == 0) {
            fn_80133C50((struct _ENEMY_WORK*)self, 0x1000);
        }
        if (self->timer_0x020 <= 0) {
            fn_80127FE4((struct _ENEMY_WORK*)self);
        } else {
            fn_80135418((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
    fn_801B7048(self);
}
