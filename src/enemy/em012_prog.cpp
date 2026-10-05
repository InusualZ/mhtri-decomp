/* enemy/em012_prog.cpp - enemy 012 program
 *
 * `.text` 0x80170600..0x80176C30, 30 functions written (the rest of the range is not decompiled yet).
 * Phase 4: fold of 3 registered units, built from `enemy/fn_80170600.cpp`, `enemy/fn_80170FA8.cpp`, `enemy/fn_80171194.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 */

/* Retired header of `enemy/fn_80170600.cpp` (kept for its notes and residuals): */
/* src/enemy/fn_80170600.cpp - one `_ENEMY_WORK` action family, `.text` 0x80170600..0x80170FA8 (18 functions).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap: every
 * `dump` name in the range is `zz_XXXXXXXX_`, and the one real dump hit, `l2cu_release_rcb`, is flagged
 * `ambiguous` in dumpmap-join.json - the same name is proposed for two addresses - so it is not evidence).
 *
 * What it is.  The unit is the enemy AI action-state family that hangs off `_ENEMY_WORK`.
 *   * `fn_80170A00` is the dispatcher: it switches on `state_sub` (the action slot) and tail-calls one
 *     of the per-slot step functions (`fn_8017088C`/`fn_80170908`/`fn_80170984`).
 *   * `fn_8017088C`, `fn_80170908`, `fn_80170984`, `fn_80170A54`, `fn_80170AD0`, `fn_80170B4C`,
 *     `fn_80170C68`, `fn_80170D04`, `fn_80170D74`, `fn_80170DF0`, `fn_80170E78`, `fn_80170EF4` are the
 *     per-action step machines: they switch on `state`, advance it, arm the action through
 *     `em_move_mode_set`/`em_mot_set`/`em_mot_set_ck`, then wait on `em_mot_end_ck`/`em_frame_check` and finish
 *     through `em_action_finish`.
 *   * `fn_80170600` clears the two `field_0x328`/`field_0x32A` timers; `fn_80170610` is the slot-2 setup
 *     (reads the action byte pair through `fn_80176090`, then seats the position through `setVector3`);
 *     `fn_801706B8` arms a status effect through `em_spawn_request`; `fn_80170758` reacts to an event byte;
 *     `fn_80170804` runs the slot-0 idle tick.
 *
 * Language.  C++: `nw4r::math::setVector3` is declared for C++ only (`nw4r/math.h`), and the calls the
 * target makes to `em_die_ck__FP11_ENEMY_WORK`/`em_frame_check__FP11_ENEMY_WORKUsff` are the manglings of
 * the C++ functions `em_die_ck`/`em_frame_check` (mangle.py verified).  The unit's own symbols are plain
 * (`fn_80170600`, no mangling), so they carry `extern "C"`.
 *
 * Types.  `_ENEMY_WORK` lives in its one shared home, `enemy/ENEMY_WORK.h` (docs/plan.md 6.5 rule 1).
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit enemy/fn_80170600.cpp`.
 */

/* Retired header of `enemy/fn_80170FA8.cpp` (kept for its notes and residuals): */
/* enemy/fn_80170FA8.cpp - four `_ENEMY_WORK` action handlers, `.text` 0x80170FA8..0x80171194 (492 B).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_
 * name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * What it is.  Four steps of one enemy action.  `fn_80170FA8`, `fn_80171038` and `fn_801710B4` are the
 * same two-state machine on `state_0x05`: state 0 advances the state and posts the action's message
 * through `em_move_mode_set` + `em_mot_set_ck` (`0xC9`) or `em_mot_set` (`0xE`); state 1 waits on the shared
 * frame checks (`em_frame_check`, `em_mot_end_ck`) and then fires the action result (`em_state_set`) or
 * the next move (`em_action_finish`).  `fn_80171130` is the dispatcher: it reads `state_sub` (+0x1E6) and
 * tail-calls one handler per action code, codes 0 and 9 doing nothing.
 *
 * Object: `_ENEMY_WORK`, included from `enemy.h` (name evidence: the mangled callee
 * `em_frame_check__FP11_ENEMY_WORKUsff` carries the 11-character type name).  Included, not copied
 * (docs/plan.md 6.5 rule 1).
 *
 * Language: C++.  The one mangled callee is a C++ free function: its map spelling
 * `em_frame_check__FP11_ENEMY_WORKUsff` decodes to `em_frame_check(_ENEMY_WORK*, u16, f32, f32)`, so
 * the source names the owner's real identifier and the front-end emits the map's mangling (rule 9);
 * writing the mangled spelling as the callee would be a rule-9 violation the gate refuses.  The four
 * unit functions stay plain symbols through `extern "C"`, so the map's `fn_XXXXXXXX` stems pair.
 *
 * Data: the unit owns no pool.  `lbl_80797910` (0.0f) and `lbl_80797928` (170.0f) are shared `.sdata2`
 * constants (declared, not defined here).  The `.data` jump table `jumptable_805A83DC` is emitted by the
 * compiler from the dispatch switch; its run 0x805A83DC..0x805A8418 is requested through the outbox's
 * `range` rather than claimed here, because the emitted section is 8-aligned where the retail range is
 * 4-aligned (the preceding `em012_prog_tbl` ends there) - the split warns on that claim, and a data
 * range that does not lay out byte-for-byte is not a claim (docs/plan.md 8.4).
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit enemy/fn_80170FA8.cpp`.
 */

