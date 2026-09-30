/* enemy/fn_801926EC.cpp - the enemy "em" action band's per-motion step group.
 *
 * `.text` 0x801926EC..0x8019E670 (0xBF84 B); extab 0x8000EDF4..0x8000F124; extabindex
 * 0x8002A3D8..0x8002A8A0; `.ctors` 0x8056F340..0x8056F344; `.data` 0x805AD3DC..0x805AE750; `.bss`
 * 0x806A7A70..0x806A7A88.  Registered from `proposal/801926EC_fn_801926EC.cpp`; the unit absorbed the
 * former `enemy/fn_801993E0.cpp` (0x801993E0..0x8019E670) in the 2026-09-30 recut.
 *
 * Seam.  The right edge 0x8019E670 is where the TU's own static-initializer ends: the `.ctors` word
 * points at `fn_8019E604`, which seeds the two vectors at `lbl_806A7A70`, and MWCC emits a TU's
 * `__sinit` last.  The next function, `fn_8019E670`, is the first reader of the second 0.0 pool entry
 * (`lbl_80798538`; the first, `lbl_80798238`, is read here), and one TU pools a value once.  The data
 * proves the 0x801993E0 edge false: the jump tables and tables at 0x805AE118..0x805AE6FC are read only by
 * 0x801994F4..0x8019D8B8 and the table at 0x805AE720 by `fn_8019287C`, so one `.data` run spans both
 * old units.  The left edge 0x801926EC is NOT proven: the same two pieces of evidence put this TU's
 * start at 0x80192348 (the function after `fn_80192204`, the preceding TU's `__sinit`), i.e. the
 * tail of `enemy/fn_80191598.cpp` (0x80192348..0x801926EC, written over its own `EmActWork` model)
 * belongs here.
 *
 * Module `enemy`: every callee out of the range is an enemy-band function.  Language C++ (the range
 * reaches the mangled `em_frame_check`).
 *
 * Name.  No `__FILE__` string survives in the range and the runtime dump answers only `zz_`
 * placeholders (`dumpmap.py lookup 0x801926EC` -> `zz_01926ec_`), so the map's own stem keeps the
 * file name (brief section 2, class 4).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py
 * lookup on the range's inventory: every name is a bare `.text` entry in
 * config/RMHE08/symbols.txt and the runtime dump answers only `zz_XXXXXXXX_` placeholders)
 *
 * Structure.  `fn_80199B24` dispatches `self->action` (0x1E5) into this range's
 * six dispatchers `fn_80192F24`/`fn_80193394`/`fn_801938D8`/`fn_801953BC`/`fn_80196618`/`fn_801987E4`,
 * each of which switches on `self->state_sub` (0x1E6) and tail-calls one per-motion step.  Every
 * state machine reads `self->state` (0x005) and, at state 0, runs `em_move_mode_set` + a motion setter
 * (`em_mot_set_ck`/`fn_8012F5C4`/`em_mot_set`); at state 1 it waits on `em_mot_end_ck(self) == 1` and
 * runs a completion hook.
 *
 * Status of the original 0x801926EC..0x801993E0 range (official `report.json`; the absorbed range is below).
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
 * Absorbed range 0x801993E0..0x8019E670: 14 functions written, nine unwritten - `fn_801994F4` (1336 B),
 * `fn_80199BE0` (980 B), `fn_80199FB4` (544 B), `fn_8019A1D4` (12336 B), `fn_8019D204` (1716 B),
 * `fn_8019DBDC` (692 B), `fn_8019DE98` (556 B), `fn_8019E0C4` (724 B), `fn_8019E604` (108 B, the `__sinit`).
 * Residuals: `stage_map_kind_get`'s `clrlwi r0,r3,24` after the call
 * is folded away (`fn_8019D8B8` 90.52, `fn_8019D9BC` 82.62, `fn_8019DAC0` 96.00: the owner's `u32`
 * declaration would need a `u8` view, which re-measures every landed consumer); `fn_8019E398` 89.85 and
 * `fn_8019E49C` 92.11 are register allocation on the `||` blocks and the f30 save of the radius test.
 *
 * Residual shapes on the 12 near-misses (all are register/evaluation ordering, not comprehension):
 *   * fn_801933DC - the target keeps a `clrlwi r4,r4,16` before `em_mot_set` that MWCC folds away for
 *     the constant ternary; the motion value needs a u16 spelling the header's `s32` prototype cannot ask
 *     for.  4 B short.
 *   * fn_801934EC - `mr r3,self` is hoisted before the height ternary in the target, after it here; one row.
 *   * fn_80193964/fn_8019485C/fn_8019610C/fn_801961C8/fn_80195BCC/fn_80196768/fn_80196A50/fn_80197C84/
 *     fn_80198FE8 - single-row register colouring / `mr r3` placement around the `fn_8012F9*` wait.
 *   * fn_80194CC8 - the +0x020 timer loop's `lfs`/`fcmpo` ordering.
 */
