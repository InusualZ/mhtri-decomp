/* enemy/fn_80177608.cpp - two enemy action handlers, 0x80177608..0x80177890 (2 functions, 648 bytes).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap: both
 * addresses resolve to bare `.text` entries `fn_80177608`/`fn_80177774` in config/RMHE08/symbols.txt,
 * and `dumpmap lookup` gives only a `zz_XXXXXXXX_` dump name, which is not evidence).
 *
 * What it is: a two-function slice of one enemy action - `fn_80177608` seeds a VEC3 from the engine's
 * vector helper, walks `state_0x05` through two steps and drives the effect/frame helpers
 * (`em_frame_check`, the `setVector3`/`fn_80304508` effect spawns); `fn_80177774` is the four-step
 * sibling that opens the action, waits on `em_mot_end_ck`, counts `field_0x20` down and closes it.
 *
 * Object: `_ENEMY_WORK` (name evidence: the mangled callee
 * `em_frame_check__FP11_ENEMY_WORKUsff` carries the 11-character type name).  Field offsets and widths
 * are read from the target's load/store instructions; only the two fields this unit touches are named
 * (`state_0x05`, `field_0x20`), the rest come from the shared record in `include/enemy.h`.
 *
 * Language: C++ (the region defines no symbol but its undefined set is mangled:
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`).
 *
 * Registration: moved to its final home `enemy/fn_80177608.cpp` from the discovery proposal
 * `proposal/80177608_fn_80177608.cpp`; the module is the band's (`enemy` - the bracketing registered
 * units are `enemy/fn_8014A1BC.c` below and `lobby/lobby_scene.c` above, and the region's callees are
 * the `em_*` enemy helpers), the name is the map's own `fn_XXXXXXXX` stem (Naming note above).
 *
 * Data runs in this range are NOT claimed: a stub object emits no `.sdata2`, and a range our object
 * does not emit must not be claimed (docs/plan.md 8.4).  The `extab`/`extabindex` fragments travel
 * with the code unit and ARE claimed in `splits.txt`.
 *
 * Declarations: `em_move_mode_set`, `em_mot_set`, `em_mot_end_ck` and `VEC3_ctor` come from the shared
 * headers (`include/unsplit/enemy.h`, `include/ef.h`); their signatures are the shared ones.  The
 * symbols whose owning unit is not registered and whose band has no sound header
 * (`fn_80304508`, `fn_80056A54`, `fn_8012933C`, `em_action_finish`) are declared here, as the landed
 * `enemy/fn_8014A1BC.c` does.  `fn_8013221C`, `fn_80132224`, `fn_80132264` belong to the `enemy` band
 * too, but `include/unsplit/enemy.h` does not carry them yet - see the outbox `shared-file` request.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy.h"
#include "unsplit/enemy.h"

/* Flags and evidence: the unit's command line is the `enemy` lib's.  A scoped `#pragma peephole off`
 * is required because this front-end's `-O3` peephole fuses the mask and its zero-compare into the
 * record form where retail keeps them apart: `(field_0x20 & 7) == 0` becomes `clrlwi.` (target keeps
 * `clrlwi` + `cmpwi`), and the count-down becomes `subic.` (target keeps `subi` + `cmpwi`).  Both
 * regressed the score until the pragma was added; no lib flag changes (docs/plan.md 8.2). */
#pragma peephole off

/* `em_frame_check` is a C++ mangling (`em_frame_check__FP11_ENEMY_WORKUsff`): declaring its real name
 * at global scope makes the front-end emit the map symbol, which is what docs/plan.md 6.5 rule 9 asks
 * for (never write the mangled spelling as the identifier). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

extern "C" {
/* Not registered yet, and the two bracketing registered units of their address bands name different
 * modules, so there is no sound `include/unsplit/<module>.h` to move them to (rule 2's named gap). */
void fn_80304508(_ENEMY_WORK* self, u32 a, u32 b, VEC3* v, f32 s);
void fn_80056A54(_ENEMY_WORK* self, u32 a, u32 b);
void fn_8012933C(_ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_action_finish(_ENEMY_WORK* self);

/* `enemy`-band, not yet in include/unsplit/enemy.h (see the outbox `shared-file` request). */
void fn_8013221C(_ENEMY_WORK* self, f32 a, u32 b, u32 c);
void fn_80132224(_ENEMY_WORK* self);
void fn_80132264(_ENEMY_WORK* self);
}

/* `.sdata2` constants this unit reads (shared pool; unsplit band, no sound module header). */
extern f32 lbl_80797B18;
extern f32 lbl_80797B28;
extern f32 lbl_80797B2C;
extern f32 lbl_80797B30;
extern f32 lbl_80797B34;
extern f32 lbl_80797B38;
extern f32 lbl_80797B3C;
extern f32 lbl_80797B40;
extern f32 lbl_80797B44;

/* The map names these two with a bare `fn_XXXXXXXX` stem (no mangling), so they take C linkage to
 * emit that exact symbol; rule 9 keeps an `fn_` stem legal. */
extern "C" void fn_80177608(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state_0x05) {
    case 0:
        self->state_0x05++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 0xA, 0);
        self->field_0x20 = 0;
        break;
    case 1:
        setVector3(&v, lbl_80797B18, lbl_80797B28, lbl_80797B2C);
        if (em_frame_check(self, 0, lbl_80797B30, lbl_80797B18) == 1U) {
            fn_80304508(self, 0, 0x18, &v, lbl_80797B34);
        }
        if (em_frame_check(self, 0, lbl_80797B38, lbl_80797B18) == 1U) {
            fn_80056A54(self, 0x1A, 0xA);
            fn_8012933C(self, 0, 0x1C, 5);
        }
        if (em_frame_check(self, 3, lbl_80797B3C, lbl_80797B40) == 1U) {
            if ((self->field_0x20 & 7) == 0) {
                fn_80304508(self, 1, 0x18, &v, lbl_80797B34);
            }
            self->field_0x20 = self->field_0x20 + 1;
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80177774(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05++;
            em_mot_set(self, 4, 0, 0);
            self->field_0x20 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2:
        fn_8013221C(self, lbl_80797B44, 1, 0xF);
        self->field_0x20 = self->field_0x20 - 1;
        if ((s32)self->field_0x20 <= 0) {
            self->state_0x05++;
            em_mot_set(self, 5, 4, 0);
            fn_80132264(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}
