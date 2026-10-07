/* enemy/fn_801B7020.cpp - the em036 enemy's motion/act-instruction group.
 * RANGE. .text 0x801B7020-0x801B98C8 (50 functions); .ctors 0x8056F34C-0x8056F350 (`fn_801B985C`, the static
 *   initializer), .data 0x805B2118-0x805B2804 (from `em036_prog_tbl`), .bss 0x806A7AB8-0x806A7AD0,
 *   .sdata 0x80791AB0-0x80791AB8, .sdata2 0x80798CF8-0x80798D88, extab, extabindex.
 * SEAM. The band 0x801B7020-0x801E0ADC is six TUs: each one's `.data` opens with its `emNNN_prog_tbl`, each ends with
 *   its static initializer (the `.ctors` words 0x8056F34C/350/354/358) and the `.sdata2` pool repeats a value at each
 *   change - em036 here, em040 `enemy/em040_ai.cpp`, em006 `enemy/em006_prog.cpp`, em004 `enemy/em004_act.cpp`, em005
 *   `enemy/em005_act.cpp`, em007 `enemy/em007_act.cpp`.  Right edge: `fn_801B985C` is this TU's static initializer and
 *   the 0.0 pool entry repeats at `lbl_80798DA4` from `fn_801B98C8` on; left edge: the 4-byte `fn_801B701C` sits in the
 *   0.0 dedupe window 0x801B701C-0x801B70A4 and stays with `enemy/em034_prog.cpp` (unproven either way).
 * NAMES. The map stem; the dump answers `zz_01b7020_` for 0x801B7020 and nothing for the later rows, and no
 *   `__FILE__` string is referenced.  em036 is a GUESS from the prog table that opens the TU's data.
 * RESIDUALS. 26 rows unwritten: 0x801B7F78-0x801B98C8 (the static initializer among them).
 *  - `fn_801B70A4`: retail copies the 12-byte +0x08..+0x14 block through r3/r0 pairs, ours one word at a time
 *    through r0 (a `memcpy` call and a float-member assignment score lower; the three-u32 struct assignment is best);
 *  - `fn_801B7BDC`: one `fmuls` operand order (`f1 * f0` in retail) after the `-5.0f` local is narrowed.
 *   flipcheck: `.ctors`/`.data`/`.sdata` claimed, not emitted; `.sdata2`/`.text`/extab/extabindex short of the
 *   claim.
 * SHAPES. `#pragma peephole off` over the bodies (`fn_801B7048`'s `joint_flags & 6` and `fn_801B73A0`'s
 *   `bits_0x1EC & 7` tests keep `clrlwi`/`rlwinm` + `cmpwi`); `self->state++`, not `self->state = self->state + 1`
 *   (the `+ 1` store drags a `clrlwi` in front of the `stb`); a `switch` for the sparse dispatches (grouped
 *   `case 0: case 1: case 2:` labels in `fn_801B7590` turn the chain into a range test); `(s32)(u8)mode` for the
 *   gates retail compares with `cmpwi`; `EmProgWork` is this unit's view because it keeps an `f32` at +0x328 where
 *   `enemy/ENEMY_WORK.h` has an `s16`, and names the +0x328..+0x358 cluster, the +0x310/+0x320 vectors and the
 *   +0x210/+0x228 words.
 */
#include "types.h"

#include "nw4r/math.h"

/* Retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs the peephole pass folds into record forms
 * (`fn_801B7048`'s `joint_flags` test, `fn_801B73A0`'s `bits_0x1EC` test; playbook 39). */
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

/* The C++-mangled callees, declared at C++ scope for the map's manglings; `rotVecY` (0x80051064) is
 * `fn_8004CAD8.cpp`'s. */
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

/* The 0x20-byte out record `em_ground_rec_clear` prepares and `em_ground_rec_find` fills: the mode byte at +0x03
 * (`fn_801B7118`), the vector at +0x08 and the u32 at +0x18 `fn_801B71F4` narrows to u16.
 * size: 0x20 */