#include "types.h"
#include "nw4r/math.h"

#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_801251D0.h" /* fn_801280F4, fn_80128030 */
#include "enemy/fn_8012BDF4.h" /* em_busy_set */
#include "enemy/fn_8012EC74.h" /* em_camera_req */
#include "enemy/fn_80138074.h"  /* em_res_user_data_set, em_res_user_data_ck, fn_8013A654, fn_8013918C */
#include "enemy/fn_80191598.h" /* fn_80192370, fn_80192618 */
#include "fn_8004CAD8.h"       /* calcDistanceSqXZ */
#include "unsplit/enemy.h"     /* the plain enemy-band callees (rule 2 band header) */

/* The `.sdata2` and `.data` pool labels this range loads.  They are unsplit (no registered unit
 * claims a range covering them) and `stylelint`'s rule-2 `module()` answers `None`, so they stay
 * declared here as the counted "address band interleaves modules" gap - the same shape the landed
 * `enemy/fn_8019ED34.cpp` uses for its own pool runs.  Never defined: the target addresses them. */
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
extern f32 lbl_807983B0;
extern f32 lbl_807983F0;
extern f32 lbl_807983F4;
extern f32 lbl_807983F8;
extern f32 lbl_80798510;
/* The `em_turn_seq_start`/`em_turn_seq_step` descriptor blocks this band arms (`.data` tables). */
extern u8 lbl_80570050[];
extern u8 lbl_80570090[];
extern u8 lbl_805700D0[];
extern u8 lbl_80570110[];

extern "C" {

/* 0x802B0668 - the map-id lookup: a byte table, `0xFF` meaning "no entry" (the argument comes back). */
u32 stage_map_kind_get(u32 kind);

/* This range's own functions, declared so the dispatchers can tail-call them (they are defined
 * below, in address order). */
void fn_801993E0(struct _ENEMY_WORK* self);
void fn_80199468(struct _ENEMY_WORK* self);
void fn_801994F4(struct _ENEMY_WORK* self);
void fn_80199A2C(struct _ENEMY_WORK* self);
void fn_80199ADC(struct _ENEMY_WORK* self);
void fn_8019E398(struct _ENEMY_WORK* self);
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
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80192C8C - the same shape with motion 20. */
void fn_80192C8C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80192D08 - the same shape with motion 29. */
void fn_80192D08(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 29, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
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
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 40, 20, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
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
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 54, 20, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
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
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        fn_80132198(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
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

/* 0x80192F78 - arm motion 6 for 10 frames; state 1 runs `em_state_set(self, 1, 5)`. */
void fn_80192F78(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 1, 5);
        }
        break;
    }
}

/* 0x80192FFC - arm motion 7 for 4 frames. */
void fn_80192FFC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193078 - arm motion 0x1A for 4 frames. */
void fn_80193078(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801930F4 - arm motion 0x1E for 4 frames; state 1 fires the joint effect when `em_frame_check`
 * reports the frame and then waits on `em_mot_end_ck`. */
void fn_801930F4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1E, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798264, lbl_80798238) == 1) {
            self->field_0x358 = 1;
            em_camera_req(self, -1, 7);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801931A4 - arm motion 0x72 for 10 frames. */
void fn_801931A4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x72, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193220 - arm motion 8 for 4 frames. */
void fn_80193220(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019329C - arm motion 20 for 4 frames. */
void fn_8019329C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193318 - arm motion 0x1D for 4 frames. */
void fn_80193318(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
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
        em_move_mode_set(self, 0);
        {
            u16 motion = (u8)a == 1 ? 9 : 2;
            em_mot_set(self, motion, 4, 0);
        }
        switch ((u8)b) {
        default:
            em_approach_start(self, lbl_80798268, 0);
            break;
        case 1:
            em_approach_start(self, lbl_80798238, 0);
            if (self->value_0x378 > lbl_8079826C) {
                self->value_0x378 = lbl_8079826C;
            }
            break;
        case 2:
            em_approach_start(self, lbl_80798270, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801934EC - arm motion 21 and pick the height by `a`. */
void fn_801934EC(struct _ENEMY_WORK* self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 21, 4, 0);
        {
            f32 height = (u8)a == 1 ? lbl_80798270 : lbl_80798268;
            em_approach_start(self, height, 0);
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801935A0 - arm the `lbl_80570050` descriptor and wait on `em_turn_seq_step`. */
void fn_801935A0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570050, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570050) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019362C - arm motion 0x1B for 4 frames. */
void fn_8019362C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
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
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x14, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80193F70 - arm mode 2 with (0x31/0x14/0/1) and the `em_state_set(self, 5, 0x1D)` completion. */
void fn_80193F70(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 5, 0x1D);
        }
        break;
    }
}

/* 0x80193FF8 - arm mode 2 with (0x3A/0x0A/0/1) and the `fn_80128030` completion. */
void fn_80193FF8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
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
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x68, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
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
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x2E, 6, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
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
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x14, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x801966DC - arm motion 0x54 for 10 frames with the `em_hit_window_set_default(self, 0, 1)` pair. */
void fn_801966DC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x54, 0xA, 0);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196A50 - arm motion 0x57 for 10 frames and the two joint slots whose ids depend on `a`. */
void fn_80196A50(struct _ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x57, 0xA, 0);
        {
            u32 first, second;
            if (a == 1) {
                first = 0x16;
                second = 0x17;
            } else {
                first = 6;
                second = 7;
            }
            em_hit_window_set(self, 0, first, 8);
            em_hit_window_set(self, 1, second, 0x10);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80194CC8 - the release step: clears the bit pair, arms motion 0x80 and runs the +0x020 timer
 * down to zero, then completes with `em_state_set(self, 5, 0x20)`. */
void fn_80194CC8(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x80, 4, 0);
        self->timer_0x020 = 0xBE;
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            em_state_set(self, 5, 0x20);
        }
        break;
    }
}

