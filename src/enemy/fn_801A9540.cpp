/* enemy/fn_801A9540.cpp - the enemy per-action state-machine band between
 * `enemy/fn_801A4504.cpp` (ends at 0x801A9540) and the unclaimed run at 0x801B0010.
 *
 * .text 0x801A9540..0x801B0010 (0x6AD0), 82 functions; extab 0x8000F324..0x8000F54C (69 records);
 * extabindex 0x8002ABA0..0x8002AEDC (69 x 12 B).  The extab/extabindex runs are the entries whose
 * `funcStart` falls in this range, read out of the DOL: the entry before the first is
 * `0x8002AB94` (fn_801A94C4, the neighbour unit's last framed function) and the one after the last
 * is `0x8002AEE8` (fn_801B0010, the next proposal's first), so both runs are exactly this unit's.
 *
 * Registration (proposal/801A9540_fn_801A9540.cpp).  The range is registered once, here, at its
 * final home.  Which evidence class decided the name and module:
 *   * class 1 (a `__FILE__` string) fails.  `nm -u` over the range's 82 split objects names no
 *     string symbol at all: the whole undefined set is the `.sdata2` float pool, the `.data`
 *     tables/jumptables and `fn_XXXXXXXX` callees.  Nothing in the range references
 *     `enemy_control.cpp` (the last accepted `__FILE__` name before the run, at
 *     0x801411B8..0x80147CE0) either.
 *   * class 2 (a runtime-dump name) fails: `python tools/symbols/dumpmap.py lookup 0x801A9540`
 *     answers the `zz_01a9540_` placeholder, which the brief states is not evidence.
 *   * class 3 decides the module: `enemy`.  Both bracketing registered units are `enemy/*`
 *     (`enemy/fn_801A4504.cpp` below, `enemy/fn_801B7020.cpp` above), and every callee out of the
 *     range is enemy-band (`em_act_ck`, `em_frame_check`, `em_get_mot_no`, `get_joint_wmat_em`,
 *     `get_move_work_adrs`, `em_action_finish`, `em_mot_set`, ...); every body drives the
 *     `_ENEMY_WORK` record `include/enemy/ENEMY_WORK.h` owns.
 *   * class 4 keeps the name: nothing supports a file name, so the map's own `fn_801A9540` stem is
 *     the file name (the sibling units use the same scheme).
 *
 * C++ (`-lang=c++` through the lib's `cflags_main`) because the range reaches genuinely mangled
 * callees (`em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`,
 * `get_joint_wmat_em__FP11_ENEMY_WORKUlPQ34nw4r4math5MTX34`, `ran_suu__Fl`) through their real
 * signatures (rule 9); every plain `fn_XXXXXXXX` definition here is `extern "C"` so its map name is
 * emitted.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked over
 * config/RMHE08/symbols.txt - every symbol this unit defines is a bare `.text` entry with no owner
 * name - and `python tools/symbols/dumpmap.py lookup 0x801A9540` answers the `zz_01a9540_`
 * placeholder form, which is not evidence).  The owner names this range *does* reach
 * (`em_parts_damage_level_get`, `em_get_mot_no`) are its callees, not its own functions.
 *
 * What the range is.  It is the enemy side's per-action step band: 20 of its functions are
 * `switch (self->state_sub)` dispatchers that tail-call one handler each, and the handlers
 * themselves are the small `switch (self->state)` step machines the armed action runs
 * (`em_move_mode_set(self, 0)` starts the motion, `em_mot_set`/`em_mot_set_ck` set it, `em_mot_end_ck`/
 * `fn_80134114` report it, `em_action_finish(self)` ends the action).  `fn_801AD1F0`/`fn_801AD3F0` are
 * the two program-table dispatchers (the `.data` tables `lbl_805B07E0`.. and `lbl_805B0C08`..).
 * The bodies read `_ENEMY_WORK` fields through `include/enemy/ENEMY_WORK.h` and the enemy-band
 * callees through their owner headers.
 *
 * Status (measured with `python tools/units/recompile.py enemy/fn_801A9540.cpp --measure <symbol>`,
 * the official report metric).  54 of the 82 functions are written and EVERY written row is above
 * the 80 % bar: 29 are byte-identical and the mean is 97.81 %.  The 28 unwritten rows are the
 * follow-up queue in the outbox, in address order.
 *
 * Residuals, by measurement (every near-miss is a codegen shape, not comprehension):
 *   * `fn_801A960C` 87.60 (100 B target / 88 B ours).  The target narrows its `u8` parameter
 *     (`clrlwi r4,r4,24`) before the `kind - 2 <= 1` test and truncates the callee's `u8` return to
 *     a byte (`clrlwi r0,r3,24`) before the bit-0 test (`clrlwi r0,r0,31`); ours folds both away
 *     and emits the fused `clrlwi. r0,r3,31`.  `em_parts_damage_level_get` was measured both as
 *     `u8` (the owner's return) and with an explicit `(u8)` cast, and the truncation is dropped
 *     either way; the fused record-form is what costs the 12 B.
 *   * `fn_801AC0E0`/`fn_801AC1A4`/`fn_801ACADC` 92.2-92.7 (4-8 B short).  These are the only rows
 *     whose target uses paired-single register save/restore (`psq_st`/`psq_l` around the `f32`
 *     argument, which GNU objdump 2.42 renders as `xscmpeqdp`/`vmrghb`); the bodies match, the FP
 *     spill shape does not.
 *   * the remaining sub-100 rows differ by one tail branch or a 4 B spill slot
 *     (`fn_801AB9DC` 97.44, `fn_801ABA90` 96.10, `fn_801ABD2C` 97.98, `fn_801ABF84` 96.10,
 *     `fn_801AC268` 95.56, `fn_801AC388` 93.90, `fn_801AC524` 98.14, `fn_801AC6B0` 92.80,
 *     `fn_801AC778` 95.35, `fn_801AC824` 96.19, `fn_801AC8CC` 95.57, `fn_801AC9E4` 95.81,
 *     `fn_801ACB94` 97.24, `fn_801ACD5C` 95.24, `fn_801AD498` 95.56, `fn_801AD528` 95.56,
 *     `fn_801A9540` 98.71, `fn_801A9DF4` 97.78, `fn_801AA0F8` 95.43, `fn_801A9670` 95.11,
 *     `fn_801ABC74` 95.22).
 *   * `fn_801AD1F0` (0x200) and `fn_801AD3F0` (0x90) are NOT written: their bodies are ready (a
 *     `switch (self->state_sub)` over the two `.data` program tables) but `fn_801251D0`'s declared
 *     signature has three parameters where the target's call sites pass four - r3 `self`, r4 the
 *     table, r5 0, r6 the id (measured at both this range's call sites and at
 *     `enemy/fn_8014A1BC.c`'s: `lis/addi r4,table; li r5,0; li r6,id; b 0x801251D0`, and the callee
 *     itself is `clrlwi r5,r5,24; b 0x80124C5C`).  Writing them needs `include/enemy/fn_801251D0.h`
 *     and the `extern "C" void fn_801251D0(...)` definition in `src/enemy/fn_801251D0.cpp` moved to
 *     the four-argument form - a shared-file edit outside this unit's registration, so it is left
 *     as a recorded correction for the owner rather than made here.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801B0010.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "unsplit/enemy.h"
#include "enemy/fn_801993E0.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_8011D448.h"
#include "ef/fn_800CDB2C.h"
#include "g3d/g3d_anmchr.h"
#include "fn_8004CAD8.h"
#include "sys_mem.h"
#include "enemy/fn_80138074.h"
#include "unsplit/unknown.h"

/* The pooled `.sdata2` floats this range reads (each is a bare marker symbol in the map; the values
 * drive the comparisons/floats below).  Declared, never defined here: the pool belongs to the data
 * pass. */
