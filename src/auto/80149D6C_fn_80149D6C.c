/* auto/80149D6C_fn_80149D6C.c - one enemy action step, `.text` 0x80149D6C..0x8014A1BC.
 *
 * What it is.  The unit is one `_ENEMY_WORK` action.  A two-state machine on `state_0x05` (state 0
 * initialises through `fn_80134F70`/`fn_80134004`/`fn_80130248`/`fn_80135584`, state 1 runs the action)
 * plus a `phase_0x06`/`step_0x07` pair picks the body; the shared tail first asks `fn_80134114`
 * whether the enemy is in the released group and then either sets an act id
 * (`fn_80128A14`/`fn_80128A70`) or hands the parameter table `lbl_8056F9E0` to `fn_80135000`.  The
 * `u8` argument is the action index the dispatcher in this region tail-calls the function with (0/1/2).
 *
 * Flags.  The unit needs a scoped `#pragma peephole off` (playbook 39): retail keeps the `clrlwi`
 * zero-extensions of the argument at every use, and the `-O3` peephole folds them away.  With the
 * peephole off the one comparison that must stay *signed* is `if ((arg & 0xFF) == 1)` (`cmpwi`);
 * `(u8)arg == 1` folds to `cmplwi` and costs a point.
 *
 * Source shapes worth keeping (each measured against the target):
 *   * `state_0x05`/`phase_0x06` are incremented, not assigned a literal, so retail reuses the value
 *     the `switch` already loaded (`addi r0,r3,1`).
 *   * the shared tail is written *inside* `case 1`, so the outer switch's default returns (a branch
 *     to the epilogue) instead of falling into the tail (`b` to the tail body).
 *   * the two nested switches write `default:` first (playbook 37), and the released-tail switch
 *     lists `case 2: case 3: case 4:` before `case 0:` - that is the order whose fall-through body
 *     is the one retail lays out last.
 *
 * Types.  `_ENEMY_WORK` is the enemy work record; its size 0xB18 is the kind-3 stride in
 * `create_move_work__Fl` (`mulli r30,r3,2840` at 0x800CFA0C) and `fn_80131034` walks the same array
 * with it.  `field_0x188`/`pos_0x1BC`/`area_no` follow the sibling units that already reconstruct
 * this record; the remaining fields are named by offset where the byte's role is not settled.
 *
 * Data.  The unit owns no pool: `lbl_8056F9E0` (`.rodata`, the action parameter table),
 * `lbl_80796E1C` (0.0f) and `lbl_80796EE8` (16000000.0f) are shared and stay `extern`-declared
 * (playbook 29).
 *
 * Language.  The unit's own symbol is plain and nothing names its original source file, so the file
 * stays C and the one mangled callee is declared by its map spelling.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/80149D6C_fn_80149D6C.c`.
 */

#include "types.h"

/* ---- the enemy work record ---- */

/* The engine's three-float vector; the distance check and the position update both take one. size: 0x0C */
typedef struct Vec3 {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
} Vec3; /* size: 0x0C */

/* The enemy work record.  Only the bytes this unit reads are named, and their names agree with the
 * sibling units that already reconstruct the record (`80101FA4_fn_80101FA4.cpp`,
 * `80104BD0_fn_80104BD0.c`).
 * size: 0xB18 (the per-kind allocation stride `create_move_work__Fl` uses for kind 3). */
