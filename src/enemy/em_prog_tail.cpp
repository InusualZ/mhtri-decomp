/*
 * enemy/em_prog_tail.cpp - unit, `.text` 0x80385EE0..0x803868DC (14 functions, 2556 bytes).
 *
 * 11 of 14 functions have a body here.
 *
 * FLAGS.  `cflags_main`.  GUESS (rule 7): the stem names the program tail of the note-pane band; its helpers are
 * declared by `enemy/em_prog_support.h`.
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.sdata2, .text, extab, extabindex).
 */
/* ---- header inherited from src/enemy/fn_80382310.cpp (written against its pre-phase-4 range) ---- */
/* enemy/fn_80382310.cpp - the enemy `em009`/`em019` program band's shared support block, `.text`
 * 0x80382310..0x803868DC (the tail 0x803868DC..0x80387844 moved to `enemy/em009_act.cpp` in the
 * 2026-09-30 recut; the range still holds more than one TU, see SEAM).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80382310` -> `zz_0382310_`; the map's own rows for the
 * range in config/RMHE08/symbols.txt carry no real name either), and no `__FILE__` string is
 * referenced by any body - every `lis`/`addi` and every `@sda21` relocation in the range resolves to
 * the `.sdata2` float pool, a switch/jumptable or one of the band's own record tables, never to a
 * source-file-name literal (checked by reading all `R_PPC_*` relocations of the range's 118 split
 * objects and cstring-ing each referenced `data:string` label in orig/RMHE08/sys/main.dol: of the 302
 * bare `<name>.c/.cpp/.h` string labels in the image, none is referenced from 0x80382310..0x80387844).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string (above).  2. `dumpmap.py
 * lookup` answers only `zz_XXXXXXXX_` placeholders.  3. The code places the unit in `enemy`: the
 * immediately following registered unit is `enemy/em009_act.cpp` (0x80387844, the monster-AI action
 * band), the immediately preceding `.data` is the enemy program-table block (`em019_prog_tbl`
 * at 0x805EE518, its table run 0x805EE584..0x805EE5B0 that `fn_80382310` indexes, `em009_prog_tbl` at
 * 0x805EF990, and the `jumptable_805EF4F4`/`jumptable_805EF52C` switch tables of the same band), and
 * every body drives the shared `_ENEMY_WORK` record through `em_frame_check__FP11_ENEMY_WORKUsff`,
 * `em_act_ck__FP11_ENEMY_WORKUcUc`, `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`,
 * `get_em_chg_scale__FP11_ENEMY_WORK` and `em_magma_check`.  The file therefore keeps the map's
 * `fn_80382310` stem (brief option 4); no name was invented and no module was guessed.
 *
 * LANGUAGE AND SECTIONS.  C++ (langcheck: the range defines one mangled function,
 * `qn_get_motion_no__FP7_QNPC_W`, and reaches genuinely mangled callees - `MHchar::getTevKColor`/
 * `setTevKColor`, `setVector3__FPQ34nw4r4math4VEC3fff`, `get_joint_wpos_em__FP11_ENEMY_WORK...` -
 * through their real signatures, rule 9).  Every plain `fn_` definition is `extern "C"` so it keeps
 * the map's name (playbook 42).  Sections claimed: `.text` 0x80382310..0x803868DC, extab
 * 0x80017E2C..0x8001803C, extabindex 0x80037BA8..0x80037EC0, `.ctors` 0x8056F3A8..0x8056F3B0, `.data`
 * 0x805EF8C0..0x805EF990, `.bss` 0x806C23E8..0x806C5488 and `.sbss` 0x80794C00..0x80794C08.  The band's
 * `.sdata2` tables live in other splits and are referenced here as the map's `lbl_`/`jumptable_`
 * symbols, never re-emitted (rule 10 / rule 2).
 *
 * SEAM.  The right edge 0x803868DC starts the `em009` TU (`enemy/em009_act.cpp`).  The range is still
 * more than one TU, not split yet: the two `.ctors` words (`fn_8038309C`, `fn_80385E7C`) are two TUs'
 * static initializers (the first TU ends at 0x80383148; `fn_80385E7C` constructs the `lbl_806C4A88`
 * array with `fn_80385E9C`, emitted after it), and the pool repeats 41c00000 at `lbl_8079BFAC` (first
 * read by `fn_803865B4`), so a third TU starts in 0x80385E7C..0x80386028 (taken as
 * 0x80385EE0..0x803868DC: GUESS).  The left edge 0x80382310 is discovery's cap, not a proven boundary -
 * `tudiscover.py at 0x80382310` extends the range left to 0x8037F940 on its strong cuts.
 *
 * RECONSTRUCTION STATUS (measured with `tools/units/recompile.py enemy/fn_80382310 --measure <sym>`,
 * the official report metric, against MAIN's retired split objects `auto_fn_*_text.o`).  72 of the
 * range's 118 functions have a body (5068 of 21812 `.text` bytes, 23.23 %); 34 are byte-identical and
 * 61 measure >= 80 %.  The unit is short of the 80 % bar: the 72 bodies written this session are the
 * band's mechanical half - the colour/slot accessors, the note-pane allocator/constructor/destructor
 * set, the eleven sub-state machines and the tail-call dispatchers - and the band's byte mass sits in
 * the large bodies not attempted yet (fn_80384434 0x700, fn_80386C9C 0x600, fn_80382310 0x46C,
 * fn_803865B4 0x328, fn_803828B8 0x2F8, fn_80386A04 0x298, fn_80387620 0x224, ...) - the 46
 * functions without a body cover 16744 bytes.
 *
 * RESIDUALS (what still differs and why):
 *   - **`fn_80384ECC` (14.5 %).**  The retail body dispatches through the 17-entry jump table
 *     `jumptable_805EF94C`; the conformant `switch (a) { case 0..3 }` spells the same predicate but MWCC
 *     emits a compare chain here (104 B vs 124 B).  The jump-table shape needs an explicit case per
 *     index; recorded, not forced.
 *   - **`fn_803851D4` (48.6 %).**  The five-slot allocator is written from the target's unrolled shape
 *     but indexes `lbl_806C4A88[i]` (156 B vs 152 B): the retail body walks a pointer by +0x1F8 instead.
 *   - **`note_pane_motion_end_ck`/`fn_80385C70`/`fn_80385C80` (60/45/45 %).**  The three MHchar tail-call thunks
 *     differ in the argument-narrowing the compiler inserts before the `b` (12 B vs the target's 16 B
 *     for the two 3-argument forms).
 *   - **`note_pane_set_anim_pair` (60 %).**  The body is right but MWCC knows the u8 parameters are already narrow
 *     and drops the two `clrlwi` the retail object carries (12 B vs 20 B).
 *   - **`fn_80384B34` (74.8 %), `fn_803857BC` (76.8 %), `fn_803852B8` (79.3 %), `fn_80382C00` (73.4 %),
 *     `fn_80382F94` (90.9 %).**  Sign/narrowing and load-order residuals inside otherwise-correct bodies.
 *
 * The per-symbol table is in the outbox .pi/outbox/80382310-fn-80382310-2982.json and the batch note
 * .pi/notes/80382310-fn-80382310-2982.md.
 */