extern const f32 lbl_80798528;
extern const f32 lbl_8079852C;
extern const f32 lbl_80798538;
extern const f32 lbl_80798560;
extern const f32 lbl_807985C8;
extern const f32 lbl_807985CC;
extern const f32 lbl_80798664;
extern const f32 lbl_807986A8;
extern const f32 lbl_807988F4;
extern const f32 lbl_807988F8;
extern const f32 lbl_807988FC;
extern const f32 lbl_80798900;
extern const f32 lbl_80798904;
extern const f32 lbl_8079893C;
extern const f32 lbl_80798948;
extern const f32 lbl_8079894C;
extern const f32 lbl_80798950;
extern const f32 lbl_80798954;
extern const f32 lbl_80798958;
extern const f32 lbl_8079895C;
extern const f32 lbl_80798960;
extern const f32 lbl_8079896C;
extern const f32 lbl_80798974;
extern const f32 lbl_80798978;
extern const f32 lbl_8079897C;
extern const f32 lbl_80798980;
extern const f32 lbl_80798984;
extern const f32 lbl_80798988;
extern const f32 lbl_8079898C;
extern const f32 lbl_80798990;
extern const f32 lbl_80798994;
extern const f32 lbl_80798998;
extern const f32 lbl_8079899C;
extern const f32 lbl_807989A0;
extern const f32 lbl_807989A4;
extern const f32 lbl_807989A8;
extern const f32 lbl_807989AC;
extern const f32 lbl_807989B0;
extern const f32 lbl_807989B4;
extern const f32 lbl_807989B8;
extern const f32 lbl_807989BC;
extern const f32 lbl_807989C0;
extern const f32 lbl_807989C4;
extern const f32 lbl_807989C8;
extern const f32 lbl_807989CC;
extern const f32 lbl_807989D0;
extern const f32 lbl_80798A48;
extern const f32 lbl_80798A4C;
extern const f32 lbl_80798A70;
extern const f32 lbl_80798A74;
extern const f32 lbl_80798A78;