/* Retired header of `enemy/fn_80171194.cpp` (kept for its notes and residuals): */
/* enemy/fn_80171194.cpp
 *
 * Enemy-band translation unit at .text 0x80171194-0x80176C58 (46 functions, 23 236 B).
 *
 * Registered (attribute.py queue): one maximal unclaimed run
 * whose left/right boundaries `tools/splits/tudiscover.py at 0x80171194` calls strong (2/1 anchors),
 * and whose extabindex run (0x80029790-0x80029928, 34 records) places every entry inside this .text
 * range. The unit owns `extab`/`extabindex`/`.text` (dtk's split added the `.ctors` word at
 * 0x8056F334-0x8056F338 on its own, so the TU has a static constructor); the .rodata/.data/.bss/
 * .sdata/.sbss/.sdata2 runs tudiscover lists are left unclaimed (they stay in the auto data objects) -
 * the neighbours (`enemy/fn_8014A1BC.c`, `enemy/fn_8013BE60.c`, `enemy/fn_8012BDF4.cpp`) claim exactly
 * those three sections.
 *
 * Module `enemy`: the nearest registered units are `enemy/fn_8014A1BC.c` (0x8014A1BC-0x801502C8) and the
 * rest of the `enemy` directory, and 11 of the run's mangled-undefined references are the enemy work
 * API (`em_frame_check__FP11_ENEMY_WORKUsff`, `em_get_mot_no__FP11_ENEMY_WORK`,
 * `get_em_chg_scale__FP11_ENEMY_WORK`, ...) plus nw4r::math helpers. Language C++: every call out of the
 * range is a mangled symbol (a C TU would reference them unmangled).
 *
 * Name: the map has only `fn_XXXXXXXX` for this range and the runtime dump has only `zz_` placeholders
 * (`.pi/notes/dumpmap-join.json`), so the map stem is kept as the file name.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked symbols.txt via the
 *   brief's section 3 inventory - all 46 symbols are fn_* - and the runtime dump's zz_ placeholders in
 *   .pi/notes/dumpmap-join.json; the .rodata/.data runs carry no source-file string)
 *
 * Object: `_ENEMY_WORK` (enemy.h, the union of every consumer's copy, size 0xB1C; the run's
 * mangled-undefined `em_*__FP11_ENEMY_WORK*` arguments name it, and the split's extabindex records
 * place all 34 exception entries inside this range). Every field this file names already carries an
 * offset and a name in that header, so nothing is redefined here (rule 1).
 *
 * Status.  7 of the 46 symbols are byte-identical and one more is above the bar (8/46 >= 80 %):
 *   `fn_80173264`, `fn_8017326C`, `fn_80173278`, `fn_80173280`, `fn_801740A0`, `fn_80176328`,
 *   `fn_80176374` 100.000 %; `fn_801740F4` 99.944 % (above the bar).  The other 38 are unwritten (0 %).
 *   This first batch is the `action_0x1E5`/`state_sub` dispatch group, which needs no header change.
 *
 * Residuals and what the next round needs (all measured against
 * build/RMHE08/obj/enemy/fn_80171194.o, `report generate` fuzzy_match_percent):
 *   - `fn_801740F4` 99.944 %.  Every case body, the jump table and its order match; the one difference is
 *     the range test - retail tests `cmplwi r0,13` where this source's 8 dense cases give `cmplwi r0,7`
 *     (cases 8..13 are the default block).  A `switch` whose expression has a 14-value enum/type range is
 *     the shape that produces the 13 bound (MWCC bounds by the type's range, not the written cases);
 *     naming that enum needs 14 state ids the code does not name, so it is recorded rather than guessed.
 *   - **Header gap (the big one).**  The 38 unwritten functions touch `_ENEMY_WORK` bytes the shared
 *     header still calls `pad_*`, so they cannot be written under rule 5 without naming them (and
 *     `enemy.h` is read-only here): `+0x1E4` (read as a byte by `fn_80176AA8`, next to
 *     `action_0x1E5`), `+0x32C..+0x333` (`fn_80176C30` writes `+0x328` as a float, then five bytes and a
 *     u16; the header's `+0x320 VEC3`/`+0x32C u16` do not fit the byte stores), `+0x328` again in
 *     `fn_801762A0` (a `sth`, so the header's `VEC3 v_0x320` is the wrong type there), and the `+0x9AC`
 *     effect queue the header already describes as `_ENEMY_QUEUE_ENTRY`.  The outbox carries the list as a
 *     `shared-file` config_request.
 *   - **Unsplit callees.**  The range calls 10 functions outside it that no shared header declares
 *     (`fn_80170A00`, `fn_80171130`, `em_frame_flag_set`, `em_busy_set`, `eft_spawn_type10`, `VEC3_ctor`,
 *     `fn_8011E6EC`, `fn_803B9BA0`, `em_parts_damage_level_get`, `fn_8012EC74`); all but the library ones
 *     are enemy-band and belong in `unsplit/enemy.h` (its own docstring says so).  That header is
 *     read-only here, so the declarations sit at the top of this file and the outbox carries them as a
 *     `shared-file` config_request.
 *   - **`.ctors`.**  dtk's own split added `.ctors 0x8056F334-0x8056F338` to the registration, so the TU
 *     has a static constructor; the static object it initialises is not reconstructed yet (it is part of
 *     the same pass as the `.rodata`/`.data` runs tudiscover lists, which are deliberately left to the
 *     auto data objects for now).
 */