#include "types.h"
#include "enemy/em_prog_support.h"
#include "nw4r/math.h"
#include "sound/mhchar.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_8012E968.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "sound/fn_800DD1F0.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/note_work.h" /* NoteWork and the pane/slot/layout types (rule 1) */
#include "lobby/lb_quest_screen.h" /* note_pane_get_motion (rule 2: the owner's header) */
#include "ef/pRoot.h"
#include "lobby/lb_server_sel_trans.h" /* fn_803C7EAC / fn_803C7F88 (owner's header, rule 2) */
#include "Runtime.PPCEABI.H/CPlusLibPPC.h" /* __construct_array (owner's header, rule 2) */

extern "C" {

struct _QNPC_W;
}

/* The note band's own still-unwritten members, declared with the argument view their call sites use
 * (the record type is `NoteWork` unless the signature says otherwise). */

extern "C" {
void fn_80386028(NoteWork* self);
void fn_803860B8(NoteWork* self);
void fn_8038613C(NoteWork* self);
void fn_80386160(NoteWork* self);
void fn_803861F8(NoteWork* self);
void fn_80386328(NoteWork* self);
void fn_803863C8(NoteWork* self);
void fn_80386484(NoteWork* self);
void fn_803864C0(NoteWork* self);
void fn_80385EE0(NoteWork* self);
}

/* Callees whose map names are C++ manglings (rule 9: declare the owner's real signature, never the
 * mangled spelling). */
