/* enemy/fn_801926EC.cpp - the enemy "em" action band's per-motion step group.
 *
 * `.text` 0x801926EC..0x801993E0 (93 functions, 0x6CF4 B); extab 0x8000EDF4..0x8000F084;
 * extabindex 0x8002A3D8..0x8002A7B0.  Registered from `proposal/801926EC_fn_801926EC.cpp`.
 *
 * Module `enemy`: the bracketing registered units are `enemy/fn_80191598.cpp` (ends 0x801926EC)
 * and `enemy/fn_801993E0.cpp` (starts 0x801993E0), and every callee out of the range is an
 * enemy-band function.  Language C++ (the range reaches the mangled `em_frame_check`).
 *
 * Name.  No `__FILE__` string survives in the range and the runtime dump answers only `zz_`
 * placeholders (`dumpmap.py lookup 0x801926EC` -> `zz_01926ec_`), so the map's own stem keeps the
 * file name (brief section 2, class 4).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py
 * lookup on the range's inventory: every name is a bare `.text` entry in
 * config/RMHE08/symbols.txt and the runtime dump answers only `zz_XXXXXXXX_` placeholders)
 *
 * Structure.  `fn_801993E0.cpp`'s `fn_80199B24` dispatches `self->action` (0x1E5) into this range's
 * six dispatchers `fn_80192F24`/`fn_80193394`/`fn_801938D8`/`fn_801953BC`/`fn_80196618`/`fn_801987E4`,
 * each of which switches on `self->state_sub` (0x1E6) and tail-calls one per-motion step.  Every
 * state machine reads `self->state` (0x005) and, at state 0, runs `fn_80130478` + a motion setter
 * (`fn_8012F62C`/`fn_8012F5C4`/`fn_8012F5B8`); at state 1 it waits on `fn_8012F93C(self) == 1` and
 * runs a completion hook.
 *
 * Status (official `report.json`, this worktree, split target `build/RMHE08/obj/enemy/fn_801926EC.o`).
 * 56 of the 93 functions are written and every written one is above the 80 % bar: 44 are byte-identical
 * and 12 are codegen near-misses (fn_801933DC 89.71, fn_801934EC 95.56, fn_80193964 97.98,
 * fn_8019485C 98.09, fn_80194CC8 96.10, fn_80195BCC 98.06, fn_8019610C 97.77, fn_801961C8 97.61,
 * fn_80196768 97.10, fn_80196A50 97.94, fn_80197C84 96.08, fn_80198FE8 96.77).  Unit: 33.057507 %
 * fuzzy, 6756 / 27892 `.text` bytes matched, 44 / 93 functions matched.
 *
 * The 37 unwritten functions are the residual (address order): fn_801926EC (400 B), fn_8019287C (60 B),
 * fn_801928BC (664 B), fn_80192B54 (188 B), fn_801936A8 (560 B), fn_80193AEC (476 B), fn_80193CC8 (288 B),
 * fn_80193DE8 (264 B), fn_80194078 (672 B), fn_801943A0 (484 B), fn_80194584 (356 B), fn_801946E8 (372 B),
 * fn_80194AE4 (484 B), fn_80194D6C (564 B), fn_80194FA0 (248 B), fn_80195214 (296 B), fn_80195504 (308 B),
 * fn_80195720 (320 B), fn_80195860 (876 B), fn_80195CA4 (220 B), fn_80195D80 (340 B), fn_801963A8 (624 B),
 * fn_8019687C (468 B), fn_80196BF4 (540 B), fn_80196E10 (1156 B), fn_80197294 (368 B), fn_80197404 (364 B),
 * fn_80197570 (328 B), fn_801976B8 (1288 B), fn_80197D50 (436 B), fn_80197F04 (588 B), fn_80198150 (1372 B),
 * fn_80198910 (1264 B), fn_80198E00 (272 B), fn_801990E0 (260 B), fn_801991E4 (508 B).  Each is a
 * `switch (self->state)` state machine of the same family as the ones above; they were not reached in
 * this round.
 *
 * Residual shapes on the 12 near-misses (all are register/evaluation ordering, not comprehension):
 *   * fn_801933DC - the target keeps a `clrlwi r4,r4,16` before `fn_8012F5B8` that MWCC folds away for
 *     the constant ternary; the motion value needs a u16 spelling the header's `s32` prototype cannot ask
 *     for.  4 B short.
 *   * fn_801934EC - `mr r3,self` is hoisted before the height ternary in the target, after it here; one row.
 *   * fn_80193964/fn_8019485C/fn_8019610C/fn_801961C8/fn_80195BCC/fn_80196768/fn_80196A50/fn_80197C84/
 *     fn_80198FE8 - single-row register colouring / `mr r3` placement around the `fn_8012F9*` wait.
 *   * fn_80194CC8 - the +0x020 timer loop's `lfs`/`fcmpo` ordering.
 */
