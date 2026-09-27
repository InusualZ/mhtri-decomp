/* Monster Hunter Tri (RMHE08) - the enemy program band 0x8037EA64-0x8037F940 (`.text`, 12 functions,
 * 3804 B), reconstructed from the split target object.
 *
 * WHAT IT IS.  The enemy work record's action-program tail: `em_act_run` (0x8037F524) is the
 * per-frame action runner - it clears the +0x358 run flag, dispatches on the action id
 * `self->action` (+0x1E5) through the 14-entry table its own `.data` carries, ticks the +0x35A
 * counter while +0x358 is 1 and calls the +0x1E2 hook pair; action 9 of that table is
 * `em_act_prog_dispatch` (0x8037F4D8), the *program* dispatcher, which tail-calls through the
 * 9-entry jump table on `self->state_sub` (+0x1E6).  Its cases 1..8 are this band's
 * `em_act_prog_1`..`em_act_prog_8` (case 0, `fn_8037E0E8`, belongs to the em019 band below); each is
 * a phase machine on `self->state` (+0x005) whose phase 0 arms a motion and whose phase 1 waits on it
 * (`fn_8012F93C`, `em_frame_check`).  `em_parts_damage_ck` (0x8037F624) steps the seven part damage
 * meters and `em_act_effect_ck` (0x8037F8A8) drops the record's own ground-stamp vector.
 *
 * MODULE AND NAME (brief section 2, evidence order).  Class 1, a `__FILE__` string: none - the DOL
 * carries one copy of each band file name it has and none is reachable from this range, whose
 * `.data`/`.rodata`/`.sdata2` runs hold no printable byte.  Class 2, the runtime dump: `dumpmap.py
 * lookup` answers `zz_XXXXXXXX_` for all 12 addresses.  Class 3 decides the module: every body takes
 * the shared `_ENEMY_WORK` in r3 and drives it through the enemy API
 * (`em_frame_check__FP11_ENEMY_WORKUsff` x8, `em_parts_damage_level_get__FP11_ENEMY_WORKUc`,
 * `get_em_chg_scale__FP11_ENEMY_WORK`), its link neighbours are `enemy/*`, and the sibling `em_*`
 * bands are the naming scheme.  The file name `em019_prog` follows the *program table* evidence: the
 * `.data` table `em019_prog_tbl` (0x805EE518, `scope:global`) is the em019 program's entry list and
 * its +0x0C entry is this band's own `em_act_run` - the band is the em019 file's program half.  Every
 * symbol here is a **derived name (GUESS)** from its own body; each function's comment carries the
 * datum behind it.
 *
 * SEAM (re-drawn, not the brief's `--max-bytes` cut).  The brief's range was 0x8037EA64..0x80382310,
 * one `attribute.py` byte-budget run over TWO translation units, and it is registered here as the two
 * the evidence names.  The cut is 0x8037F940, and three instruments agree: `tools/splits/tudiscover.py
 * at 0x8037E0E8` reports it as the strong seam (`.data` run jump `jumptable_805EF4F4` ->
 * `jumptable_805EF52C`, each referenced by exactly one side, plus the `.sdata2` run jump
 * `lbl_8079BE88` -> `lbl_8079BE8C`); the already-registered `enemy/em019_ai.cpp` (whose file this band
 * is the tail of) records the same extent, 0x80378F9C..0x8037F940; and the bracketing extab runs tile
 * - this unit takes 0x80017DBC..0x80017E14 (11 records - every function here but
 * `em_act_prog_dispatch` carries one) and the other half 0x80017E1C..0x80017E2C, which ends exactly where
 * `enemy/fn_80382310.cpp` starts its own run.  The left edge is the same `--max-bytes` artifact seen
 * from this side: `enemy/em019_ai.cpp` ends at 0x8037EA64, and this band's first function is the next
 * one in address order, so this registration is a *fragment* of that file - the `range` config_request
 * in the outbox asks for the two to be folded once the em019 file's bodies are written.
 *
 * rule 7 deferred: references only to other units' unrenamed `fn_XXXXXXXX` symbols - this file names
 * every symbol it *defines* (the 12 definitions below); what remains are callees whose owning band is
 * still a `fn_` row in the map (`fn_80130478`, `fn_8012F5B8`, `fn_80127F48` and ~40 more, plus the
 * em019 band's `fn_8037E0E8`..`fn_803797F4`), which the pass that writes those bands owns
 * (`grep -n "fn_" src/enemy/em019_prog.cpp`: every `fn_` left is a call or an `extern` declaration).
 * Every definition is `extern "C"` so objdiff pairs it by the map's name (playbook 42).  The
 * genuinely mangled callees (`em_frame_check`, `em_parts_damage_level_get`, `get_em_chg_scale`,
 * `calcVecAng2`, `rotVecY`) are called through their C++ declarations at global scope (rule 9).
 *
 * SECTIONS.  `.text` 0x8037EA64..0x8037F940, `extab` 0x80017DBC..0x80017E14 (11 x 8 B), `extabindex`
 * 0x80037B00..0x80037B84 (11 x 12 B).  The `.data` run 0x805EF4D0..0x805EF52C (two jump tables) and
 * the `.rodata` table 0x80570AE0 are **not claimed**: a claim without the emitting source is a
 * `target-extra` row, so this object declares them and emits none of them (playbook 23).
 */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/EM_PART_BLOCK.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "ef/fn_80105314.h"