/* 0x80195BCC - the flag-arming release step: arms motion 0xCD, refreshes the scale, then either
 * the `em_state_set(self, 6, 1)` completion (`a == 1`) or `fn_801280F4`. */
void fn_80195BCC(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCD, 0, 0);
        fn_80132198(self);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (a == 1) {
                em_state_set(self, 6, 1);
            } else {
                fn_801280F4(self);
            }
        }
        break;
    }
}

/* 0x80194318 - arm motion 0x3D with the common `em_action_finish` completion. */
void fn_80194318(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x3D, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193964 - the requested-motion step: `a == 1` primes the two-state release first; state 1
 * waits on the `lbl_80570090` descriptor and runs `fn_80128030` when it finishes. */
void fn_80193964(struct _ENEMY_WORK* self, u8 a) {
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570090, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570090) == 1) {
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
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_805700D0, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_805700D0) == 1) {
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
 * `em_state_set(self, 0xD, 1)` completion. */
void fn_80198F28(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570090, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570090) == 1) {
            em_state_set(self, 0xD, 1);
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
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570110, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570110) == 1) {
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
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x1F, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798264, lbl_80798238) == 1) {
            self->field_0x358 = 1;
            em_camera_req(self, -1, 7);
        }
        if (em_mot_end_ck(self) == 1) {
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
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 8, 8);
        em_hit_window_set(self, 1, 9, 0x10);
        fn_80130CDC(self, -0xC);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80798338, lbl_80798238) == 0) {
            fn_80136D4C(self, lbl_80798260);
        }
        if (em_mot_end_ck(self) == 1) {
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
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD0, 0, 0);
        fn_801303EC(self, lbl_80798238);
        fn_80136D14(self);
        self->field_0x359 = 1;
        break;
    case 1:
        em_busy_set(self);
        if (em_frame_check(self, 2, lbl_80798290, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_frame_check(self, 2, lbl_8079827C, lbl_80798238) == 1) {
            fn_80136D14(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196768 - the three-state step: arm motion 0x59, advance to 0x5A when the frame gate fires,
 * then complete with `em_action_finish`. */
void fn_80196768(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 0xA, 0);
        em_hit_window_set(self, 0, 3, 3);
        em_approach_start(self, lbl_80798328, 0);
        fn_80130CDC(self, -0xC);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 0x5A, 2, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_8079832C, lbl_80798238) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
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
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0xA, 8, 0, 1);
        em_approach_start(self, lbl_80798270, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80798370, lbl_80798238) == 1 &&
            (em_approach_step(self, 0, 0x40) == 1 || em_mot_end_ck(self) == 1)) {
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
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0xD6, 0x14, 0, 1);
        em_hit_window_set(self, 0, 2, 3);
        fn_801303EC(self, lbl_80798238);
        fn_80136D14(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798398, lbl_80798238) == 1) {
            fn_80136D14(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0xD3, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_8079839C, lbl_80798238) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80197BC0 - the two-stage rotation step: state 0 advances the +0x1C0 angle by 0x8000, state 1
 * arms motion 0x7F, state 2 completes with `em_action_finish`. */
void fn_80197BC0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7E, 0, 0);
        self->field_0x1C0 += 0x8000;
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x7F, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
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
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019610C - the same arming as `fn_801961C8` with the `em_turn_to_target(self, 0x300)` completion. */
void fn_8019610C(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        fn_80132198(self);
        break;
    case 1:
        if (em_turn_to_target(self, 0x300) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019485C - arm motion 0xCA for 10 frames, advance to 0xDE on completion, then `fn_80128030`. */
void fn_8019485C(struct _ENEMY_WORK* self, u8 a) {
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCA, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0xDE, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80195098 - arm motion 0x7F, advance to 0x82 on completion, then `em_action_finish`. */
void fn_80195098(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7F, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x82, 0, 0xBC);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80195ED4 - arm motion 0xDA with the joint slot 0xF; the frame effects re-arm the flag and
 * fire the joint effect, and `em_action_finish` completes the step. */
void fn_80195ED4(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDA, 0, 0);
        em_hit_window_set_default(self, 0, 0xF);
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
            em_turn_to_target(self, 0x400);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196004 - arm motion 0xDB with the joint slot 0x10 and the two frame-effect hooks. */
void fn_80196004(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDB, 0, 0);
        fn_801303EC(self, lbl_80798238);
        em_hit_window_set_default(self, 0, 0x10);
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
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80198FE8 - arm mode 2 with (0x37/0x14/0/1) and the `lbl_807983A0` blend; state 1 waits on the
 * descriptor and advances to (0x2F/0x28/0/1), completing with `em_state_set(self, 0xD, 2)`. */
void fn_80198FE8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);
        em_approach_start(self, lbl_807983A0, 0x12);
        break;
    case 1:
        fn_80136D4C(self, lbl_807982E0);
        if (em_approach_step(self, 0, 0x80) == 1 && fn_8012F948(self) == 0) {
            self->state++;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 2);
        }
        break;
    }
}