#include "enemy/fn_80128A8C.h" /* fn_80128A8C (rule 2: the owner's header) */
#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80171194.h"
#include "unsplit/enemy.h"
#include "enemy/enemy_control.h" /* em_spawn_request (the owner's header, rule 2) */
#include "unsplit/ef.h"
#include "ef/fn_80105314.h"
#include "ef/eft_slot.h"     /* enemy_data_find / enemy_data_grp (their owner's header) */
#include "ef.h"
#include "enemy/EnemyData.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_80128A8C_c1 ((void (*)(_ENEMY_WORK *, u8, u8))fn_80128A8C)

extern "C" {
/* The record `enemy_data_find` looks up; only the flag byte at +0x08 is read here.
 * size: 0x09 (approximate - only +0x08 is observed) */
typedef struct ENEMY_ENTRY {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ u8 field_0x08;
} ENEMY_ENTRY;

/* ---- foreign callees the owner's header does not declare yet ---- */

void fn_801706B8(_ENEMY_WORK *self);
}

/* The one mangled callee, declared by its owner's real name so the C++ front-end reproduces the map's
 * `em_frame_check__FP11_ENEMY_WORKUsff`.  `unsplit/enemy.h` carries the C spelling of the same
 * symbol for the units that stay C (rule 9: a mangled spelling is never the callable identifier). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

/* The shared `.sdata2` pool entries this unit loads (declared only, never defined here). */
extern f32 lbl_80797910; /* 0.0f */
extern f32 lbl_80797928; /* 170.0f */

/* `em_parts_damage_level_get` is the one callee the map spells *mangled*, so it is declared as a C++
 * free function - outside the `extern "C"` block below - and the front-end then emits exactly the map's
 * `em_parts_damage_level_get__FP11_ENEMY_WORKUc` (rule 9).  It is the enemy-band damage-part query
 * `src/ef/eft009.cpp` also declares. */
u8 em_parts_damage_level_get(_ENEMY_WORK* enemy, u8 part);