#include "fn_8004CAD8.h"
#include "stage/fn_802B2AA0.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"

/* Retail keeps this band's unfused narrow forms: the seven `em_parts_damage_ck` slot `clrlwi`
 * pairs and `em_act_prog_8`'s countdown compare are the peephole's input (`em_act_prog_7` 100 -> 96,
 * `em_parts_damage_ck` 100 -> 95.43, `em_act_effect_ck` 100 -> 97.24 and the unit 97.62 -> 96.41 with
 * the pass on).  The `stage/fn_802B2AA0.h` include no longer carries the pragma this unit used to
 * inherit, so the unit states it here (playbook 39); `tools/units/stylelint.py` rule 10 keeps a
 * codegen pragma out of the shared header. */
#pragma peephole off

/* This band's view of the shell object `shell_set_func_ptr` points at: +0x2C is the job-injection
 * slot (`lwz r12, 44(r8)` + `mtctr` + `bctrl`, with the work record, two ids, a VEC3, a scale and the
 * +0xAEA flag word as its arguments).  `include/stage/fn_802B2AA0.h` owns the pointer and the
 * +0x80/+0x84 view of the other band.
 * size: 0x30 (approximate: only +0x00..+0x2C is read). */
typedef struct EmShellFuncs {
    /* +0x000 */ u8 pad_0x00[0x2C];
    /* +0x02C */ void (*method_0x2C)(struct _ENEMY_WORK* self, s32 a, s32 b, nw4r::math::VEC3* pos,
                                     f32 scale, u16 flags);
} EmShellFuncs;

/* The `.rodata` motion table `fn_80134964`/`fn_80134B0C` walk (0x80570AE0..0x80570B20, 64 B). */
extern "C" u8 lbl_80570AE0[];

/* The band's pooled floats (`.sdata2` 0x8079BC88..0x8079BE88), declared never defined: none is this
 * object's private pool entry (they are shared with the neighbouring bands' pools), so none is
 * claimable (playbook 58) and a definition would emit a second copy the linker does not merge. */
extern "C" f32 lbl_8079BC88;   /* 0.0 */
extern "C" f32 lbl_8079BC94;   /* 1.5 */
extern "C" f32 lbl_8079BCA0;   /* 200.0 */
extern "C" f32 lbl_8079BCA8;   /* 1.0 */
extern "C" f32 lbl_8079BCB4;   /* 220.0 */
extern "C" f32 lbl_8079BCC4;   /* 20.0 */
extern "C" f32 lbl_8079BCD4;   /* -900.0 */
extern "C" f32 lbl_8079BCD8;   /* -14.0 */
extern "C" f32 lbl_8079BCDC;   /* 10.0 */
extern "C" f32 lbl_8079BCE0;   /* 50.0 */
extern "C" f32 lbl_8079BCE4;   /* 70.0 */
extern "C" f32 lbl_8079BCE8;   /* 5.0 */
extern "C" f32 lbl_8079BCEC;   /* 196.0 */
extern "C" f32 lbl_8079BCF0;   /* 44.0 */
extern "C" f32 lbl_8079BCF4;   /* 56.0 */
extern "C" f32 lbl_8079BCF8;   /* 62.0 */
extern "C" f32 lbl_8079BD04;   /* 90.0 */
extern "C" f32 lbl_8079BD08;   /* 110.0 */
extern "C" f32 lbl_8079BD24;   /* 100.0 */
extern "C" f32 lbl_8079BE88;   /* -20.0 */