typedef struct _ENEMY_WORK {
    /* +0x000 */ u8 unused_0x000[0x002 - 0x000];
    /* +0x002 */ u8 slot_0x02; /* the record's own slot in the per-kind move-work array */
    /* +0x003 */ u8 unused_0x003[0x005 - 0x003];
    /* +0x005 */ u8 state_0x05; /* the action's outer state, 0 or 1 */
    /* +0x006 */ u8 phase_0x06; /* the state-1 phase, 0 or 1 */
    /* +0x007 */ u8 step_0x07;  /* the state-1 phase-1 step, 1 or 2 */
    /* +0x008 */ u8 unused_0x008[0x188 - 0x008];
    /* +0x188 */ Vec3 field_0x188; /* one end of the distance the step-2 body measures */
    /* +0x194 */ u8 unused_0x194[0x1BC - 0x194];
    /* +0x1BC */ Vec3 pos_0x1BC; /* the position `fn_80135584` advances */
    /* +0x1C8 */ u8 unused_0x1C8[0x1E1 - 0x1C8];
    /* +0x1E1 */ u8 area_no; /* the area the enemy is in */
    /* +0x1E2 */ u8 unused_0x1E2[0x1E7 - 0x1E2];
    /* +0x1E7 */ u8 timer_0x1E7; /* the countdown `fn_801277F4` decrements */
    /* +0x1E8 */ u8 unused_0x1E8[0x1F9 - 0x1E8];
    /* +0x1F9 */ u8 field_0x1F9; /* gates the state-1 phase-0 body */
    /* +0x1FA */ u8 unused_0x1FA[0x36C - 0x1FA];
    /* +0x36C */ Vec3 field_0x36C; /* the other end of the distance the step-2 body measures */
    /* +0x378 */ u8 unused_0x378[0x382 - 0x378];
    /* +0x382 */ u8 field_0x382; /* 255 means "no area", gates an act id */
    /* +0x383 */ u8 unused_0x383[0x43D - 0x383];
    /* +0x43D */ u8 field_0x43D; /* 1 selects the second step value */
    /* +0x43E */ u8 unused_0x43E[0x8A2 - 0x43E];
    /* +0x8A2 */ u16 field_0x8A2; /* compared against 500 */
    /* +0x8A4 */ u8 unused_0x8A4[0x9F8 - 0x8A4];
    /* +0x9F8 */ u8 field_0x9F8; /* the area compared with `area_no`, 255 means "none" */
    /* +0x9F9 */ u8 unused_0x9F9[0xB18 - 0x9F9];
} _ENEMY_WORK; /* size: 0xB18 */

/* ---- callees ---- */

extern void fn_8012CF20(_ENEMY_WORK* self);
extern void fn_80134F70(_ENEMY_WORK* self, u8* params);
extern void fn_80134004(_ENEMY_WORK* self, f32 scale, u16 id);
extern void fn_80130248(_ENEMY_WORK* self);
extern void fn_80135584(_ENEMY_WORK* self, Vec3* pos);
extern u32 fn_8012EC3C(_ENEMY_WORK* self);
extern _ENEMY_WORK* fn_80131034(_ENEMY_WORK* self, u8 kind, u8 distance_check);
extern u32 fn_8012E5A8(_ENEMY_WORK* self);
extern u32 fn_80131BD4(_ENEMY_WORK* self);
extern void fn_8012B380(_ENEMY_WORK* self, u8 a, u8 b, u8 c);
extern void fn_80128A14(_ENEMY_WORK* self, u8 a, u8 b);
extern void fn_8013AAC4(_ENEMY_WORK* self);
extern u32 fn_80134114(_ENEMY_WORK* self, s32 a, s32 b);
extern void fn_801481FC(_ENEMY_WORK* self);
extern void fn_801277F4(_ENEMY_WORK* self, s32 a);
extern void fn_80128A70(_ENEMY_WORK* self, u8 a, u8 b);
extern void fn_80135000(_ENEMY_WORK* self, u8 a, u8* params);
extern f32 calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3(Vec3* a, Vec3* b);

/* Shared pool literals: declared, never defined here (the pool belongs to the data pass). */
extern u8 lbl_8056F9E0[];
extern f32 lbl_80796E1C; /* 0.0f */
extern f32 lbl_80796EE8; /* 16000000.0f */

#pragma peephole off