struct EmSelRec {
    /* +0x00 */ u8 unused_0x00[0x03];
    /* +0x03 */ u8 mode;          /* 0 or 2: the random threshold `fn_801B7118` picks */
    /* +0x04 */ u8 unused_0x04[0x04];
    /* +0x08 */ nw4r::math::VEC3 vec_0x08; /* zeroed by `em_ground_rec_clear` through `VEC3_ctor` */
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
    /* +0x01A */ u16 field_0x01A;        /* the id `em_ground_rec_find` is handed */
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

/* `vec3_sub_assign` subtracts `b` from `self` in place (`ef/ef_postfield.cpp`, whose header clashes with `mh3_pad.h`'s
 * `VEC3_ctor`/`setVec3`); `lbl_805B2188` is this unit's 0x60-byte `.data` table `em_key_curve_eval` is handed. */
extern "C" {
void vec3_sub_assign(nw4r::math::VEC3* dst, const nw4r::math::VEC3* src);
/* 0x80051EE0 (`fn_8004CAD8.cpp`): r3 `out`, r4 `in`, f1 the scale it keeps in f31 while `VEC3_ctor` zeroes `out`;
 * the tree spells it with three and four arguments, so it is declared here. */
void vec3_scale(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 scale);
extern u8 lbl_805B2188[];
}

/* The range. */

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

/* Rotates a 45-degree-up vector by the work's Y rotation and hands it to the aim setter. */
extern "C" void fn_801B70A4(EmProgWork* self) {
    nw4r::math::VEC3 rot;
    EmVecWords out;

    VEC3_ctor(&rot);
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 45.0f;
    rotVecY(&rot, self->field_0x1C0);
    out = *(EmVecWords*)&rot;
    fn_80130350((struct _ENEMY_WORK*)self, &out);
}

/* Picks the random-motion kind for the work's `field_0x01A` entry: 0 = no record, 1/2 = the kind the
 * selection record's mode chooses (0 → the 10 % branch, 2 → the 60 % branch). */
extern "C" s32 fn_801B7118(u16 id) {
    EmSelRec rec;
    s32 roll;

    em_ground_rec_clear(&rec);
    if (move_work_state_ck() == 0) {
        return 0;
    }
    if (em_ground_rec_find((u16)id, &rec) == 0) {
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
 * kinds (1 and 4) or restart the motion set (`em_fall_height_get`/`em_fall_start`). */
extern "C" void fn_801B71F4(EmProgWork* self, u8 mode) {
    EmSelRec rec;
    s32 pick;

    em_ground_rec_clear(&rec);
    self->field_0x32C = 0;
    if (em_ground_rec_find(self->field_0x01A, &rec) == 1) {
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
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        if (self->field_0x00A & 1) {
            em_act_arm_unless_down((struct _ENEMY_WORK*)self, 3, 10);
        } else {
            em_act_arm_unless_down((struct _ENEMY_WORK*)self, 0, 3);
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

/* Retargets the program instruction (r4 the `{ code, ... }` record, r5 the value): only the code 11 row with an
 * empty 3-bit `bits_0x1EC` field runs, moving the byte 0 → 29 and 5 → 30. */
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

/* The first two-step motion of the set: step 1 arms the motion set (`em_move_mode_set` with mode 0 and the
 * `em_mot_set_ck` window 4/4/0), step 2 hands over to `em_action_finish` once `em_mot_end_ck` reports done. */
extern "C" void fn_801B7494(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 4, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same two-step shape with the motion set restarted (`em_fall_height_get`/`em_fall_start`) and the
 * 1/4/0 window, handing over to `em_action_finish_fall`. */
extern "C" void fn_801B7510(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
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
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 4, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same shape with the 12/2/0 window. */
extern "C" void fn_801B7648(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 12, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The three-step run-in: step 1 restarts the motion set and arms the 101/2/0 window plus the
 * distance/speed pair, step 2 waits for `em_ground_ck`, step 3 ends the motion. */
extern "C" void fn_801B76C4(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 101, 2, 0);
        em_dive_start((struct _ENEMY_WORK*)self);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.y = -5.0f;
        self->value_0x320 = -1.0f;
        break;
    case 1:
        CancelFade((struct _ENEMY_WORK*)self);
        em_fall_height_get((struct _ENEMY_WORK*)self);
        if (em_ground_ck((struct _ENEMY_WORK*)self) == 1) {
            self->state++;
            em_move_mode_set((struct _ENEMY_WORK*)self, 0);
            em_mot_set((struct _ENEMY_WORK*)self, 102, 2, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same two-step shape with the 103/2/0 window. */
extern "C" void fn_801B77B8(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 103, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same two-step shape with the 107/4/0 window through `em_mot_set`. */
extern "C" void fn_801B7834(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set((struct _ENEMY_WORK*)self, 107, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
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
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set((struct _ENEMY_WORK*)self, 5, 4, 0);
        break;
    case 1:
        if (em_frame_check((struct _ENEMY_WORK*)self, 1, 6.0f, 3000.0f) == 1) {
            self->state++;
            em_fall_height_get((struct _ENEMY_WORK*)self);
            em_fall_start((struct _ENEMY_WORK*)self);
            em_move_vec2_clr((struct _ENEMY_WORK*)self);
            self->vec_0x310.y = em_key_curve_eval((struct _ENEMY_WORK*)self, lbl_805B2188);
            em_move_offset_apply((struct _ENEMY_WORK*)self);
        }
        break;
    case 2:
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.y = em_key_curve_eval((struct _ENEMY_WORK*)self, lbl_805B2188);
        em_move_offset_apply((struct _ENEMY_WORK*)self);
        if (em_frame_check((struct _ENEMY_WORK*)self, 1, 16.0f, 3000.0f) == 1) {
            self->state++;
            em_mot_set((struct _ENEMY_WORK*)self, 1, 10, 0);
            self->timer_0x020 = 10;
            em_move_vec2_clr((struct _ENEMY_WORK*)self);
        }
        break;
    case 3:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The charge run-in: restarts the motion set, takes the speed from the mode's constant (`em_approach_start`,
 * 8 frames) and scales the normalised vector to the target into the run-in slot. */
extern "C" void fn_801B7A68(EmProgWork* self, u8 mode) {
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 out;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 2, 20, 0);
        em_approach_start((struct _ENEMY_WORK*)self, (s32)(u8)mode == 1 ? -450.0f : -80.0f, 8);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        copyVec3(&vec, &self->target);
        vec.y += 60.0f;
        vec3_sub_assign(&vec, &self->pos);
        if (vec3_length_sq((const f32*)&vec) > 0.001f) {
            vec3_normalize_into(&vec, &vec);
            vec3_scale(&out, &vec, 8.0f);
            copyVec3(&self->vec_0x310, &out);
        }
        self->timer_0x020 = 240;
        break;
    case 1:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        if (em_approach_step((struct _ENEMY_WORK*)self, 0, 0) == 1 || self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
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
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 3, 20, 0);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
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
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        if (self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
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
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 1, 4, 0);
        em_dive_start((struct _ENEMY_WORK*)self);
        break;
    case 1:
        em_dive_step((struct _ENEMY_WORK*)self);
        em_fall_height_get((struct _ENEMY_WORK*)self);
        if (em_ground_ck((struct _ENEMY_WORK*)self) == 1) {
            self->state++;
            em_move_mode_set((struct _ENEMY_WORK*)self, 0);
            em_mot_set((struct _ENEMY_WORK*)self, 6, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* A two-step run-in that ends as soon as `em_turn_to_target` reports the end of the motion. */
extern "C" void fn_801B7DB0(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 1, 4, 0);
        break;
    case 1:
        if (em_turn_to_target((struct _ENEMY_WORK*)self, 0x400) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same run-in vector as `fn_801B7A68` with a shorter 150-frame hold and no speed constant. */
extern "C" void fn_801B7E34(EmProgWork* self, u8 mode) {
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 out;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 2, 20, 0);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        copyVec3(&vec, &self->target);
        vec.y += 60.0f;
        vec3_sub_assign(&vec, &self->pos);
        if (vec3_length_sq((const f32*)&vec) > 0.001f) {
            vec3_normalize_into(&vec, &vec);
            vec3_scale(&out, &vec, 10.0f);
            copyVec3(&self->vec_0x310, &out);
        }
        self->timer_0x020 = 150;
        break;
    case 1:
        if (mode == 0) {
            em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        }
        if (self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
    fn_801B7048(self);
}

/* The unit's `.bss` (0x806A7AB8-0x806A7AD0): the two-vector record (a GUESS: a pair of model-space points) its
 * static constructor `fn_801B985C` builds and the `.data` tables point at. */
VEC3 vec_pair_801B7020_0[2];  /* +0x806A7AB8 */