/* `include/stage/fn_802B2AA0.h` (this unit's `shell_set_func_ptr` owner) declares `fn_80041E8C` with
 * a `void` result, while the target's call site consumes the returned pointer; the band reaches it
 * through the signature its own body has.  `em035_prog.cpp` records the same gap, and the outbox
 * carries the shared-file request. */
typedef nw4r::math::VEC3* (*Fn80041E8C)(nw4r::math::VEC3* out, f32 x, f32 y, f32 z);

/* The out-of-range callees whose owning band is still a `fn_` row in the map - each spelling is the
 * callee's own body (the same gap the sibling bands `enemy/em_act_step.cpp` and
 * `enemy/em_action.cpp` record).  The `fn_8037...`/`fn_80378...` group is the em019 band's own
 * functions, which `enemy/em019_ai.cpp` (the rest of this file) owns. */
extern "C" {
u8 fn_80127CC4(struct _ENEMY_WORK* self, u32 a);
void fn_80127D20(struct _ENEMY_WORK* self, u32 a, u32 b);
u8 fn_80378F9C(struct _ENEMY_WORK* self, u32 a);
u8 fn_8037900C(struct _ENEMY_WORK* self, u32 a);
void fn_803797F4(struct _ENEMY_WORK* self);
void fn_80379DE4(struct _ENEMY_WORK* self);
void fn_8037A470(struct _ENEMY_WORK* self);
void fn_8037A7F0(struct _ENEMY_WORK* self);
void fn_8037AF08(struct _ENEMY_WORK* self);
void fn_8037DAC8(struct _ENEMY_WORK* self);
void fn_8037DC04(struct _ENEMY_WORK* self);
void fn_8037DFF0(struct _ENEMY_WORK* self);
void fn_8037E0D4(struct _ENEMY_WORK* self);
void fn_8037E0E8(struct _ENEMY_WORK* self);
u32 fn_80382E48(struct _ENEMY_WORK* self, u32 a);
void fn_803B9BA0(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a);
}

/* The enemy work's action-program channel 1: phase 0 clears the +0x1E4 byte, the whole per-part
 * block (+0x328..+0x357, four slots) and the +0x358/+0x35A pair, then arms motion 20 with the two
 * effect-position resets; phase 1 ends the program through `fn_80127F48` once `fn_8012F93C` reports
 * the motion done. */
extern "C" void em_act_prog_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
        s32 i;

        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 20, 0, 0);
        fn_80146058(self, lbl_8079BC88, lbl_8079BC88, lbl_8079BC88);
        fn_8014610C(self, lbl_8079BC88, lbl_8079BC88, lbl_8079BC88);
        fn_8014619C(self);
        self->field_0x1E4 = 0;
        for (i = 0; i < 4; i++) {
            part->colour_id[i] = 0;
            part->timer[i] = 0;
            part->meter[i] = lbl_8079BC88;
            part->flag[i] = 0;
            part->r[i] = 0;
            part->g[i] = 0;
            part->b[i] = 0;
            part->a[i] = 0;
        }
        self->field_0x358 = 0;
        self->tev_0x35A = 0;
        break;
    }
    case 1:
        if (fn_8012F93C(self) == 1)
            fn_80127F48(self);
        break;
    }
}

/* The enemy work's action-program channel 2: phase 0 arms motion 201 sub-motion 4 and zeroes the
 * +0x1CC sequence; phase 1 runs the four `em_frame_check` windows that latch the record's part
 * helpers and then ends on the motion's own frame check, handing the +0x1E4 value to part 13/3. */