extern u8 lbl_805701A0[];
extern u8 lbl_805701E0[];
extern u8 lbl_80570220[];

extern "C" {

/* 0x80339E04 `lobby/lb_companion_ui.cpp` (the companion/status UI band).  The declaration sits here,
 * not in `include/unsplit/unknown.h`, because that band may not carry a symbol a registered unit owns
 * (rule 2), and the owner's header cannot be included from this unit - it redeclares `system_w` as
 * `LbSystemView` against this unit's own view (measured - `(10563) identifier 'system_w' redeclared`).
 * The two-argument signature is this unit's call site; the callee reads only r3. */
void lb_area_change_send(u8 a, s32 b);

/* r3 the work record; the per-action entry: resets the joint-effect slots on the request, ticks the
 * motion, and walks the two live slots through `fn_803B9588`. */
void fn_801A9540(struct _ENEMY_WORK* self) {
    s32* slots = self->handles_0x328;

    if (fn_8012D1A0(self) == 1U) {
        lb_area_change_send((u8)my_player_no(), 0);
    }
    fn_8013A9F4(self);
    if (self->area_no == 2) {
        em_move_mode_set(self, 0);
        em_mot_set(self, 13, 0, 0);
        fn_801303EC(self, lbl_80798538);
        self->field_0x7B0 = lbl_807988F4;
    }
    fn_8019EA04(self);
    fn_803B9588(self, 3, slots, 0, 0);
    if (slots[0] != -1) {
        self->states_0x338[0] = 8;
    }
}

/* r3 the work record, r4 the part kind (a byte); the "this part is attackable" predicate: the part
 * must be 2 or 3, its damage level must not be odd, and the per-part flag must be clear. */
s32 fn_801A960C(struct _ENEMY_WORK* self, u8 kind) {
    if ((u32)(kind - 2) <= 1U && (em_parts_damage_level_get(self, kind) & 1) == 0 &&
        self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

/* r3 the work record, r4 the part kind; raises the per-part damage flag the kind maps to.  Kinds 6
 * and 7 only raise theirs at damage level 2 or above. */
void fn_801A9670(struct _ENEMY_WORK* self, u8 kind) {
    switch (kind) {
    case 0:
        self->flags_0x836 |= 0x1000;
        return;
    case 1:
        self->flags_0x836 |= 0x2000;
        return;
    case 6:
        if (em_parts_damage_level_get(self, 6) >= 2U) {
            self->flags_0x836 |= 0x8000;
            return;
        }
        return;
    case 7:
        if (em_parts_damage_level_get(self, 7) >= 2U) {
            self->flags_0x836 |= 0x4000;
        }
        return;
    default:
        return;
    }
}

/* r3 the work record; asks the area table for action 13's slot, 2 in area 1 and 3 otherwise. */
void fn_801A9724(struct _ENEMY_WORK* self) {
    if (self->area_no == 1) {
        fn_80128AEC(self, 13, 2);
        return;
    }
    fn_80128AEC(self, 13, 3);
}

/* r3 the work record; the kind-3 request: clears its slot word, arms the aim motion and takes the
 * 3-byte area-table entry. */
void fn_801A9C6C(struct _ENEMY_WORK* self) {
    u8 sp8[8];

    fn_8005D1AC(sp8, 0);
    fn_8013A654(self, 3);
}

/* r3 the work record; releases the joint-effect slots and re-arms the two it owns. */
void fn_801A9DF4(struct _ENEMY_WORK* self) {
    s32* slot = self->handles_0x328;
    u16 i;

    fn_8019EA04(self);
    for (i = 0; i < 2; i++) {
        fn_803B9588(self, i, slot + i, 5, 0);
    }
}

/* r3 the record, r4 the sign-extended flag; releases the record's user data and frees it when the
 * flag is positive.  Returns r3 (the record). */
s32 fn_801AA0F8(s32 record, s16 free_it) {
    if (record != 0) {
        fn_8013918C((struct _ENEMY_WORK*)record, 0);
        if (free_it > 0) {
            operator delete((void*)record);
        }
    }
    return record;
}

/* r3 the work record; clears the first byte of its effect-slot block. */
void fn_801AB048(struct _ENEMY_WORK* self) {
    self->init_0x328.slots_0x328[0] = 0;
}

/* r3 the work record; the escape action's step machine (motion 1, then the end request). */
void fn_801AB930(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the sub-state dispatcher over the escape action's three recorded sub-steps. */
void fn_801AB9AC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801AB930(self);
        return;
    case 1:
        fn_801AB930(self);
        return;
    case 2:
        fn_801AB930(self);
        return;
    }
}

/* r3 the work record; the two-motion escape variant (0x17 then 0x18). */
void fn_801AB9DC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 0x14, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0x14, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the three-motion escape variant (0x15, motion 2, 0x18). */
void fn_801ABA90(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0x14, 0);
            return;
        }
        return;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0x16). */