#include "types.h"

#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h" /* fn_801280F4, fn_80128030 */
#include "enemy/fn_8012BDF4.h" /* fn_8012CF20 */
#include "enemy/fn_8012EC74.h" /* fn_80136B50 */
#include "enemy/fn_80138074.h"  /* fn_801390FC, fn_801391E8 */
#include "unsplit/enemy.h"     /* the plain enemy-band callees (rule 2 band header) */

/* The `.sdata2` and `.data` pool labels this range loads.  They are unsplit (no registered unit
 * claims a range covering them) and `stylelint`'s rule-2 `module()` answers `None`, so they stay
 * declared here as the counted "address band interleaves modules" gap - the same shape the landed
 * `enemy/fn_801993E0.cpp` uses for its own pool runs.  Never defined: the target addresses them. */
extern f32 lbl_80798238;
extern f32 lbl_80798264;
extern f32 lbl_80798268;
extern f32 lbl_8079826C;
extern f32 lbl_80798270;
extern f32 lbl_80798260;
extern f32 lbl_8079827C;
extern f32 lbl_80798288;
extern f32 lbl_80798290;
extern f32 lbl_80798298;
extern f32 lbl_80798304;
extern f32 lbl_80798308;
extern f32 lbl_8079830C;
extern f32 lbl_80798310;
extern f32 lbl_80798314;
extern f32 lbl_80798328;
extern f32 lbl_8079832C;
extern f32 lbl_80798338;
extern f32 lbl_80798370;
extern f32 lbl_80798398;
extern f32 lbl_8079839C;
extern f32 lbl_807983A0;
extern f32 lbl_807982E0;
/* The `fn_80134964`/`fn_80134B0C` descriptor blocks this band arms (`.data` tables). */
extern u8 lbl_80570050[];
extern u8 lbl_80570090[];
extern u8 lbl_805700D0[];
extern u8 lbl_80570110[];