extern "C" {
void fn_80171194(_ENEMY_WORK* self, u8 kind, s32 arg);
void fn_80171744(_ENEMY_WORK* self, s32 arg);
void fn_80171EB8(_ENEMY_WORK* self, s32 arg);
void fn_801721B8(_ENEMY_WORK* self);
void fn_80172E10(_ENEMY_WORK* self);
void fn_80172E98(_ENEMY_WORK* self);
void fn_801731BC(_ENEMY_WORK* self);
void fn_80173264(_ENEMY_WORK* self);
void fn_8017326C(_ENEMY_WORK* self);
void fn_80173278(_ENEMY_WORK* self);
void fn_80173280(_ENEMY_WORK* self);
void fn_801732B0(_ENEMY_WORK* self);
void fn_8017394C(_ENEMY_WORK* self);
void fn_80173A04(_ENEMY_WORK* self);
void fn_80173C60(_ENEMY_WORK* self);
void fn_80173D04(_ENEMY_WORK* self);
void fn_80173FF4(_ENEMY_WORK* self);
void fn_801740A0(_ENEMY_WORK* self);

/* ------------------------------------------------------------------------------------------------ *
 * Callees outside this range.  They belong in `unsplit/enemy.h`; that header is read-only for
 * this round, so the declarations sit here with the parameter widths the call sites show.  The
 * functions at 0x80170A00/0x80171130 are the `action_0x1E5` handlers that live just below this range.
 * ------------------------------------------------------------------------------------------------ */
void fn_80170A00(_ENEMY_WORK* self);
void fn_80171130(_ENEMY_WORK* self);
void fn_803B9BA0(_ENEMY_WORK* self, VEC3* pos, s32 value);
}

#pragma peephole off

extern "C" {
/* `enemy_data_grp`/`enemy_data_find` (0x803439D4 / 0x803438E4) come from their owner's header,
 * `ef/eft_slot.h` (rule 2). */

/* ---- the range's functions ---- */
void fn_80170600(_ENEMY_WORK *self) {
    self->field_0x32A = 0;
    self->field_0x328 = 0;
}

void fn_80170610(_ENEMY_WORK *self, u8 a) {
    Vec3 v;
    u8 b, c;

    VEC3_ctor(&v);
    if ((a & 0xFF) == 2) {
        fn_80176090(self, &b, &c);
        fn_80128A8C_c1(self, b, c);
        em_state_refresh(self);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, 0.0f, 45.0f);
        fn_801057A4(self, 0x14, &v, 0.7f, 0x1C72);
    }
}

void fn_801706B8(_ENEMY_WORK *self) {
    if (self->team == 0xC) {
        em_spawn_request(self->field_0x01A, 0xA, 0, self->area_no, self->field_0x46C, 1, 2, 8,
                    0xFF, 0, 0);
    } else {
        em_spawn_request(self->field_0x01A, 0xD, 0, self->area_no, self->field_0x46C, 1, 2, 8,
                    0xFF, 0, 0);
    }
}

void fn_80170758(_ENEMY_WORK *self, u8 a, u8 b) {
    if ((a & 0xFF) == 1) {
        switch (b) {
        case 5:
            fn_80130F74(self);
            break;
        case 0xA:
            self->field_0x32A = 0;
            break;
        case 0xC:
            if (em_busy_ck(self) == 1) {
                fn_801706B8(self);
            }
            break;
        case 0xD:
            if (em_busy_ck(self) == 1) {
                fn_801706B8(self);
            }
            break;
        case 0xE:
            fn_801376B4(self);
            break;
        }
    }
}

void fn_80170804(_ENEMY_WORK *self) {
    if (self->field_0x328 > 0) {
        self->field_0x328--;
    }
    if (em_die_ck(self) == 0) {
        u8 v = (u8)enemy_data_grp(self->team, self->field_0x00A);
        ENEMY_ENTRY *entry = (ENEMY_ENTRY *)enemy_data_find(v, self->field_0x46C);
        if (entry != 0 && entry->field_0x08 == 0xFF) {
            self->field_0x46C = 0xFF;
            fn_8013AAC4(self);
        }
    }
}

void fn_8017088C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170908(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170984(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170A00(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8017088C(self);
        break;
    case 1:
        fn_80170908(self);
        break;
    case 2:
        fn_80170984(self);
        break;
    case 3:
        fn_8017088C(self);
        break;
    case 4:
        fn_8017088C(self);
        break;
    case 6:
        fn_8017088C(self);
        break;
    }
}

void fn_80170A54(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170AD0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170B4C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2: {
        f32 t = 0.4f;
        fn_8013221C(self, t, 1, 0xF);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x71, 4, 0);
            fn_80132264(self);
        }
    }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170C68(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        self->timer_0x020 = 0x84;
        break;
    case 1:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        } else {
            self->state++;
            em_state_set(self, 1, 5);
        }
        break;
    }
}

void fn_80170D04(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set(self, 0xCA, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170D74(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170DF0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 138.0f, 0.0f) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170E78(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x71, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170EF4(_ENEMY_WORK *self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        break;
    case 1:
        if ((u8)a == 1) {
            if (em_frame_check(self, 1, 110.0f, 0.0f) == 1) {
                em_state_set(self, 1, 0xC);
            }
        } else {
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
            }
        }
        break;
    }
}
}