void fn_801ABB7C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 0x14, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0x19). */
void fn_801ABBF8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the variant selector; the two-motion escape whose second motion depends on
 * the selector (7 or 0x1D). */
void fn_801ABC74(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (kind) {
        case 0:
            em_mot_set(self, 7, 0x10, 0);
            return;
        case 1:
            em_mot_set(self, 0x1D, 0x10, 0);
            return;
        }
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the selector the escape's first motion depends on; the variant that also
 * arms the effect scale. */
void fn_801ABD2C(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (kind) {
        case 0:
            em_mot_set(self, 8, 0xA, 0);
            break;
        case 1:
            em_mot_set(self, 0x1A, 0x10, 0);
            break;
        case 2:
            em_mot_set(self, 0x1C, 0x10, 0);
            break;
        }
        em_mot_speed_set(self, lbl_80798978);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0x1B). */
void fn_801ABE10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 0x10, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0xD). */
void fn_801ABE8C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD, 0x10, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion escape variant (0x11). */
void fn_801ABF08(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 0x10, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the three-motion escape variant (0xA, 0xB, 0xC). */
void fn_801ABF84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xB, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xC, 0, 0);
            return;
        }
        return;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the escape band's sub-state dispatcher (the 13 entries the `.data` table at
 * 0x805B073C selects). */