extern "C" void em_act_prog_2(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 201, 4, 0);
        fn_801303EC(self, lbl_8079BC88);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079BCEC, lbl_8079BC88) == 1)
            fn_80136D14(self);
        if (em_frame_check(self, 0, lbl_8079BCF0, lbl_8079BC88) == 1)
            fn_8012933C(self, 0, 13, 10);
        if (em_frame_check(self, 0, lbl_8079BCF4, lbl_8079BC88) == 1)
            fn_8012933C(self, 0, 24, 26);
        if (em_frame_check(self, 0, lbl_8079BCF8, lbl_8079BC88) == 1)
            fn_80129724(self, 0);
        if (fn_8012F93C(self) == 1) {
            fn_80130478(self, 4);
            fn_801303EC(self, fn_8013032C(self));
            fn_80128A14(self, 13, 3);
        }
        break;
    }
}

/* The enemy work's action-program channel 3: the `fn_80131E00` hook runs every frame; phase 0 arms
 * motion 4 and starts the `lbl_80570AE0` motion-table sequence with the +0x1CC sequence at 5.0;
 * phase 1 polls that sequence and lands the +0x1E4 value on part 13/4 when it reports done. */
extern "C" void em_act_prog_3(_ENEMY_WORK* self) {
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_80134964(self, lbl_80570AE0, 0, 0, 0);
        fn_8012F8C8(self, lbl_8079BCE8);
        fn_801303EC(self, fn_8013032C(self));
        fn_80378F9C(self, 255);
        break;
    case 1:
        if (fn_80134B0C(self, lbl_80570AE0) == 1)
            fn_80128A14(self, 13, 4);
        break;
    }
}

/* The enemy work's action-program channel 4: the same `fn_80131E00` hook, then phase 0 arms motion
 * 89 and starts the `lbl_80570AE0` table with a zero +0x1CC sequence; phase 1 waits out its
 * 256-frame window and lands the +0x1E4 value on part 13/5. */
extern "C" void em_act_prog_4(_ENEMY_WORK* self) {
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F5B8(self, 89, 0, 0);
        fn_80134004(self, 0, lbl_8079BC88);
        fn_8012F8C8(self, lbl_8079BCE8);
        fn_801303EC(self, fn_8013032C(self));
        fn_80378F9C(self, 255);
        break;
    case 1:
        if (fn_80134114(self, 0, 256) == 1)
            fn_80128A14(self, 13, 5);
        break;
    }
}

/* The enemy work's action-program channel 5: the two per-frame hooks run unconditionally, then
 * phase 0 arms motion 202, resets the +0x1CC sequence and the part 255 scale and opens two
 * `fn_8012933C` hit windows; phase 1 sequences three `em_frame_check` windows, and once the motion
 * and `fn_8012D1A0` both report done it either lands the part 13/6 hit or ends the program. */
extern "C" void em_act_prog_5(_ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    fn_80043EA8(&pos);
    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 202, 0, 0);
        fn_801303EC(self, lbl_8079BC88);
        fn_80378F9C(self, 255);
        fn_80136D14(self);
        fn_8012933C(self, 0, 32, 8);
        fn_8012933C(self, 1, 31, 24);
        fn_80131E00(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_8079BD04, lbl_8079BC88) == 1)
            fn_80136D14(self);
        if (em_frame_check(self, 2, lbl_8079BCA0, lbl_8079BC88) == 1)
            fn_80131E00(self);
        if (em_frame_check(self, 0, lbl_8079BCB4, lbl_8079BC88) == 1) {
            nw4r::math::VEC3 off;

            fn_80136B50(self, -1, 5);
            fn_80041E40(&pos, ((Fn80041E8C)fn_80041E8C)(&off, lbl_8079BC88, lbl_8079BC88,
                                                        lbl_8079BD08));
            ((EmShellFuncs*)shell_set_func_ptr)
                ->method_0x2C(self, 9, 1, &pos, lbl_8079BC94, self->field_0xAEA);
        }
        if (fn_8012F93C(self) == 1 && fn_8012D1A0(self) == 1) {
            if (fn_80382E48(self, 0) == 1)
                fn_80128A14(self, 13, 6);
            else
                fn_80127F48(self);
        }
        break;
    }
}

/* The enemy work's action-program channel 6: the four phases drive the +0x36C target vector - phase
 * 0 measures the bearing to it, picks the +0x006 turn direction off the +0x1C0 heading, arms motion
 * 31 or 32 and builds the +0x310 offset vector and the +0x318 radius; phases 1 and 2 run the turn
 * and the `lbl_80570AE0` sequence, phase 3 hands the result on once `fn_8012D1A0` reports done. */