/* 0x801993E0 - the state-advance entry of the enemy's "em" action: state 0 arms the motion, state 1
 * waits for `em_mot_end_ck` to report the current action finished and then runs the band's
 * `em_state_set(self, 13, 5)` completion. */
void fn_801993E0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 49, 20, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 13, 5);
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
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 46, 6, 0, 1);
        fn_80130CDC(self, 1000);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80199A2C - the motion step of the action `fn_80199ADC` dispatches case 7 to: state 0 arms mode
 * 31 with the band's `em_demo_pos_set`/`em_demo_rot_set` pair and zeroes the stored height, state 1 waits for
 * `em_mot_end_ck` and then runs `fn_80128030`. */
void fn_80199A2C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 31, 0, 0);
        em_demo_pos_set(self, lbl_807983F0, lbl_807983F4, lbl_807983F8);
        em_demo_rot_set(self, lbl_80798238, lbl_807983B0, lbl_80798238);
        fn_801303EC(self, lbl_80798238);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
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
 * every action shares: the +0x1E2 gate that runs the pair `em_busy_set`/`em_busy_timer_reset`, then this
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
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    fn_8019E398(self);
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

/* 0x8019D9BC - the "load the action's joint position" init: arm mode 4, publish the two record
 * bytes and ask `fn_80126278` for the area's joint, then run the band's common tail. */
void fn_8019D9BC(struct _ENEMY_WORK* self, u8* out_a, u8* out_b) {
    em_move_mode_set(self, 4);
    *out_a = 12;
    *out_b = 0;
    switch (stage_map_kind_get(self->field_0x1E0)) {
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

/* 0x8019DAC0 - the area dispatch of the "already in mode" state: the map lookup picks the area
 * group, this area decides whether the state is armed. */
void fn_8019DAC0(struct _ENEMY_WORK* self) {
    u32 state = 0;
    if (stage_map_kind_get(self->field_0x1E0) == 3) {
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
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
    } else if (state == 2) {
        em_move_mode_set(self, 2);
        em_mot_set(self, 40, 0, 0);
    }
}

/* 0x8019DB9C - the "action 2 still running" gate: true only in mode 4 of the enemy-control state
 * machine while `em_alt_mode_ck` reports not-yet-armed. */
u32 fn_8019DB9C(struct _ENEMY_WORK* self) {
    if (self->field_0x1E2 == 4 && em_alt_mode_ck(self) == 0) {
        return 1;
    }
    return 0;
}

/* 0x8019DE90 - the one-line tail call the band's teardown uses: `fn_8013A654(self, 1)`. */
void fn_8019DE90(struct _ENEMY_WORK* self) {
    fn_8013A654(self, 1);
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

} /* extern "C" */

/* This unit's own `.bss` (`splits.txt` `.bss 0x806A7A70..0x806A7A88`): the two-vector record its static
 * constructor `fn_8019E604` builds (`.data` tables point at it).  The name is a GUESS: a pair of model-space
 * points. */
VEC3 vec_pair_801926EC_0[2];  /* +0x806A7A70 */