struct _PLW;
void* get_move_work_adrs(u8 kind);
s32 Pl_act_ck(_PLW* plw, u8 a, u16 b);

extern "C" {
void eft_rot_vec_copy(void* dst, void* src);
extern f32 lbl_8079BF94;
extern f32 lbl_8079BF90;
}

/* 0x80385EE0 */
extern "C" void fn_80385EE0(NoteWork* self) {
    note_pane_set_anim_pair(self, 0, 0);
}

/* 0x80385EEC */
extern "C" void fn_80385EEC(NoteWork* self) {
    self->field_0x001 = 0;
    fn_80385EE0(self);
}

/* 0x8038613C */
extern "C" void fn_8038613C(NoteWork* self) {
    switch (self->field_0x19F) {
    case 0:
        fn_80386028(self);
        return;
    case 1:
        fn_803860B8(self);
        return;
    }
}

/* 0x80386484 */
extern "C" void fn_80386484(NoteWork* self) {
    switch (self->field_0x19F) {
    case 0:
        fn_80386160(self);
        return;
    case 1:
        fn_803861F8(self);
        return;
    case 2:
        fn_80386328(self);
        return;
    case 3:
        fn_803863C8(self);
        return;
    }
}

/* 0x803864C0 */
extern "C" void fn_803864C0(NoteWork* self) {
    switch (self->field_0x19D) {
    case 0:
        fn_8038613C(self);
        return;
    case 1:
        fn_80386484(self);
        return;
    }
}

/* 0x80386028 - action sub-machine 0. */
extern "C" void fn_80386028(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 1, 0, 0);
        self->field_0x16C = 200;
        self->field_0x1B0 = lbl_8079BF90;
    } else if (v == 1) {
        if (--self->field_0x16C < 0) {
            fn_80385EE0(self);
        }
    }
}

/* 0x803860B8 - action sub-machine 1. */
extern "C" void fn_803860B8(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 3, 0, 0);
    } else if (v == 1) {
        if (note_pane_motion_end_ck(self) == 1) {
            note_pane_set_anim_pair(self, 1, 2);
        }
    }
}

/* 0x80386160 - action sub-machine 2. */
extern "C" void fn_80386160(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 0, 0);
        self->field_0x16C = 600;
    } else if (v == 1) {
        self->field_0x18C = (u16)(self->field_0x18C + 112);
        if (--self->field_0x16C <= 0) {
            fn_80385EE0(self);
        }
    }
}

/* 0x803861F8 - action sub-machine 3. */
extern "C" void fn_803861F8(NoteWork* self) {
    u8* work = (u8*)get_move_work_adrs(2) + self->field_0x198 * 2848;
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 0, 0);
        self->field_0x1B0 = lbl_8079BF90;
        copyVec3(&self->vec_0x170, (nw4r::math::VEC3*)(work + 0x3C));
        eft_rot_vec_copy(&self->field_0x188, work + 0x54);
        qn_chr_flag_set((_QNPC_W*)self, 90);
    } else if (v == 1) {
        if (Pl_act_ck((_PLW*)work, 9, 0) == 1) {
            copyVec3(&self->vec_0x170, (nw4r::math::VEC3*)(work + 0x3C));
            eft_rot_vec_copy(&self->field_0x188, work + 0x54);
            qn_chr_flag_set((_QNPC_W*)self, 90);
        } else {
            if (Pl_act_ck((_PLW*)work, 9, 1) == 1) {
                note_pane_set_anim_pair(self, 0, 1);
            } else {
                note_pane_set_anim_pair(self, 1, 2);
            }
        }
    }
}

/* 0x80386328 - action sub-machine 4. */
extern "C" void fn_80386328(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 4, 0);
        self->field_0x16C = 53;
    } else if (v == 1) {
        self->field_0x18C = (u16)(self->field_0x18C + 688);
        if (--self->field_0x16C <= 0) {
            note_pane_set_anim_pair(self, 1, 3);
        }
    }
}

/* 0x803863C8 - action sub-machine 5. */
extern "C" void fn_803863C8(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 4, 0);
        self->field_0x16C = 100;
    } else if (v == 1) {
        if (--self->field_0x16C <= 0) {
            fn_80385EE0(self);
        }
        if (self->field_0x16C < 5) {
            self->field_0x1B0 = (f32)self->field_0x16C / lbl_8079BF94;
        }
    }
}