void fn_801AC070(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801AB9DC(self);
        return;
    case 1:
        fn_801ABA90(self);
        return;
    case 2:
        fn_801ABB7C(self);
        return;
    case 3:
        fn_801ABBF8(self);
        return;
    case 4:
        fn_801ABC74(self, 0);
        return;
    case 5:
        fn_801ABD2C(self, 0);
        return;
    case 6:
        fn_801ABE10(self);
        return;
    case 7:
        fn_801ABE8C(self);
        return;
    case 8:
        fn_801ABF08(self);
        return;
    case 9:
        fn_801ABF84(self);
        return;
    case 10:
        fn_801ABD2C(self, 1);
        return;
    case 11:
        fn_801ABD2C(self, 2);
        return;
    case 12:
        fn_801ABC74(self, 1);
        return;
    }
}

/* r3 the work record, f1 the blend the motion's effect is armed with; the held-swing step
 * (motion 3, mask 0x80). */
void fn_801AC0E0(struct _ENEMY_WORK* self, f32 blend) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 3, 4, 0);
        if (self->field_0x00A == 2) {
            em_mot_speed_set(self, lbl_8079897C);
        }
        fn_80134004(self, 0, blend);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, f1 the blend; the held-swing step (motion 4, mask 0x80). */
void fn_801AC1A4(struct _ENEMY_WORK* self, f32 blend) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        if (self->field_0x00A == 2) {
            em_mot_speed_set(self, lbl_80798948);
        }
        fn_80134004(self, 0, blend);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the selector; the selector-driven escape variant (the sub-state dispatches
 * 0 or 1 to the position/rotation reset and the rest to be begun). */
void fn_801AC268(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (kind) {
        case 0:
            fn_80134964(self, lbl_805701A0, 0, 0, 0);
            return;
        case 1:
            fn_80134964(self, lbl_805701A0, 1, 0, 0);
            return;
        case 2:
            fn_80134964(self, lbl_805701A0, 0, 0, 0);
            em_mot_speed_set(self, lbl_80798948);
            return;
        }
        return;
    case 1:
        if (fn_80134B0C(self, lbl_805701A0) == 1) {
            em_action_finish(self);
            return;
        }
        if (kind == 2) {
            em_mot_speed_set(self, lbl_80798948);
        }
        return;
    }
}