extern "C" {

/* This range's own functions, declared so the dispatchers can tail-call them (they are defined
 * below, in address order). */
void fn_801926EC(struct _ENEMY_WORK* self, u8 act);
void fn_8019287C(void* self);
void fn_801928B8(void);
void fn_801928BC(struct _ENEMY_WORK* self, u8 kind, u8 arg);
void fn_80192B54(struct _ENEMY_WORK* self);
void fn_80192C10(struct _ENEMY_WORK* self);
void fn_80192C8C(struct _ENEMY_WORK* self);
void fn_80192D08(struct _ENEMY_WORK* self);
void fn_80192D84(struct _ENEMY_WORK* self);
void fn_80192E04(struct _ENEMY_WORK* self);
void fn_80192E84(struct _ENEMY_WORK* self);
void fn_80192F24(struct _ENEMY_WORK* self);
void fn_80192F78(struct _ENEMY_WORK* self);
void fn_80192FFC(struct _ENEMY_WORK* self);
void fn_80193078(struct _ENEMY_WORK* self);
void fn_801930F4(struct _ENEMY_WORK* self);
void fn_801931A4(struct _ENEMY_WORK* self);
void fn_80193220(struct _ENEMY_WORK* self);
void fn_8019329C(struct _ENEMY_WORK* self);
void fn_80193318(struct _ENEMY_WORK* self);
void fn_80193394(struct _ENEMY_WORK* self);
void fn_801933DC(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_801934EC(struct _ENEMY_WORK* self, u32 a);
void fn_801935A0(struct _ENEMY_WORK* self);
void fn_8019362C(struct _ENEMY_WORK* self);
void fn_801936A8(struct _ENEMY_WORK* self);
void fn_801938D8(struct _ENEMY_WORK* self);
void fn_80193964(struct _ENEMY_WORK* self, u8 a);
void fn_80193A34(struct _ENEMY_WORK* self);
void fn_80193AEC(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80193CC8(struct _ENEMY_WORK* self);
void fn_80193DE8(struct _ENEMY_WORK* self, u32 a);
void fn_80193EF0(struct _ENEMY_WORK* self);
void fn_80193F70(struct _ENEMY_WORK* self);
void fn_80193FF8(struct _ENEMY_WORK* self);
void fn_80194078(struct _ENEMY_WORK* self, u32 a);
void fn_80194318(struct _ENEMY_WORK* self);
void fn_801943A0(struct _ENEMY_WORK* self, u32 a);
void fn_80194584(struct _ENEMY_WORK* self);
void fn_801946E8(struct _ENEMY_WORK* self, u32 a);
void fn_8019485C(struct _ENEMY_WORK* self, u8 a);
void fn_80194938(struct _ENEMY_WORK* self);
void fn_801949B4(struct _ENEMY_WORK* self);
void fn_80194A64(struct _ENEMY_WORK* self);
void fn_80194AE4(struct _ENEMY_WORK* self);
void fn_80194CC8(struct _ENEMY_WORK* self);
void fn_80194D6C(struct _ENEMY_WORK* self);
void fn_80194FA0(struct _ENEMY_WORK* self);
void fn_80195098(struct _ENEMY_WORK* self);
void fn_8019515C(struct _ENEMY_WORK* self);
void fn_80195214(struct _ENEMY_WORK* self);
void fn_8019533C(struct _ENEMY_WORK* self);
void fn_801953BC(struct _ENEMY_WORK* self);
void fn_80195504(struct _ENEMY_WORK* self, u32 a);
void fn_80195638(struct _ENEMY_WORK* self, u32 a);
void fn_80195720(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80195860(struct _ENEMY_WORK* self);
void fn_80195BCC(struct _ENEMY_WORK* self, u8 a);
void fn_80195CA4(struct _ENEMY_WORK* self);
void fn_80195D80(struct _ENEMY_WORK* self);
void fn_80195ED4(struct _ENEMY_WORK* self);
void fn_80196004(struct _ENEMY_WORK* self);
void fn_8019610C(struct _ENEMY_WORK* self, u8 a);
void fn_801961C8(struct _ENEMY_WORK* self, u8 a);
void fn_80196278(struct _ENEMY_WORK* self);
void fn_801963A8(struct _ENEMY_WORK* self);
void fn_80196618(struct _ENEMY_WORK* self);
void fn_801966DC(struct _ENEMY_WORK* self);
void fn_80196768(struct _ENEMY_WORK* self);
void fn_8019687C(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80196A50(struct _ENEMY_WORK* self, u8 a);
void fn_80196B1C(struct _ENEMY_WORK* self);
void fn_80196BF4(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80196E10(struct _ENEMY_WORK* self, u32 a);
void fn_80197294(struct _ENEMY_WORK* self);
void fn_80197404(struct _ENEMY_WORK* self);
void fn_80197570(struct _ENEMY_WORK* self, u32 a);
void fn_801976B8(struct _ENEMY_WORK* self);
void fn_80197BC0(struct _ENEMY_WORK* self);
void fn_80197C84(struct _ENEMY_WORK* self);
void fn_80197D50(struct _ENEMY_WORK* self);
void fn_80197F04(struct _ENEMY_WORK* self);
void fn_80198150(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_801986AC(struct _ENEMY_WORK* self);
void fn_801987E4(struct _ENEMY_WORK* self);
void fn_80198F10(struct _ENEMY_WORK* self);
void fn_80198F14(struct _ENEMY_WORK* self);

/* ---------------------------------------------------------------------------------------------- */
/* the per-motion step group dispatched by fn_80192F24 (states 0,1,2,4,5,7)                         */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80192C10 - arm motion 1 for 4 frames; state 1 waits and re-enters the motion. */
void fn_80192C10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 1, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80192C8C - the same shape with motion 20. */
void fn_80192C8C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 20, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80192D08 - the same shape with motion 29. */
void fn_80192D08(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 29, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80192D84 - arm motion mode 2 with the four-argument setter (40/20/0/3) and the
 * `fn_80128030` completion. */
void fn_80192D84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F5C4(self, 40, 20, 0, 3);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80192E04 - the same shape with (54/20/0/3). */
void fn_80192E04(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F5C4(self, 54, 20, 0, 3);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80192E84 - the flag-arming step: sets the two action-start flags, arms mode 4 with motion 0xCE
 * and the fresh-scale pair, then waits on `fn_801280F4`. */
void fn_80192E84(struct _ENEMY_WORK* self) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F62C(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        fn_80132198(self);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x80192F24 - the action-0 dispatcher: `state_sub` selects this group's six steps. */
void fn_80192F24(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80192C10(self);
        break;
    case 1:
        fn_80192C8C(self);
        break;
    case 2:
        fn_80192D08(self);
        break;
    case 4:
        fn_80192D84(self);
        break;
    case 5:
        fn_80192E04(self);
        break;
    case 7:
        fn_80192E84(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the step group dispatched by fn_80193394 (states 0..7)                                           */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80192F78 - arm motion 6 for 10 frames; state 1 runs `fn_80128A14(self, 1, 5)`. */
void fn_80192F78(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 6, 10, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128A14(self, 1, 5);
        }
        break;
    }
}

/* 0x80192FFC - arm motion 7 for 4 frames. */
void fn_80192FFC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 7, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80193078 - arm motion 0x1A for 4 frames. */
void fn_80193078(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1A, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801930F4 - arm motion 0x1E for 4 frames; state 1 fires the joint effect when `em_frame_check`
 * reports the frame and then waits on `fn_8012F93C`. */
void fn_801930F4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1E, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798264, lbl_80798238) == 1) {
            self->field_0x358 = 1;
            fn_80136B50(self, -1, 7);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801931A4 - arm motion 0x72 for 10 frames. */
void fn_801931A4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x72, 10, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80193220 - arm motion 8 for 4 frames. */
void fn_80193220(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 8, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8019329C - arm motion 20 for 4 frames. */
void fn_8019329C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 20, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80193318 - arm motion 0x1D for 4 frames. */
void fn_80193318(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1D, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80193394 - the action-1 dispatcher (states 0..7). */
void fn_80193394(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80192F78(self);
        break;
    case 1:
        fn_80192FFC(self);
        break;
    case 2:
        fn_80193078(self);
        break;
    case 3:
        fn_801930F4(self);
        break;
    case 4:
        fn_801931A4(self);
        break;
    case 5:
        fn_80193220(self);
        break;
    case 6:
        fn_8019329C(self);
        break;
    case 7:
        fn_80193318(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the step group dispatched by fn_801938D8 (states 0..10)                                          */
/* ---------------------------------------------------------------------------------------------- */

/* 0x801933DC - arm motion `a == 1 ? 9 : 2` and pick the height by `b`.  The `default` arm is
 * written first: MWCC emits its body right after the compare chain (playbook row 37).  Both
 * selectors are narrowed at use (`(u8)a`/`switch ((u8)b)`) and the motion to `u16`, which is the
 * `clrlwi` pair the target keeps. */
void fn_801933DC(struct _ENEMY_WORK* self, u32 a, u32 b) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        {
            u16 motion = (u8)a == 1 ? 9 : 2;
            fn_8012F5B8(self, motion, 4, 0);
        }
        switch ((u8)b) {
        default:
            fn_80134004(self, 0, lbl_80798268);
            break;
        case 1:
            fn_80134004(self, 0, lbl_80798238);
            if (self->value_0x378 > lbl_8079826C) {
                self->value_0x378 = lbl_8079826C;
            }
            break;
        case 2:
            fn_80134004(self, 0, lbl_80798270);
            break;
        }
        break;
    case 1:
        if (fn_80134114(self, 0, 0x40) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801934EC - arm motion 21 and pick the height by `a`. */
void fn_801934EC(struct _ENEMY_WORK* self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 21, 4, 0);
        {
            f32 height = (u8)a == 1 ? lbl_80798270 : lbl_80798268;
            fn_80134004(self, 0, height);
        }
        break;
    case 1:
        if (fn_80134114(self, 0, 0x40) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801935A0 - arm the `lbl_80570050` descriptor and wait on `fn_80134B0C`. */
void fn_801935A0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_80134964(self, lbl_80570050, 0, 0, 0);
        break;
    case 1:
        if (fn_80134B0C(self, lbl_80570050) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8019362C - arm motion 0x1B for 4 frames. */
void fn_8019362C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1B, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801938D8 - the action-2 dispatcher (states 0..10). */
void fn_801938D8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801933DC(self, 0, 0);
        break;
    case 1:
        fn_801934EC(self, 0);
        break;
    case 2:
        fn_801935A0(self);
        break;
    case 3:
        fn_8019362C(self);
        break;
    case 4:
        fn_801933DC(self, 1, 0);
        break;
    case 5:
        fn_801936A8(self);
        break;
    case 6:
        fn_801933DC(self, 0, 1);
        break;
    case 7:
        fn_801933DC(self, 1, 1);
        break;
    case 8:
        fn_801934EC(self, 1);
        break;
    case 9:
        fn_801933DC(self, 0, 2);
        break;
    case 10:
        fn_801933DC(self, 1, 2);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the large per-action dispatchers                                                                 */
/* ---------------------------------------------------------------------------------------------- */

/* 0x801953BC - the action-5 dispatcher (states 0..39). */
void fn_801953BC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80193964(self, 0);
        break;
    case 1:
        fn_80193A34(self);
        break;
    case 2:
        fn_80193AEC(self, 0, 0);
        break;
    case 3:
        fn_80193AEC(self, 1, 0);
        break;
    case 4:
        fn_80193AEC(self, 2, 0);
        break;
    case 5:
        fn_80193AEC(self, 0, 1);
        break;
    case 6:
        fn_80193AEC(self, 1, 1);
        break;
    case 7:
        fn_80193AEC(self, 2, 1);
        break;
    case 8:
        fn_80193CC8(self);
        break;
    case 9:
        fn_80193DE8(self, 0);
        break;
    case 10:
        fn_80193EF0(self);
        break;
    case 11:
        fn_80193F70(self);
        break;
    case 12:
        fn_80193FF8(self);
        break;
    case 13:
        fn_80194078(self, 0);
        break;
    case 14:
        fn_80194318(self);
        break;
    case 15:
        fn_801943A0(self, 0);
        break;
    case 16:
        fn_80194584(self);
        break;
    case 17:
        fn_801946E8(self, 0);
        break;
    case 18:
        fn_8019485C(self, 0);
        break;
    case 19:
        fn_80194938(self);
        break;
    case 20:
        fn_801949B4(self);
        break;
    case 21:
        fn_80193DE8(self, 1);
        break;
    case 22:
        fn_80193AEC(self, 3, 0);
        break;
    case 23:
        fn_80193AEC(self, 3, 1);
        break;
    case 24:
        fn_80193964(self, 1);
        break;
    case 25:
        fn_80193AEC(self, 3, 2);
        break;
    case 26:
        fn_80193AEC(self, 3, 3);
        break;
    case 27:
        fn_801946E8(self, 1);
        break;
    case 28:
        fn_8019485C(self, 1);
        break;
    case 29:
        fn_80194A64(self);
        break;
    case 30:
        fn_80194AE4(self);
        break;
    case 31:
        fn_80194CC8(self);
        break;
    case 32:
        fn_80194D6C(self);
        break;
    case 33:
        fn_80194FA0(self);
        break;
    case 34:
        fn_80195098(self);
        break;
    case 35:
        fn_8019515C(self);
        break;
    case 36:
        fn_80195214(self);
        break;
    case 37:
        fn_8019533C(self);
        break;
    case 38:
        fn_80194078(self, 1);
        break;
    case 39:
        fn_801943A0(self, 1);
        break;
    }
}

/* 0x80196618 - the action-6 dispatcher (states 0..24). */
void fn_80196618(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80195504(self, 0);
        break;
    case 1:
        fn_80195638(self, 0);
        break;
    case 2:
        fn_80195720(self, 0, 2);
        break;
    case 3:
        fn_80195860(self);
        break;
    case 4:
        fn_80195BCC(self, 0);
        break;
    case 5:
        fn_80195CA4(self);
        break;
    case 7:
        fn_80195D80(self);
        break;
    case 10:
        fn_80195ED4(self);
        break;
    case 11:
        fn_80196004(self);
        break;
    case 12:
        fn_8019610C(self, 0);
        break;
    case 13:
        fn_801961C8(self, 0);
        break;
    case 14:
        fn_80196278(self);
        break;
    case 15:
        fn_80195720(self, 1, 0);
        break;
    case 16:
        fn_80195720(self, 0, 1);
        break;
    case 18:
        fn_80195504(self, 1);
        break;
    case 19:
        fn_80195638(self, 1);
        break;
    case 20:
        fn_80195720(self, 2, 0);
        break;
    case 21:
        fn_8019610C(self, 1);
        break;
    case 22:
        fn_801961C8(self, 1);
        break;
    case 23:
        fn_801963A8(self);
        break;
    case 24:
        fn_80195BCC(self, 1);
        break;
    }
}

/* 0x801987E4 - the action-7 dispatcher (states 0..31). */
void fn_801987E4(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801966DC(self);
        break;
    case 1:
        fn_80196768(self);
        break;
    case 2:
        fn_8019687C(self, 0, 0);
        break;
    case 3:
        fn_8019687C(self, 1, 0);
        break;
    case 4:
        fn_80196A50(self, 0);
        break;
    case 5:
        fn_80196B1C(self);
        break;
    case 6:
        fn_80196BF4(self, 0, 0);
        break;
    case 7:
        fn_80196E10(self, 0);
        break;
    case 8:
        fn_80196E10(self, 1);
        break;
    case 9:
        fn_80197294(self);
        break;
    case 10:
        fn_80196E10(self, 2);
        break;
    case 11:
        fn_80197404(self);
        break;
    case 12:
        fn_80196A50(self, 1);
        break;
    case 13:
        fn_80197570(self, 0);
        break;
    case 14:
        fn_80197570(self, 1);
        break;
    case 15:
        fn_80197570(self, 2);
        break;
    case 16:
        fn_80197570(self, 3);
        break;
    case 17:
        fn_801976B8(self);
        break;
    case 18:
        fn_80197BC0(self);
        break;
    case 19:
        fn_80197C84(self);
        break;
    case 20:
        fn_80196BF4(self, 1, 0);
        break;
    case 21:
        fn_80197D50(self);
        break;
    case 22:
        fn_80197F04(self);
        break;
    case 23:
        fn_80198150(self, 0, 0);
        break;
    case 24:
        fn_801986AC(self);
        break;
    case 25:
        fn_80198150(self, 1, 0);
        break;
    case 26:
        fn_80198150(self, 0, 1);
        break;
    case 27:
        fn_80198150(self, 1, 1);
        break;
    case 28:
        fn_8019687C(self, 0, 1);
        break;
    case 29:
        fn_8019687C(self, 1, 1);
        break;
    case 30:
        fn_80196BF4(self, 0, 1);
        break;
    case 31:
        fn_80196BF4(self, 1, 1);
        break;
    }
}

/* 0x80198F10 - forwards to this band's action-6 state-3 step. */
void fn_80198F10(struct _ENEMY_WORK* self) {
    fn_80195CA4(self);
}

/* 0x80198F14 - runs `fn_80198F10` while the action-8 sub-state is 0. */
void fn_80198F14(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_80198F10(self);
    }
}

/* 0x801928B8 - a bare `return` (the target body is a single `blr`). */
void fn_801928B8(void) {
}

/* ---------------------------------------------------------------------------------------------- */
/* the per-action steps the 0x801953BC/0x80196618/0x801987E4 dispatchers select                 */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80193EF0 - arm mode 2 with (0x32/0x14/0/3) and the `fn_80128030` completion. */
void fn_80193EF0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x32, 0x14, 0, 3);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80193F70 - arm mode 2 with (0x31/0x14/0/1) and the `fn_80128A14(self, 5, 0x1D)` completion. */
void fn_80193F70(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x31, 0x14, 0, 1);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128A14(self, 5, 0x1D);
        }
        break;
    }
}

/* 0x80193FF8 - arm mode 2 with (0x3A/0x0A/0/1) and the `fn_80128030` completion. */
void fn_80193FF8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x3A, 0xA, 0, 1);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80194938 - arm mode 2 with motion 0x68 for 10 frames; completion `fn_80128030`. */
void fn_80194938(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x68, 0xA, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80194A64 - arm mode 2 with (0x2E/6/0/1); completion `fn_80128030`. */
void fn_80194A64(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x2E, 6, 0, 1);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8019533C - arm mode 2 with (0x36/0x14/0/3); completion `fn_80128030`. */
void fn_8019533C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x36, 0x14, 0, 3);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x801966DC - arm motion 0x54 for 10 frames with the `fn_80129668(self, 0, 1)` pair. */
void fn_801966DC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x54, 0xA, 0);
        fn_80129668(self, 0, 1);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80196A50 - arm motion 0x57 for 10 frames and the two joint slots whose ids depend on `a`. */
void fn_80196A50(struct _ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x57, 0xA, 0);
        {
            u32 first, second;
            if (a == 1) {
                first = 0x16;
                second = 0x17;
            } else {
                first = 6;
                second = 7;
            }
            fn_8012933C(self, 0, first, 8);
            fn_8012933C(self, 1, second, 0x10);
        }
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80194CC8 - the release step: clears the bit pair, arms motion 0x80 and runs the +0x020 timer
 * down to zero, then completes with `fn_80128A14(self, 5, 0x20)`. */
void fn_80194CC8(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x80, 4, 0);
        self->timer_0x020 = 0xBE;
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            fn_80128A14(self, 5, 0x20);
        }
        break;
    }
}

/* 0x80195BCC - the flag-arming release step: arms motion 0xCD, refreshes the scale, then either
 * the `fn_80128A14(self, 6, 1)` completion (`a == 1`) or `fn_801280F4`. */
void fn_80195BCC(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    fn_8012CF20(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F5B8(self, 0xCD, 0, 0);
        fn_80132198(self);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            if (a == 1) {
                fn_80128A14(self, 6, 1);
            } else {
                fn_801280F4(self);
            }
        }
        break;
    }
}

/* 0x80194318 - arm motion 0x3D with the common `fn_80127F48` completion. */
void fn_80194318(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x3D, 0, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80193964 - the requested-motion step: `a == 1` primes the two-state release first; state 1
 * waits on the `lbl_80570090` descriptor and runs `fn_80128030` when it finishes. */
void fn_80193964(struct _ENEMY_WORK* self, u8 a) {
    if (a == 1) {
        fn_8012CF20(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_80570090, 0, 1, 0);
        break;
    case 1:
        if (fn_80134B0C(self, lbl_80570090) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80798298);
        } else {
            fn_80136D4C(self, lbl_80798260);
        }
        break;
    }
}

/* 0x80193A34 - the same shape for the `lbl_805700D0` descriptor. */
void fn_80193A34(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_805700D0, 0, 1, 0);
        break;
    case 1:
        if (fn_80134B0C(self, lbl_805700D0) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80798298);
        } else {
            fn_80136D4C(self, lbl_80798260);
        }
        break;
    }
}