extern "C" void em_act_prog_6(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u16 angle;

        self->state++;
        fn_80130478(self, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle >= 0x8000)
            self->state_0x006 = 1;
        else
            self->state_0x006 = 0;
        switch (self->state_0x006) {
        case 0:
            fn_8012F5B8(self, 31, 10, 0);
            if (angle < 0x2000)
                angle = 0x2000;
            else if (angle > 0x6000)
                angle = 0x6000;
            break;
        case 1:
            fn_8012F5B8(self, 32, 10, 0);
            if (angle < 0xA000)
                angle = (u16)-0x6000;
            else if (angle > 0xE000)
                angle = (u16)-0x2000;
            break;
        }
        fn_801353F8(self);
        self->field_0x318 = lbl_8079BCD8 * fn_8012F8E4(self) * get_em_chg_scale(self);
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + angle);
        if (angle > 0x8000)
            angle = (u16)(0x10000 - angle);
        self->timer_0x020 = (s16)angle;
        fn_80129668(self, 0, 30);
        break;
    }
    case 1:
        switch (self->state_0x006) {
        case 0:
            fn_80133E3C(self, self->timer_0x020, lbl_8079BCDC, lbl_8079BCE0);
            break;
        case 1:
            fn_80133E3C(self, -self->timer_0x020, lbl_8079BCDC, lbl_8079BCE0);
            break;
        }
        if (em_frame_check(self, 3, lbl_8079BCC4, lbl_8079BCE4) == 1)
            CancelFade(self);
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_80134964(self, lbl_80570AE0, 0, 0, 0);
        }
        break;
    case 2:
        if (fn_80134B0C(self, lbl_80570AE0) == 1) {
            self->state++;
            self->state_0x006 = 0;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 2, 10, 0);
            fn_80134004(self, 0, lbl_8079BCD4);
        }
        break;
    case 3:
        switch (self->state_0x006) {
        case 0:
            if (fn_80134114(self, 0, 128) == 1) {
                self->state_0x006++;
                if (fn_8012D1A0(self) == 0)
                    fn_8012F5C4(self, 20, 20, 0, 1);
            }
            break;
        case 1:
            break;
        }
        if (fn_8012D1A0(self) == 1) {
            if (fn_80382E48(self, 1) == 1)
                fn_80128A14(self, 13, 7);
            else
                fn_80127F48(self);
        }
        break;
    }
}

/* The enemy work's action-program channel 7: phase 0 arms motion 6 sub-motion 10, stores the
 * 300-frame countdown at +0x020 and starts the `fn_803B9BA0` job over the +0x188 position; phase 1
 * ticks that countdown and lands the +0x1E4 value on part 13/8 when it reaches zero. */
extern "C" void em_act_prog_7(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 6, 10, 0);
        self->timer_0x020 = 300;
        fn_803B9BA0(self, &self->pos, 60);
        break;
    case 1:
        if (--self->timer_0x020 <= 0)
            fn_80128A14(self, 13, 8);
        break;
    }
}

/* The enemy work's action-program channel 8: phase 0 arms motion 10 sub-motion 6 and sets the
 * 1000-frame wait at +0x020; phase 1 ends the program once `fn_8012F93C` reports the motion done. */
extern "C" void em_act_prog_8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 10, 6, 0);
        fn_80130CDC(self, 1000);
        break;
    case 1:
        if (fn_8012F93C(self) == 1)
            fn_80127F48(self);
        break;
    }
}

/* The program dispatcher: tail-calls the step of the program id at +0x1E6 through the unit's own
 * 9-entry jump table (the table's case 0 is the em019 band's `fn_8037E0E8`). */
extern "C" void em_act_prog_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8037E0E8(self);
        break;
    case 1:
        em_act_prog_1(self);
        break;
    case 2:
        em_act_prog_2(self);
        break;
    case 3:
        em_act_prog_3(self);
        break;
    case 4:
        em_act_prog_4(self);
        break;
    case 5:
        em_act_prog_5(self);
        break;
    case 6:
        em_act_prog_6(self);
        break;
    case 7:
        em_act_prog_7(self);
        break;
    case 8:
        em_act_prog_8(self);
        break;
    }
}