#pragma peephole on

/* state 0: advance and post action 0xC9.  state 1: wait for the frame check, then fire action 0xD. */
extern "C" void fn_80170FA8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80797928, lbl_80797910) == 1U) {
            em_state_set(self, 1, 0xD);
        }
        break;
    }
}

/* state 0: advance and post action 0xC9.  state 1: advance the move when the frame check passes. */
extern "C" void fn_80171038(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* state 0: advance and post action 0xE.  state 1: advance the move when the frame check passes. */
extern "C" void fn_801710B4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* The action-code dispatcher: `state_sub` 0..14, codes 0 and 9 return without a handler. */
extern "C" void fn_80171130(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1: fn_80170A54(self); break;
    case 2: fn_80170AD0(self); break;
    case 3: fn_80170B4C(self); break;
    case 4: fn_80170C68(self); break;
    case 5: fn_80170D04(self); break;
    case 6: fn_80170D74(self); break;
    case 7: fn_80170DF0(self); break;
    case 8: fn_80170E78(self); break;
    case 10: fn_80170EF4(self, 1); break;
    case 11: fn_80170EF4(self, 0); break;
    case 12: fn_80170FA8(self); break;
    case 13: fn_80171038(self); break;
    case 14: fn_801710B4(self); break;
    case 0:
    case 9: break;
    }
}

#pragma peephole off

extern "C" {
/* ------------------------------------------------------------------------------------------------ *
 * The `state_sub` (+0x1E6) and `action_0x1E5` (+0x1E5) dispatchers.
 *
 * `fn_801740F4` is the outer one: it switches on `action_0x1E5`, and the entries 0 and 1 are the two
 * functions immediately below this range (0x80170A00/0x80171130), i.e. the tail of the same dispatch
 * table continues into the previous TU - which is why its left boundary is a "strong" cut.
 * ------------------------------------------------------------------------------------------------ */
void fn_80173264(_ENEMY_WORK* self) {
    fn_80171744(self, 2);
}

void fn_8017326C(_ENEMY_WORK* self) {
    fn_80171194(self, 10, 1);
}

void fn_80173278(_ENEMY_WORK* self) {
    fn_80171EB8(self, 0);
}

void fn_80173280(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80173264(self);
        break;
    case 1:
        fn_8017326C(self);
        break;
    case 2:
        fn_80173278(self);
        break;
    }
}

void fn_801740A0(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801732B0(self);
        break;
    case 1:
        fn_8017394C(self);
        break;
    case 2:
        fn_80173A04(self);
        break;
    case 3:
        fn_80173C60(self);
        break;
    case 4:
        fn_80173D04(self);
        break;
    case 5:
        fn_80173FF4(self);
        break;
    }
}

void fn_801740F4(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_80170A00(self);
        break;
    case 1:
        fn_80171130(self);
        break;
    case 2:
        fn_801721B8(self);
        break;
    case 3:
        fn_80172E10(self);
        break;
    case 4:
        fn_80172E98(self);
        break;
    case 5:
        fn_801731BC(self);
        break;
    case 6:
        fn_80173280(self);
        break;
    case 7:
        fn_801740A0(self);
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * The damage-part gates at the tail of the range.  Both read `team` (+0x003) and `pos` (+0x188,
 * `VEC3`) from the shared layout and ask `em_parts_damage_level_get` about a part.
 * ------------------------------------------------------------------------------------------------ */
int fn_80176328(_ENEMY_WORK* self, u32 arg) {
    if ((u8)arg == 1 && ((u8)em_parts_damage_level_get(self, 1) & 1) == 0) {
        return 1;
    }
    return 0;
}

void fn_80176374(_ENEMY_WORK* self, u32 arg) {
    if ((u8)arg == 0 && self->team == 14 && em_parts_damage_level_get(self, 0) == 1) {
        fn_803B9BA0(self, &self->pos, 100);
    }
}
}

/* This unit's own `.bss` (`splits.txt` `.bss 0x806A79A0..0x806A79D0`), in address order: the 2 two-vector record(s)
 * its static constructor `fn_80176B7C` builds (`.data` tables point at them).  Names are GUESSes: each record is a
 * pair of model-space points. */
VEC3 vec_pair_80171194_0[2];  /* +0x806A79A0 */
VEC3 vec_pair_80171194_1[2];  /* +0x806A79B8 */