/* 0x80198F28 - the same descriptor shape as `fn_80193964` with the
 * `fn_80128A14(self, 0xD, 1)` completion. */
void fn_80198F28(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_80570090, 0, 1, 0);
        break;
    case 1:
        if (fn_80134B0C(self, lbl_80570090) == 1) {
            fn_80128A14(self, 0xD, 1);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80798298);
        } else {
            fn_80136D4C(self, lbl_80798260);
        }
        break;
    }
}

/* 0x8019515C - the same descriptor shape for `lbl_80570110`. */
void fn_8019515C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_80134964(self, lbl_80570110, 0, 1, 0);
        break;
    case 1:
        if (fn_80134B0C(self, lbl_80570110) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80798298);
        } else {
            fn_80136D4C(self, lbl_80798260);
        }
        break;
    }
}

/* 0x801949B4 - arm motion 0x1F for 4 frames, fire the joint effect on the frame and complete with
 * `fn_80128030`. */
void fn_801949B4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x1F, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798264, lbl_80798238) == 1) {
            self->field_0x358 = 1;
            fn_80136B50(self, -1, 7);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80196B1C - arm motion 0x4E with the two joint slots 8/9; while the frame effect has not
 * expired it keeps the two-state release armed. */
void fn_80196B1C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F5B8(self, 0x4E, 0xA, 0);
        fn_8012933C(self, 0, 8, 8);
        fn_8012933C(self, 1, 9, 0x10);
        fn_80130CDC(self, -0xC);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80798338, lbl_80798238) == 0) {
            fn_80136D4C(self, lbl_80798260);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80195638 - the flag-armed step: arms motion 0xD0, then holds the two flags while the frame
 * effects run.  The second argument the dispatcher passes is unused. */
void fn_80195638(struct _ENEMY_WORK* self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xD0, 0, 0);
        fn_801303EC(self, lbl_80798238);
        fn_80136D14(self);
        self->field_0x359 = 1;
        break;
    case 1:
        fn_8012CF20(self);
        if (em_frame_check(self, 2, lbl_80798290, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_frame_check(self, 2, lbl_8079827C, lbl_80798238) == 1) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80196768 - the three-state step: arm motion 0x59, advance to 0x5A when the frame gate fires,
 * then complete with `fn_80127F48`. */
void fn_80196768(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x59, 0xA, 0);
        fn_8012933C(self, 0, 3, 3);
        fn_80134004(self, 0, lbl_80798328);
        fn_80130CDC(self, -0xC);
        break;
    case 1:
        if (fn_80134114(self, 0, 0x40) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x5A, 2, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_8079832C, lbl_80798238) == 1) {
            fn_80129724(self, 0);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80197C84 - arm motion 0xA for 8 frames and the `lbl_80798270` blend; completes with
 * `fn_80128A70(self, 7, 0)`. */
void fn_80197C84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F504(self, 0xA, 8, 0, 1);
        fn_80134004(self, 0x10, lbl_80798270);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80798370, lbl_80798238) == 1 &&
            (fn_80134114(self, 0, 0x40) == 1 || fn_8012F93C(self) == 1)) {
            fn_80128A70(self, 7, 0);
        }
        break;
    }
}