/* The per-frame action runner: clears the +0x358 run flag, dispatches on the action id at +0x1E5
 * through the unit's own 14-entry table, ticks the +0x35A counter while +0x358 is 1 (clamped at 450)
 * and runs the +0x1E2 hook pair. */
extern "C" void em_act_run(_ENEMY_WORK* self) {
    self->field_0x358 = 0;
    switch (self->action) {
    case 0:
        fn_803797F4(self);
        break;
    case 1:
        fn_80379DE4(self);
        break;
    case 2:
        fn_8037A470(self);
        break;
    case 3:
        fn_8037A7F0(self);
        break;
    case 4:
        fn_8037AF08(self);
        break;
    case 5:
        fn_8037DAC8(self);
        break;
    case 6:
        fn_8037DC04(self);
        break;
    case 7:
        fn_8037DFF0(self);
        break;
    case 8:
        fn_8037E0D4(self);
        break;
    case 9:
        em_act_prog_dispatch(self);
        break;
    /* Retail's range check is `cmplwi r0, 13` over a 14-entry table, so the source's switch carried
     * cases 10..13 as well - they share the default's body (rule 8's "cases share a break"), which is
     * what makes the table 14 entries wide instead of 10. */
    case 10:
    case 11:
    case 12:
    case 13:
    default:
        fn_80127F48(self);
        break;
    }
    if (self->field_0x358 == 1) {
        if (++self->tev_0x35A > 450)
            self->tev_0x35A = 450;
    } else {
        self->tev_0x35A = 0;
    }
    if (self->field_0x1E2 == 1) {
        fn_8012CF20(self);
        fn_80131E74(self);
    }
}

/* The per-part damage check: for each of the seven slots, either raises the slot's damage meter
 * (`fn_80127CC4`) while its `fn_8037900C` arm byte is clear and its damage level is still below the
 * slot's threshold, or ticks it down (`fn_80127D20`) through the pair of ids. */
extern "C" void em_parts_damage_ck(_ENEMY_WORK* self) {
    if (fn_8037900C(self, 2) == 0 && em_parts_damage_level_get(self, 0) < 2)
        fn_80127CC4(self, 0);
    else
        fn_80127D20(self, 0, 0);
    if (fn_8037900C(self, 0) == 0 && em_parts_damage_level_get(self, 6) < 2)
        fn_80127CC4(self, 1);
    else
        fn_80127D20(self, 1, 1);
    if (fn_8037900C(self, 1) == 0 && em_parts_damage_level_get(self, 1) < 2)
        fn_80127CC4(self, 2);
    else
        fn_80127D20(self, 2, 2);
    if (fn_8037900C(self, 0) == 0 &&
        (em_parts_damage_level_get(self, 2) < 1 || em_parts_damage_level_get(self, 3) < 1))
        fn_80127CC4(self, 3);
    else
        fn_80127D20(self, 3, 3);
    if (fn_8037900C(self, 0) == 0 &&
        (em_parts_damage_level_get(self, 4) < 1 || em_parts_damage_level_get(self, 5) < 1))
        fn_80127CC4(self, 4);
    else
        fn_80127D20(self, 4, 4);
    if (fn_8037900C(self, 0) == 0 && em_parts_damage_level_get(self, 6) < 2)
        fn_80127CC4(self, 5);
    else
        fn_80127D20(self, 5, 5);
    if (fn_8037900C(self, 3) == 0 && em_parts_damage_level_get(self, 7) < 1)
        fn_80127CC4(self, 6);
    else
        fn_80127D20(self, 6, 6);
}

/* Every twentieth frame, drops the record's own ground-stamp vector: builds the (0, -20, 100) offset
 * and hands it to `fn_8010562C` with the record's own flag. */
extern "C" void em_act_effect_ck(_ENEMY_WORK* self) {
    nw4r::math::VEC3 off;

    fn_80043EA8(&off);
    if (fn_8012EC60(self) == 1) {
        if (system_w.field_0x0c % 20 == 0) {
            setVector3(&off, lbl_8079BC88, lbl_8079BE88, lbl_8079BD24);
            fn_8010562C(self, 25, 25, &off, lbl_8079BCA8);
        }
    }
}