/* r3 the work record, r4 the escape's second-motion selector; the burst step. */
void fn_801AC388(struct _ENEMY_WORK* self, f32 arg1) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 9, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 4, 0, 0x48);
            fn_80134004(self, 0, arg1);
            return;
        }
        return;
    case 3:
        if (fn_80134114(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the "effect shot" step (one table entry, then the end request). */
void fn_801AC498(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_80134964(self, lbl_805701E0, 0, 0, 0);
        return;
    case 1:
        if (fn_80134B0C(self, lbl_805701E0) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the table variant (0, 1 or anything else = 0x4000); the "effect shot"
 * variant step. */
void fn_801AC524(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        if (kind == 0) {
            fn_80134964(self, lbl_80570220, 0, 0, 0);
            return;
        }
        if (kind == 1) {
            fn_80134964(self, lbl_80570220, 1, 0, 0);
            return;
        }
        fn_80134964(self, lbl_80570220, 0, 0, 0x4000);
        return;
    case 1:
        if (fn_80134B0C(self, lbl_80570220) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the timer-gated step: arms the motion, holds for 0x100 + the low byte of the
 * action bit field, then requests the end. */
void fn_801AC610(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 3, 4, 0);
        self->timer_0x020 = (self->bits_0x1EC & 0xFF) + 0x100;
        return;
    case 1:
        self->timer_0x020 = self->timer_0x020 - 1;
        fn_80133C50(self, 0x80);
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the swing step (motion 4, then a 0x1E + the action bit field hold). */
void fn_801AC6B0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        if (self->field_0x00A == 2) {
            em_mot_speed_set(self, lbl_80798948);
        }
        fn_80134004(self, 0x10, lbl_8079893C);
        self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 0x1E;
        return;
    case 1:
        fn_80134114(self, 0, 0x80);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the instant swing step (motion 3). */
void fn_801AC778(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        if (self->field_0x00A == 2) {
            em_mot_speed_set(self, lbl_8079897C);
        }
        fn_80134004(self, 0x10, lbl_8079893C);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the shot step (motion 4, effect scale, 0x1E + the action bit field). */
void fn_801AC824(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        em_mot_speed_set(self, lbl_80798980);
        self->timer_0x020 = (self->bits_0x1EC & 0x3F) + 0x1E;
        return;
    case 1:
        fn_80133C50(self, 0x100);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, r4 the variant selector; the held shot step. */
void fn_801AC8CC(struct _ENEMY_WORK* self, u8 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        switch (kind) {
        case 0:
            em_mot_speed_set(self, lbl_80798980);
            self->timer_0x020 = (self->bits_0x1EC & 0x3F) + 0x1E;
            break;
        case 1:
            self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 0x1E;
            break;
        }
        fn_80134004(self, 0x10, lbl_8079893C);
        return;
    case 1:
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->state_0x006 == 0 && fn_80134114(self, 0, 0x100) == 1) {
            self->state_0x006++;
        }
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the motion-frame-gated step that arms the second motion and then holds it for
 * 0x1E + the low six bits of the action bit field. */
void fn_801AC9E4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        return;
    case 1:
        fn_80133C50(self, 0x100);
        if (em_frame_check(self, 1, lbl_80798984, lbl_8079893C) == 1) {
            self->state++;
            em_mot_set(self, 4, 2, 0x14);
            em_mot_speed_set(self, lbl_80798980);
            self->timer_0x020 = (self->bits_0x1EC & 0x3F) + 0x1E;
            return;
        }
        return;
    case 2:
        fn_80133C50(self, 0x100);
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record, f1 the blend; the held-swing step (motion 4, effect scale, mask 0x180). */
void fn_801ACADC(struct _ENEMY_WORK* self, f32 blend) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        em_mot_speed_set(self, lbl_80798980);
        fn_80134004(self, 0, blend);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x180) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the motion-frame-gated burst step (the attack ended when the window opens). */
void fn_801ACB94(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        self->timer_0x020 = 0;
        return;
    case 1:
        fn_80133C50(self, 0x100);
        if (em_frame_check(self, 1, lbl_80798984, lbl_8079893C) == 1) {
            self->state++;
            em_mot_set(self, 4, 2, 0x14);
            if (self->field_0x00A == 2) {
                em_mot_speed_set(self, lbl_80798948);
            }
            fn_80134004(self, 0, lbl_8079893C);
            return;
        }
        return;
    case 2:
        if (em_get_mot_no(self) == 4) {
            if (self->state_0x006 < 1U && em_mot_end_ck(self) == 1) {
                if ((self->bits_0x1EC & 0xF) < 8 || self->timer_0x020 > 4) {
                    em_mot_set(self, 0x1B, 4, 0);
                    self->state_0x006++;
                }
                self->timer_0x020 = self->timer_0x020 + 1;
            }
        } else if (em_frame_check(self, 1, lbl_80798984, lbl_8079893C) == 1) {
            em_mot_set(self, 4, 2, 0x14);
            if (self->field_0x00A == 2) {
                em_mot_speed_set(self, lbl_80798948);
            }
        }
        if (fn_80134114(self, 0, 0x100) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the motion-frame-gated burst step (the short variant). */
void fn_801ACD5C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        return;
    case 1:
        fn_80133C50(self, 0x100);
        if (em_frame_check(self, 1, lbl_80798984, lbl_8079893C) == 1) {
            self->state++;
            em_mot_set(self, 4, 2, 0x14);
            if (self->field_0x00A == 2) {
                em_mot_speed_set(self, lbl_80798948);
            }
            fn_80134004(self, 0, lbl_8079893C);
            return;
        }
        return;
    case 2:
        if (fn_80134114(self, 0, 0x100) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the shot band's sub-state dispatcher (the 21 entries the `.data` table at
 * 0x805B0720 selects). */
void fn_801ACE58(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801AC0E0(self, lbl_80798988);
        return;
    case 1:
        fn_801AC1A4(self, lbl_80798988);
        return;
    case 2:
        fn_801AC268(self, 0);
        return;
    case 3:
        fn_801AC388(self, lbl_80798988);
        return;
    case 4:
        fn_801AC498(self);
        return;
    case 5:
        fn_801AC524(self, 0);
        return;
    case 6:
        fn_801AC524(self, 1);
        return;
    case 7:
        fn_801AC524(self, 2);
        return;
    case 8:
        fn_801AC610(self);
        return;
    case 9:
        fn_801AC1A4(self, lbl_8079898C);
        return;
    case 10:
        fn_801AC6B0(self);
        return;
    case 11:
        fn_801AC778(self);
        return;
    case 12:
        fn_801AC824(self);
        return;
    case 13:
        fn_801AC8CC(self, 0);
        return;
    case 14:
        fn_801AC8CC(self, 1);
        return;
    case 15:
        fn_801AC9E4(self);
        return;
    case 16:
        fn_801ACADC(self, lbl_8079898C);
        return;
    case 17:
        fn_801AC268(self, 1);
        return;
    case 18:
        fn_801ACB94(self);
        return;
    case 19:
        fn_801AC268(self, 2);
        return;
    case 20:
        fn_801ACD5C(self);
        return;
    }
}

/* r3 the work record; the two-motion step whose second motion depends on the enemy team. */
void fn_801ACF08(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 4, 0);
        if (self->team == 0x1C) {
            fn_80129668(self, 0, 4);
            return;
        }
        fn_80129668(self, 0, 1);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion step (0x12) with the part-2 arming. */
void fn_801ACFB4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 0x10, 0);
        fn_80129668(self, 0, 2);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the part-3 arming step (0x14) that searches while its window is closed. */
void fn_801AD040(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        fn_80129668(self, 0, 3);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80798990, lbl_8079893C) == 0) {
            self->field_0x1C0 = self->field_0x1C0 + 0x17E;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the part-5 arming step (0xCE) with the effect scale and the 0x200 mask. */
void fn_801AD0F4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCE, 6, 0);
        em_mot_speed_set(self, lbl_80798994);
        fn_80129668(self, 0, 5);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80798998, lbl_8079899C) == 1) {
            fn_80133C50(self, 0x200);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the four-entry part-armer dispatcher. */
void fn_801AD1B4(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801ACF08(self);
        return;
    case 1:
        fn_801ACFB4(self);
        return;
    case 2:
        fn_801AD040(self);
        return;
    case 3:
        fn_801AD0F4(self);
        return;
    }
}

/* r3 the work record; the two-name step whose countdown is 0x78 + the low seven bits of the action
 * bit field. */
void fn_801AD498(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        self->timer_0x020 = (self->bits_0x1EC & 0x7F) + 0x78;
        return;
    case 1:
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the swing step with the 0x3C + the low seven bits of the action bit field
 * hold. */
void fn_801AD528(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 4, 4, 0);
        self->timer_0x020 = (self->bits_0x1EC & 0x7F) + 0x3C;
        return;
    case 1:
        self->timer_0x020 = self->timer_0x020 - 1;
        if (self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion step (0x1B). */
void fn_801AD5B8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the three-entry step dispatcher. */
void fn_801AD634(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801AD498(self);
        return;
    case 1:
        fn_801AD528(self);
        return;
    case 2:
        fn_801AD5B8(self);
        return;
    }
}

/* r3 the work record; the step that hands over to the area table once the per-area selector is set. */
void fn_801AD664(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 0x10, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (fn_801B0010(self->area_no) == 1) {
                em_state_set(self, 0xD, 0);
                return;
            }
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the long scripted step (motions 0x17/7/0x1B/0x66/0x68 driven by the frame
 * counter `fn_80146008(threshold)`). */
void fn_801AD704(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 0, 0);
        fn_80146058(self, lbl_807989A0, lbl_8079893C, lbl_807989A4);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (fn_80146008(0x12C) == 1) {
            self->state++;
            em_mot_set(self, 7, 0, 0x74);
            fn_80146058(self, lbl_807989A8, lbl_8079893C, lbl_807989AC);
        }
        return;
    case 2:
        if (fn_80146008(0x1AE) == 1) {
            self->state++;
            em_mot_set(self, 7, 0, 0);
        }
        return;
    case 3:
        if (fn_80146008(0x320) == 1) {
            self->state++;
            em_mot_set(self, 0x1B, 0, 0x14);
            self->field_0x1C0 = 0x3C72;
            fn_80146058(self, lbl_807989B0, lbl_8079893C, lbl_807989B4);
        }
        return;
    case 4:
        if (fn_80146008(0x3B0) == 1) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0);
            fn_80146058(self, lbl_807989B8, lbl_8079893C, lbl_807989BC);
        }
        return;
    case 5:
        if (fn_80146008(0x3E8) == 1) {
            self->state++;
            em_mot_set(self, 0x68, 0, 0);
            self->field_0x1C0 = 0;
            fn_80146058(self, lbl_8079895C, lbl_8079893C, lbl_80798960);
        }
        return;
    case 6:
        if (em_frame_check(self, 1, lbl_807989C0, lbl_8079893C) == 1) {
            fn_80132160(self);
        }
        if (fn_80146008(0x51E) == 1) {
            self->state++;
            em_mot_set(self, 0x68, 0, 0);
            fn_80146058(self, lbl_8079895C, lbl_8079893C, lbl_80798960);
        }
        return;
    case 7:
        if (em_frame_check(self, 1, lbl_807989C0, lbl_8079893C) == 1) {
            fn_80132160(self);
        }
        return;
    }
}

/* r3 the work record; the single-motion step (0x68) with the three rotation words seeded. */
void fn_801AD988(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x68, 0, 0);
        fn_80146058(self, lbl_8079895C, lbl_8079893C, lbl_80798960);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0x3C72;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the three-motion step (0x18, 0x17, 8) with rotating the second rotation word
 * negative. */
void fn_801ADA2C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 0, 0);
        fn_80146058(self, lbl_807989C4, lbl_8079893C, lbl_807989C8);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0xFFFF71C7;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (fn_80146008(0xC8) == 1) {
            self->state++;
            em_mot_set(self, 0x17, 4, 0);
        }
        return;
    case 2:
        if (fn_80146008(0x1AE) == 1) {
            self->state++;
            em_mot_set(self, 8, 0, 0);
            self->field_0x1C0 = 0xFFFF71C7;
            fn_80146058(self, lbl_807989C4, lbl_8079893C, lbl_807989C8);
        }
        return;
    case 3:
        if (fn_80146008(0x320) == 1) {
            self->state++;
            em_mot_set(self, 0x1B, 0, 0);
            self->field_0x1C0 = 0xFFFF5B06;
            fn_80146058(self, lbl_807989CC, lbl_8079893C, lbl_807989D0);
        }
        return;
    }
}