/* 0x801986AC - the three-state step: arm motion 0xD6, advance to 0xD3 on completion, then the
 * `lbl_8079839C` joint release and `fn_80128030`. */
void fn_801986AC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F504(self, 0xD6, 0x14, 0, 1);
        fn_8012933C(self, 0, 2, 3);
        fn_801303EC(self, lbl_80798238);
        fn_80136D14(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798398, lbl_80798238) == 1) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 2);
            fn_8012F5B8(self, 0xD3, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_8079839C, lbl_80798238) == 1) {
            fn_80129724(self, 0);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80197BC0 - the two-stage rotation step: state 0 advances the +0x1C0 angle by 0x8000, state 1
 * arms motion 0x7F, state 2 completes with `fn_80127F48`. */
void fn_80197BC0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x7E, 0, 0);
        self->field_0x1C0 += 0x8000;
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x7F, 0, 0);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801961C8 - the flag-armed step: arms motion 0xCE, refreshes the scale and completes through
 * `fn_801280F4`.  `a` only selects the two-state release at entry. */
void fn_801961C8(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    if (a == 1) {
        fn_8012CF20(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F5B8(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019610C - the same arming as `fn_801961C8` with the `fn_80133C50(self, 0x300)` completion. */
void fn_8019610C(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    if (a == 1) {
        fn_8012CF20(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F5B8(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        fn_80132198(self);
        break;
    case 1:
        if (fn_80133C50(self, 0x300) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019485C - arm motion 0xCA for 10 frames, advance to 0xDE on completion, then `fn_80128030`. */
void fn_8019485C(struct _ENEMY_WORK* self, u8 a) {
    if (a == 1) {
        fn_8012CF20(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xCA, 0xA, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 2);
            fn_8012F5B8(self, 0xDE, 0, 0);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80195098 - arm motion 0x7F, advance to 0x82 on completion, then `fn_80127F48`. */
void fn_80195098(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x7F, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x82, 0, 0xBC);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80195ED4 - arm motion 0xDA with the joint slot 0xF; the frame effects re-arm the flag and
 * fire the joint effect, and `fn_80127F48` completes the step. */
void fn_80195ED4(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xDA, 0, 0);
        fn_80129668(self, 0, 0xF);
        fn_801303EC(self, lbl_80798238);
        fn_80130CDC(self, -0xC);
        self->field_0x359 = 1;
        fn_80136D14(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798288, lbl_80798238) == 1) {
            fn_80136D14(self);
        }
        if (em_frame_check(self, 2, lbl_80798308, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_frame_check(self, 3, lbl_8079830C, lbl_80798288) == 1) {
            fn_80133C50(self, 0x400);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80196004 - arm motion 0xDB with the joint slot 0x10 and the two frame-effect hooks. */
void fn_80196004(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xDB, 0, 0);
        fn_801303EC(self, lbl_80798238);
        fn_80129668(self, 0, 0x10);
        fn_80130CDC(self, -0xC);
        self->field_0x359 = 1;
        fn_80136D14(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798310, lbl_80798238) == 1) {
            fn_80136D14(self);
        }
        if (em_frame_check(self, 2, lbl_80798314, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x80198FE8 - arm mode 2 with (0x37/0x14/0/1) and the `lbl_807983A0` blend; state 1 waits on the
 * descriptor and advances to (0x2F/0x28/0/1), completing with `fn_80128A14(self, 0xD, 2)`. */
void fn_80198FE8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 2);
        fn_8012F504(self, 0x37, 0x14, 0, 1);
        fn_80134004(self, 0x12, lbl_807983A0);
        break;
    case 1:
        fn_80136D4C(self, lbl_807982E0);
        if (fn_80134114(self, 0, 0x80) == 1 && fn_8012F948(self) == 0) {
            self->state++;
            fn_8012F504(self, 0x2F, 0x28, 0, 1);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80128A14(self, 0xD, 2);
        }
        break;
    }
}

} /* extern "C" */