/* Runs one enemy action step: advances the two-state machine and dispatches the per-action body. */
void fn_80149D6C(_ENEMY_WORK* self, u8 arg)
{
    fn_8012CF20(self);

    switch (self->state_0x05) {
    case 0:
        self->state_0x05++;
        self->phase_0x06 = 0;
        self->step_0x07 = 0;
        fn_80134F70(self, lbl_8056F9E0);
        fn_80134004(self, lbl_80796E1C, 25);
        fn_80130248(self);
        fn_80135584(self, &self->pos_0x1BC);
        return;

    case 1:
        if ((arg & 0xFF) == 1) {
            switch (self->phase_0x06) {
            case 0: {
                _ENEMY_WORK* v;

                if (self->field_0x1F9 != 0) {
                    break;
                }
                self->phase_0x06++;
                if (fn_8012EC3C(self) == 1 || self->field_0x8A2 < 500) {
                    v = fn_80131034(self, 27, 0);
                    if (v != 0 && fn_8012E5A8(v) == 1) {
                        v = 0;
                    }
                } else {
                    v = 0;
                }
                if (v != 0) {
                    self->step_0x07 = 1;
                    break;
                }
                if (self->field_0x43D == 1 && fn_80131BD4(self) == 1) {
                    self->step_0x07 = 2;
                }
                break;
            }

            case 1:
                switch (self->step_0x07) {
                case 1: {
                    _ENEMY_WORK* v = fn_80131034(self, 27, 1);

                    if (v != 0 && fn_8012E5A8(v) == 1) {
                        v = 0;
                    }
                    if (v != 0) {
                        fn_8012B380(self, 3, 2, v->slot_0x02);
                        fn_80128A14(self, 13, 0);
                        fn_8013AAC4(self);
                        return;
                    }
                    break;
                }

                case 2:
                    if (calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3(
                            &self->field_0x188, &self->field_0x36C) <= lbl_80796EE8) {
                        fn_80128A14(self, 3, 8);
                        fn_8013AAC4(self);
                        return;
                    }
                    break;
                }
                break;
            }
        }

        if (fn_80134114(self, 0, 0) == 1) {
            switch (arg) {
            case 0:
                fn_801481FC(self);
                break;

            case 1:
                if (self->area_no == self->field_0x9F8 || self->field_0x9F8 == 255) {
                    fn_80128A14(self, 3, 10);
                } else {
                    fn_80128A14(self, 3, 9);
                }
                break;

            case 2:
            case 3:
            case 4:
                if (self->timer_0x1E7 == 0) {
                    switch (arg) {
                    default:
                        fn_801481FC(self);
                        break;

                    case 3:
                        fn_8012B380(self, 0, 0, 0);
                        if (self->field_0x382 == 255) {
                            fn_8012B380(self, 5, 9, 0);
                        }
                        fn_80128A14(self, 7, 28);
                        break;

                    case 4:
                        fn_8012B380(self, 0, 0, 0);
                        if (self->field_0x382 == 255) {
                            fn_8012B380(self, 5, 9, 0);
                        }
                        if (fn_8012EC3C(self) == 1) {
                            fn_80128A14(self, 7, 57);
                        } else {
                            fn_80128A14(self, 7, 29);
                        }
                        break;
                    }
                } else {
                    fn_801277F4(self, 0);
                    switch (arg) {
                    default:
                        fn_80128A70(self, 3, 14);
                        break;
                    case 3:
                        fn_80128A70(self, 3, 22);
                        break;
                    case 4:
                        fn_80128A70(self, 3, 23);
                        break;
                    }
                }
                break;
            }
        } else {
            switch (arg) {
            case 2:
            case 3:
            case 4:
                fn_80135000(self, 2, lbl_8056F9E0);
                break;

            case 0:
                fn_80135000(self, 1, lbl_8056F9E0);
                break;

            default:
                fn_80135000(self, 0, lbl_8056F9E0);
                break;
            }
            fn_80130248(self);
            fn_80135584(self, &self->pos_0x1BC);
        }
        break;
    }
}

#pragma peephole on