/* r3 the work record; the single-motion step (0x1B) with the negative second rotation word. */
void fn_801ADBA8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 0, 0);
        fn_80146058(self, lbl_807989CC, lbl_8079893C, lbl_807989D0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0xFFFF5B06;
        self->field_0x1C4 = 0;
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the death/thrash step (motion 0x6B) with the effect-call pair. */
void fn_801AE25C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x6B, 0, 0);
        fn_80146058(self, lbl_8079894C, lbl_80798950, lbl_80798954);
        fn_8014610C(self, lbl_8079893C, lbl_80798958, lbl_8079893C);
        fn_8014619C(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the death step (motion 0x68) with the effect-call pair. */
void fn_801AE670(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x68, 0, 0x32);
        fn_80146058(self, lbl_80798A48, lbl_8079896C, lbl_80798A4C);
        fn_8014610C(self, lbl_8079893C, lbl_80798974, lbl_8079893C);
        fn_8014619C(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* r3 the work record; the death step (motion 4) with the effect-call pair. */
void fn_801AE8EC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 0, 0);
        fn_80146058(self, lbl_80798A70, lbl_8079893C, lbl_80798A74);
        fn_8014610C(self, lbl_8079893C, lbl_80798A78, lbl_8079893C);
        fn_8014619C(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

} /* extern "C" */
