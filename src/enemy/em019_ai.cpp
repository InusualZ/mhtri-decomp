/*
 * enemy/em019_ai.cpp - phase 4 unit, `.text` 0x80378F9C..0x80383148 (92 functions, 41388 bytes).
 *
 * PHASE 4 (docs/splits/phase4, window d).  Fold of 4 registered units: em019_ai.cpp, em019_prog.cpp, em_act_mot.cpp,
 * fn_80382310.cpp.  The functions below are the ones those sources define, in address order; every other function of
 * the range keeps its original bytes.  28 of 92 functions have a body here.
 *
 * FLAGS.  `cflags_main` (all four absorbed sources).  The rest of the old fn_802823xx band is
 * `enemy/em_prog_support.cpp` and `enemy/em_prog_tail.cpp`.
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .ctors, .data, .rodata, .sbss, .sdata, .sdata2, .text,
 * extab, extabindex).
 */
/* ---- header inherited from src/enemy/em019_ai.cpp (written against its pre-phase-4 range) ---- */
/*
 * enemy/em019_ai.cpp - the em019 monster-AI file's body, `.text` 0x80378F9C..0x8037EA64
 * (61 functions / 0x5AC8 B) with extab 0x80017C44..0x80017DBC (47 records) and extabindex
 * 0x800378CC..0x80037B00 (47 x 12 B).
 *
 * WHAT IT IS.  Monster-AI code of the em019 program, continued from `enemy/em020_ai.cpp`: every body
 * takes the shared `_ENEMY_WORK` record (`include/enemy/ENEMY_WORK.h`) and drives it through the
 * enemy core API (`em_frame_check`, `get_joint_wpos_em`, `get_em_chg_scale`, `em_mot_set` /
 * `em_mot_set_ck` / `em_mot_end_ck`) and the game's work blocks (`system_w`, `lobby_w`,
 * `get_move_work_adrs`, `my_player_no`, `Psw`).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable from the
 * range (same check as `enemy/em020_ai.cpp`: every relocation target resolves to a program table, a
 * work block, the shared `.sdata2` float pool or a call).  2. `dumpmap.py lookup` answers
 * `zz_<addr>_` everywhere except `em019_prog_tbl`.  3. The module is `enemy` and the file `em019` from
 * the `.data` program table `em019_prog_tbl` (0x805EE518, `scope:global`, size 0x6C), whose
 * entry-point list is this band's own functions (`fn_80379090` +0x00, `fn_80379124` +0x04,
 * `fn_8037946C` +0x08, `fn_8037924C` +0x10, `fn_8037939C` +0x14, `fn_8037F524` +0x24) plus the next
 * file's head (`fn_8037F940`, `fn_80382310`, ... - the same cross-file pattern `em024_prog_tbl`
 * shows for `eft052`), and from the code (`_ENEMY_WORK` field for field, `em_*` callees only).  The
 * name follows the module's `em*` scheme (`em024_ai.cpp`, `em035_prog.cpp`).
 *
 * SEAM (unproven - the range is an `attribute.py` `--max-bytes` run, not a TU boundary).  Left edge:
 * `fn_80378F7C` (the last function of `enemy/em020_ai.cpp`) is called only from 0x8036C284/0x8036C6E8
 * (em020 side) while `fn_80378F9C` (the first function here) is called only from 0x8037939C upward,
 * and the `.data` block boundary is 0x805EE518 - `em019_prog_tbl`, the first `.data` of this file's
 * block.  Right edge: `tudiscover.py at 0x8037E0E8` reports the 48-function match set
 * 0x80379694..0x8037EA64 with a strong seam at 0x8037F940 (`.data` jumptable_805EF4F4 ->
 * jumptable_805EF52C; `.sdata2` lbl_8079BE88 -> lbl_8079BE8C), so this file's real extent is
 * 0x80378F9C..0x8037F940; the brief's 0x8037EA64 cut is filed as a `range` config_request and the
 * tail (0x8037EA64..0x8037F940, 12 functions) is left to its own lane.
 *
 * Naming note: the names this file *references* in other units are still the map's generated
 * `fn_XXXXXXXX` stems (the enemy core band 0x8012xxxx/0x8013xxxx, the lobby 0x802xxxxx band and the
 * game-root 0x8042xxxx band, checked with `tools/symbols/symedit.py range`), and so are the
 * addresses of its own range that are not reconstructed yet - they are *declared*, never defined, so
 * the map row still names the target.  Every symbol this file DEFINES is named from its own body and
 * renamed in the map with `symedit.py rename`.
 *
 * Sections this unit claims: `.text` 0x80378F9C..0x8037EA64, extab 0x80017C44..0x80017DBC,
 * extabindex 0x800378CC..0x80037B00.
 *
 * Residuals: the range is registered as `NonMatching`; the bodies still unwritten keep the map's
 * `fn_XXXXXXXX` names, and the ones written but not yet byte-identical are listed in the outbox with
 * their first divergence.  Re-measure with `ninja build/RMHE08/report.json` +
 * `python tools/objdiff/symdiff.py -u enemy/em019_ai.cpp <symbol>`.
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
#include "enemy/ENEMY_WORK.h"

#ifdef __cplusplus

extern "C" {
#endif

/* The addresses of this unit's range that are not reconstructed yet (Naming note above).  They
 * are declared, never defined: the map row still names the target object's symbol. */
void fn_8037983C(struct _ENEMY_WORK* self);
void fn_803799A8(struct _ENEMY_WORK* self);
void fn_80379AC4(struct _ENEMY_WORK* self);
void fn_80379B50(struct _ENEMY_WORK* self);
void fn_80379BD0(struct _ENEMY_WORK* self);
void fn_80379C60(struct _ENEMY_WORK* self);
void fn_80379CDC(struct _ENEMY_WORK* self);
void fn_80379D64(struct _ENEMY_WORK* self);
void fn_8037A848(struct _ENEMY_WORK* self, u32 a);
void fn_8037A964(struct _ENEMY_WORK* self);
void fn_8037AAAC(void);
void fn_8037ACA8(struct _ENEMY_WORK* self, u32 a);
void fn_8037ADE4(struct _ENEMY_WORK* self, u32 a);
void fn_8037A508(struct _ENEMY_WORK* self, u32 a);
void fn_8037A600(struct _ENEMY_WORK* self, u32 a);
void fn_8037A718(struct _ENEMY_WORK* self);
void fn_803794F8(struct _ENEMY_WORK* self);
void fn_80379594(struct _ENEMY_WORK* self);
void fn_80379614(struct _ENEMY_WORK* self);
void fn_80379694(struct _ENEMY_WORK* self);
void fn_8037975C(struct _ENEMY_WORK* self);
}
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
 * (`em_mot_end_ck`, `em_frame_check`).  `em_parts_damage_ck` (0x8037F624) steps the seven part damage
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
 * Naming note: references only to other units' unrenamed `fn_XXXXXXXX` symbols - this file names
 * every symbol it *defines* (the 12 definitions below); what remains are callees whose owning band is
 * still a `fn_` row in the map (`em_move_mode_set`, `em_mot_set`, `em_action_finish` and ~40 more, plus the
 * em019 band's `fn_8037E0E8`..`em019_motion_dispatch`), which the pass that writes those bands owns
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
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/EM_PART_BLOCK.h"
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

/* This band's call of the +0x2C job-injection slot of `ShellSetFuncs` (`stage/shell_set_func_ptr.h`): the
 * target passes the work record, two ids, a VEC3, a scale and the +0xAEA flag word and no table argument,
 * which the shared 7-argument slot type cannot express (passing the table hoists its load, em_act_prog_5
 * 100 -> 97.83), so the call goes through this 6-argument spelling of the same slot. */
typedef void (*ShellJobInjectFn)(struct _ENEMY_WORK* self, s32 a, s32 b, nw4r::math::VEC3* pos, f32 scale,
                                 u16 flags);
/* The `.rodata` motion table `em_turn_seq_start`/`em_turn_seq_step` walk (0x80570AE0..0x80570B20, 64 B). */
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

/* `include/stage/fn_802B2AA0.h` (this unit's `shell_set_func_ptr` owner) declares `setVec3` with
 * a `void` result, while the target's call site consumes the returned pointer; the band reaches it
 * through the signature its own body has.  `em035_prog.cpp` records the same gap, and the outbox
 * carries the shared-file request. */
typedef nw4r::math::VEC3* (*Fn80041E8C)(nw4r::math::VEC3* out, f32 x, f32 y, f32 z);

/* The out-of-range callees whose owning band is still a `fn_` row in the map - each spelling is the
 * callee's own body (the same gap the sibling bands `enemy/em_act_step.cpp` and
 * `enemy/em_action.cpp` record).  The `fn_8037...`/`fn_80378...` group is the em019 band's own
 * functions, which `enemy/em019_ai.cpp` (the rest of this file) owns. */

extern "C" {
u8 fn_80378F9C(struct _ENEMY_WORK* self, u32 a);
u8 fn_8037900C(struct _ENEMY_WORK* self, u32 a);
void em019_motion_dispatch(struct _ENEMY_WORK* self);
void em019_state_dispatch(struct _ENEMY_WORK* self);
void fn_8037A470(struct _ENEMY_WORK* self);
void em019_approach_dispatch(struct _ENEMY_WORK* self);
void em019_battle_dispatch(struct _ENEMY_WORK* self);
void fn_8037DAC8(struct _ENEMY_WORK* self);
void fn_8037DC04(struct _ENEMY_WORK* self);
void fn_8037DFF0(struct _ENEMY_WORK* self);
void em019_action_start_if_idle(struct _ENEMY_WORK* self);
void fn_8037E0E8(struct _ENEMY_WORK* self);
u32 fn_80382E48(struct _ENEMY_WORK* self, u32 a);
void fn_803B9BA0(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a);
}
/* Monster Hunter Tri (RMHE08) - the enemy per-motion stepper band 0x8037F940-0x80382310 (`.text`,
 * 4 functions, 10704 B), reconstructed from the split target object.
 *
 * WHAT IT IS.  `em_act_mot_step` (0x8037F940, 10076 B) is the per-motion-number stepper: it calls
 * the em019 band's `em_parts_damage_ck` and `em_act_effect_ck`, reads `em_get_mot_no(self)` and
 * switches 216 ways (`cmplwi r0, 215` over the 216-entry jump table `.data` 0x805EF52C) - one arm
 * per motion the enemy can play, each arming the motion's own state and handing its damage windows
 * on.  The other three are the part-material steppers the neighbouring band calls: `em_part_reset`
 * resets one part slot's colour/meter and takes its RGB from the shared id table, `em_part_colour_lerp`
 * lerps a slot's RGB toward a target by its alpha, and `em_part_damage_meter` steps a slot's 0..1
 * damage meter by 1/damage and arms/disarms the slot.
 *
 * MODULE AND NAME (brief section 2, evidence order).  Class 1, a `__FILE__` string: none reachable
 * (this band's `.data`/`.sdata2` runs carry no printable byte).  Class 2, the runtime dump:
 * `dumpmap.py lookup` answers `zz_XXXXXXXX_` for all four addresses.  Class 3 decides the module: the
 * band drives the shared `_ENEMY_WORK` record, its link neighbours are `enemy/*`, and its three
 * steppers are called from the registered `enemy/fn_80382310.cpp`; the file name `em_act_mot` and
 * every symbol here are **derived names (GUESS)** from the bodies, on the module's `em_<noun>_<verb>`
 * scheme (`em_action.cpp`, `em_act_step.cpp`).
 *
 * SEAM (re-drawn, not the brief's `--max-bytes` cut).  This unit is the half of
 * `proposal/8037EA64_fn_8037EA64.cpp` that the brief's byte budget had merged with the em019
 * program band below; the cut is 0x8037F940 and the evidence is in the sibling half's header
 * (`src/enemy/em019_prog.cpp`): `tudiscover.py at 0x8037E0E8` reports it as the strong seam (the
 * `.data` run jump `jumptable_805EF4F4` -> `jumptable_805EF52C` and the `.sdata2` run jump
 * `lbl_8079BE88` -> `lbl_8079BE8C`, each side's labels referenced only by its own functions), the
 * registered `enemy/em019_ai.cpp` records 0x8037F940 as its file's right edge, and the bracketing
 * extab runs tile - this unit takes 0x80017E14..0x80017E2C and the next registered unit
 * (`enemy/fn_80382310.cpp`) starts its own record run at 0x80017E2C.
 *
 * Naming note: references only to other units' unrenamed `fn_XXXXXXXX` symbols; every symbol this
 * file *defines* is named (the three definitions below).  `em_act_mot_step`'s own row is **not yet
 * defined** - the 216 arm bodies are a unit of their own - so its map name stays a target-only name
 * and its row measures 0 % (see RESIDUALS).  Every definition is `extern "C"` so objdiff pairs it by
 * the map's name (playbook 42).
 *
 * SECTIONS.  `.text` 0x8037F940..0x80382310, `extab` 0x80017E14..0x80017E2C (3 x 8 B: one
 * record per function that carries one, `em_act_mot_step` included), `extabindex`
 * 0x80037B84..0x80037BA8 (3 x 12 B).  The `.data` jump table run 0x805EF4D0..0x805EF52C and the
 * shared part-colour id table 0x805EE5A0 are **not claimed**: the id table is referenced from
 * `enemy/fn_80382310.cpp` too (playbook 58's sole-referencer condition fails) and a jump table claim
 * without its emitting switch is a `target-extra` row.
 *
 * RESIDUALS.  `em_act_mot_step` (10076 B, 94 % of this unit's `.text`) is unwritten: a 216-case
 * switch whose arms must be read out of the target one at a time, which is a lane of its own.  It is
 * the unit's one 0 % row.
 */
#include "types.h"
#include "gx.h"
#include "sound/mhchar.h"

/* The shared part-colour id table the steppers index by slot (`lwzx r4, r4, r0` over 0x805EE5A0,
 * `{3, 0, 1, 4}`).  Referenced from `enemy/fn_80382310.cpp` too, so it is declared, never defined
 * (playbook 29/58). */
extern "C" u32 lbl_805EE5A0[];

/* The band's pooled floats, declared never defined: they are shared with the neighbouring bands'
 * pools, so none is claimable (playbook 58). */
extern "C" f32 lbl_8079BC88;   /* 0.0 */

extern "C" {

/* The per-motion stepper of the enemy work record - 0x8037F940, 10076 B, NOT reconstructed: its 216
 * arms would have to be read out of the target one at a time.  See the file header's RESIDUALS. */
}
#include "enemy/fn_8012E968.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/note_work.h" /* NoteWork and the pane/slot/layout types (rule 1) */
#include "lobby/lb_quest_screen.h" /* note_pane_get_motion (rule 2: the owner's header) */
#include "ef/pRoot.h"
#include "lobby/lb_server_sel_trans.h" /* fn_803C7EAC / fn_803C7F88 (owner's header, rule 2) */
#include "Runtime.PPCEABI.H/CPlusLibPPC.h" /* __construct_array (owner's header, rule 2) */

extern "C" {
void em_move_mode_set(_ENEMY_WORK* self, u32 a);
}

extern "C" {

/* The em019 action start: the shared action runner's entry wrapper.
 * 0x8037E0D0 */
void em019_action_start(void)
{
    fn_8037AAAC();
}

/* The em019 action start's idle gate: runs the action only from sub-state 0.
 * 0x8037E0D4 */
void em019_action_start_if_idle(struct _ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        em019_action_start();
    }
}

/* Arms the action's "run" flag at `+0x358`.
 * 0x80379084 */
void em019_action_run_flag_set(struct _ENEMY_WORK* self)
{
    self->field_0x358 = 1;
}

/* The em019 per-motion step dispatcher: `+0x1E6` selects one of the eight step bodies, each of which
 * is tail-called.
 * 0x80379DE4 */
void em019_state_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037983C(self);
        return;
    case 1:
        fn_803799A8(self);
        return;
    case 2:
        fn_80379AC4(self);
        return;
    case 3:
        fn_80379B50(self);
        return;
    case 4:
        fn_80379BD0(self);
        return;
    case 5:
        fn_80379C60(self);
        return;
    case 6:
        fn_80379CDC(self);
        return;
    case 7:
        fn_80379D64(self);
        return;
    }
}

/* The em019 battle step dispatcher: `+0x1E6` selects one of the twelve battle step bodies.
 * 0x8037AF08 */
void em019_battle_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037A848(self, 0);
        return;
    case 1:
        fn_8037A964(self);
        return;
    case 2:
        fn_8037AAAC();
        return;
    case 3:
        fn_8037ACA8(self, 0);
        return;
    case 4:
        fn_8037ACA8(self, 1);
        return;
    case 5:
        fn_8037ADE4(self, 0);
        return;
    case 6:
        fn_8037ACA8(self, 2);
        return;
    case 7:
        fn_8037A848(self, 1);
        return;
    case 8:
        fn_8037ACA8(self, 3);
        return;
    case 9:
        fn_8037ADE4(self, 1);
        return;
    case 10:
        fn_8037ADE4(self, 2);
        return;
    case 11:
        fn_8037A848(self, 2);
        return;
    }
}

/* The em019 approach step dispatcher: `+0x1E6` picks the approach or the retreat body.
 * 0x8037A7F0 */
void em019_approach_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037A508(self, 0);
        break;
    case 1:
        fn_8037A600(self, 0);
        break;
    case 2:
        fn_8037A718(self);
        break;
    case 3:
        fn_8037A600(self, 1);
        break;
    case 4:
        fn_8037A508(self, 1);
        break;
    }
}

/* The em019 motion step dispatcher: `+0x1E6` picks one of the five motion bodies.
 * 0x803797F4 */
void em019_motion_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_803794F8(self);
        break;
    case 1:
        fn_80379594(self);
        break;
    case 2:
        fn_80379614(self);
        break;
    case 3:
        fn_80379694(self);
        break;
    case 7:
        fn_8037975C(self);
        break;
    }
}

#ifdef __cplusplus
}
#endif


#pragma peephole off

/* The enemy work's action-program channel 1: phase 0 clears the +0x1E4 byte, the whole per-part
 * block (+0x328..+0x357, four slots) and the +0x358/+0x35A pair, then arms motion 20 with the two
 * effect-position resets; phase 1 ends the program through `em_action_finish` once `em_mot_end_ck` reports
 * the motion done. */
extern "C" void em_act_prog_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
        s32 i;

        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 0, 0);
        em_demo_pos_set(self, lbl_8079BC88, lbl_8079BC88, lbl_8079BC88);
        em_demo_rot_set(self, lbl_8079BC88, lbl_8079BC88, lbl_8079BC88);
        em_demo_enable(self);
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
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
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
        em_move_mode_set(self, 0);
        em_mot_set(self, 201, 4, 0);
        fn_801303EC(self, lbl_8079BC88);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079BCEC, lbl_8079BC88) == 1)
            fn_80136D14(self);
        if (em_frame_check(self, 0, lbl_8079BCF0, lbl_8079BC88) == 1)
            em_hit_window_set(self, 0, 13, 10);
        if (em_frame_check(self, 0, lbl_8079BCF4, lbl_8079BC88) == 1)
            em_hit_window_set(self, 0, 24, 26);
        if (em_frame_check(self, 0, lbl_8079BCF8, lbl_8079BC88) == 1)
            em_hit_window_clear(self, 0);
        if (em_mot_end_ck(self) == 1) {
            em_move_mode_set(self, 4);
            fn_801303EC(self, fn_8013032C(self));
            em_state_set(self, 13, 3);
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
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, lbl_80570AE0, 0, 0, 0);
        em_mot_speed_set(self, lbl_8079BCE8);
        fn_801303EC(self, fn_8013032C(self));
        fn_80378F9C(self, 255);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570AE0) == 1)
            em_state_set(self, 13, 4);
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
        em_move_mode_set(self, 4);
        em_mot_set(self, 89, 0, 0);
        em_approach_start(self, lbl_8079BC88, 0);
        em_mot_speed_set(self, lbl_8079BCE8);
        fn_801303EC(self, fn_8013032C(self));
        fn_80378F9C(self, 255);
        break;
    case 1:
        if (em_approach_step(self, 0, 256) == 1)
            em_state_set(self, 13, 5);
        break;
    }
}

/* The enemy work's action-program channel 5: the two per-frame hooks run unconditionally, then
 * phase 0 arms motion 202, resets the +0x1CC sequence and the part 255 scale and opens two
 * `em_hit_window_set` hit windows; phase 1 sequences three `em_frame_check` windows, and once the motion
 * and `em_busy_ck` both report done it either lands the part 13/6 hit or ends the program. */
extern "C" void em_act_prog_5(_ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 202, 0, 0);
        fn_801303EC(self, lbl_8079BC88);
        fn_80378F9C(self, 255);
        fn_80136D14(self);
        em_hit_window_set(self, 0, 32, 8);
        em_hit_window_set(self, 1, 31, 24);
        fn_80131E00(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_8079BD04, lbl_8079BC88) == 1)
            fn_80136D14(self);
        if (em_frame_check(self, 2, lbl_8079BCA0, lbl_8079BC88) == 1)
            fn_80131E00(self);
        if (em_frame_check(self, 0, lbl_8079BCB4, lbl_8079BC88) == 1) {
            nw4r::math::VEC3 off;

            em_camera_req(self, -1, 5);
            copyVec3(&pos, ((Fn80041E8C)setVec3)(&off, lbl_8079BC88, lbl_8079BC88,
                                                        lbl_8079BD08));
            ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 9, 1, &pos, lbl_8079BC94, self->field_0xAEA);
        }
        if (em_mot_end_ck(self) == 1 && em_busy_ck(self) == 1) {
            if (fn_80382E48(self, 0) == 1)
                em_state_set(self, 13, 6);
            else
                em_action_finish(self);
        }
        break;
    }
}

/* The enemy work's action-program channel 6: the four phases drive the +0x36C target vector - phase
 * 0 measures the bearing to it, picks the +0x006 turn direction off the +0x1C0 heading, arms motion
 * 31 or 32 and builds the +0x310 offset vector and the +0x318 radius; phases 1 and 2 run the turn
 * and the `lbl_80570AE0` sequence, phase 3 hands the result on once `em_busy_ck` reports done. */
extern "C" void em_act_prog_6(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u16 angle;

        self->state++;
        em_move_mode_set(self, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle >= 0x8000)
            self->state_0x006 = 1;
        else
            self->state_0x006 = 0;
        switch (self->state_0x006) {
        case 0:
            em_mot_set(self, 31, 10, 0);
            if (angle < 0x2000)
                angle = 0x2000;
            else if (angle > 0x6000)
                angle = 0x6000;
            break;
        case 1:
            em_mot_set(self, 32, 10, 0);
            if (angle < 0xA000)
                angle = (u16)-0x6000;
            else if (angle > 0xE000)
                angle = (u16)-0x2000;
            break;
        }
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_8079BCD8 * get_em_base_scale(self) * get_em_chg_scale(self);
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + angle);
        if (angle > 0x8000)
            angle = (u16)(0x10000 - angle);
        self->timer_0x020 = (s16)angle;
        em_hit_window_set_default(self, 0, 30);
        break;
    }
    case 1:
        switch (self->state_0x006) {
        case 0:
            em_turn_in_window(self, lbl_8079BCDC, lbl_8079BCE0, self->timer_0x020);
            break;
        case 1:
            em_turn_in_window(self, lbl_8079BCDC, lbl_8079BCE0, -self->timer_0x020);
            break;
        }
        if (em_frame_check(self, 3, lbl_8079BCC4, lbl_8079BCE4) == 1)
            CancelFade(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_turn_seq_start(self, lbl_80570AE0, 0, 0, 0);
        }
        break;
    case 2:
        if (em_turn_seq_step(self, lbl_80570AE0) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_move_mode_set(self, 0);
            em_mot_set(self, 2, 10, 0);
            em_approach_start(self, lbl_8079BCD4, 0);
        }
        break;
    case 3:
        switch (self->state_0x006) {
        case 0:
            if (em_approach_step(self, 0, 128) == 1) {
                self->state_0x006++;
                if (em_busy_ck(self) == 0)
                    fn_8012F5C4(self, 20, 20, 0, 1);
            }
            break;
        case 1:
            break;
        }
        if (em_busy_ck(self) == 1) {
            if (fn_80382E48(self, 1) == 1)
                em_state_set(self, 13, 7);
            else
                em_action_finish(self);
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
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 10, 0);
        self->timer_0x020 = 300;
        fn_803B9BA0(self, &self->pos, 60);
        break;
    case 1:
        if (--self->timer_0x020 <= 0)
            em_state_set(self, 13, 8);
        break;
    }
}

/* The enemy work's action-program channel 8: phase 0 arms motion 10 sub-motion 6 and sets the
 * 1000-frame wait at +0x020; phase 1 ends the program once `em_mot_end_ck` reports the motion done. */
extern "C" void em_act_prog_8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 10, 6, 0);
        fn_80130CDC(self, 1000);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
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
        em019_motion_dispatch(self);
        break;
    case 1:
        em019_state_dispatch(self);
        break;
    case 2:
        fn_8037A470(self);
        break;
    case 3:
        em019_approach_dispatch(self);
        break;
    case 4:
        em019_battle_dispatch(self);
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
        em019_action_start_if_idle(self);
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
        em_action_finish(self);
        break;
    }
    if (self->field_0x358 == 1) {
        if (++self->tev_0x35A > 450)
            self->tev_0x35A = 450;
    } else {
        self->tev_0x35A = 0;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* The per-part damage check: for each of the seven slots, either raises the slot's damage meter
 * (`em_part_rec_reset`) while its `fn_8037900C` arm byte is clear and its damage level is still below the
 * slot's threshold, or ticks it down (`em_part_rec_alt_set`) through the pair of ids. */
extern "C" void em_parts_damage_ck(_ENEMY_WORK* self) {
    if (fn_8037900C(self, 2) == 0 && em_parts_damage_level_get(self, 0) < 2)
        em_part_rec_reset(self, 0);
    else
        em_part_rec_alt_set(self, 0, 0);
    if (fn_8037900C(self, 0) == 0 && em_parts_damage_level_get(self, 6) < 2)
        em_part_rec_reset(self, 1);
    else
        em_part_rec_alt_set(self, 1, 1);
    if (fn_8037900C(self, 1) == 0 && em_parts_damage_level_get(self, 1) < 2)
        em_part_rec_reset(self, 2);
    else
        em_part_rec_alt_set(self, 2, 2);
    if (fn_8037900C(self, 0) == 0 &&
        (em_parts_damage_level_get(self, 2) < 1 || em_parts_damage_level_get(self, 3) < 1))
        em_part_rec_reset(self, 3);
    else
        em_part_rec_alt_set(self, 3, 3);
    if (fn_8037900C(self, 0) == 0 &&
        (em_parts_damage_level_get(self, 4) < 1 || em_parts_damage_level_get(self, 5) < 1))
        em_part_rec_reset(self, 4);
    else
        em_part_rec_alt_set(self, 4, 4);
    if (fn_8037900C(self, 0) == 0 && em_parts_damage_level_get(self, 6) < 2)
        em_part_rec_reset(self, 5);
    else
        em_part_rec_alt_set(self, 5, 5);
    if (fn_8037900C(self, 3) == 0 && em_parts_damage_level_get(self, 7) < 1)
        em_part_rec_reset(self, 6);
    else
        em_part_rec_alt_set(self, 6, 6);
}

/* Every twentieth frame, drops the record's own ground-stamp vector: builds the (0, -20, 100) offset
 * and hands it to `eft_spawn_type10` with the record's own flag. */
extern "C" void em_act_effect_ck(_ENEMY_WORK* self) {
    nw4r::math::VEC3 off;

    VEC3_ctor(&off);
    if (em_alt_mode_ck(self) == 1) {
        if (system_w.field_0x0c % 20 == 0) {
            setVector3(&off, lbl_8079BC88, lbl_8079BE88, lbl_8079BD24);
            eft_spawn_type10(self, 25, 25, &off, lbl_8079BCA8);
        }
    }
}


extern "C" {

/* Resets one part slot: stores the caller's K-colour index, clears the slot's timer, meter, arm byte
 * and alpha, and takes the slot's RGB either from the caller's colour or from the shared id table. */
extern "C" void em_part_reset(_ENEMY_WORK* self, u8 idx, u8 colour_id, const u8* colour) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    part->colour_id[idx] = colour_id;
    part->timer[idx] = 0;
    part->meter[idx] = lbl_8079BC88;
    part->flag[idx] = 0;
    if (colour == NULL) {
        GXColor tmp;

        ((MHchar*)self->char_0x024)->getTevKColor(lbl_805EE5A0[idx], GX_KCOLOR3, &tmp);
        part->r[idx] = tmp.r;
        part->g[idx] = tmp.g;
        part->b[idx] = tmp.b;
    } else {
        part->r[idx] = colour[0];
        part->g[idx] = colour[1];
        part->b[idx] = colour[2];
    }
    part->a[idx] = 0;
}

/* Lerps one part slot's RGB toward a target colour by the slot's alpha over the caller's ceiling,
 * and stamps the caller's alpha on the result. */
extern "C" void em_part_colour_lerp(_ENEMY_WORK* self, u8* out, u8 idx, const u8* colour, u8 ceiling) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
    f32 t = (f32)part->a[idx] / (f32)ceiling;

    out[0] = part->r[idx] + (s32)((f32)(colour[0] - part->r[idx]) * t);
    out[1] = part->g[idx] + (s32)((f32)(colour[1] - part->g[idx]) * t);
    out[2] = part->b[idx] + (s32)((f32)(colour[2] - part->b[idx]) * t);
    out[3] = 255;
}

/* Steps one part slot's damage meter by 1/damage, arming the slot when it reaches 1.0 and disarming
 * it when it falls back to 0.0. */
extern "C" void em_part_damage_meter(_ENEMY_WORK* self, u8 idx, f32 damage) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    if (damage <= lbl_8079BC88)
        return;
    if (part->flag[idx] == 0) {
        part->meter[idx] += lbl_8079BCA8 / damage;
        if (part->meter[idx] >= lbl_8079BCA8) {
            part->meter[idx] = lbl_8079BCA8;
            part->flag[idx] = 1;
        }
    } else {
        part->meter[idx] -= lbl_8079BCA8 / damage;
        if (part->meter[idx] <= lbl_8079BC88) {
            part->meter[idx] = lbl_8079BC88;
            part->flag[idx] = 0;
        }
    }
}
}


#pragma peephole on

/* 0x80382BB0 */
extern "C" void fn_80382BB0(_ENEMY_WORK* self, u8* out_a, u8* out_b) {
    em_move_mode_set(self, 4);
    *out_a = 12;
    *out_b = 0;
}

/* 0x80382BFC */
extern "C" void fn_80382BFC(void) {
}

/* 0x80382C00 */
extern "C" u32 fn_80382C00(_ENEMY_WORK* self) {
    return em_parts_damage_level_get(self, 7) != 0;
}

/* 0x80382C40 */
extern "C" s32 fn_80382C40(_ENEMY_WORK* self) {
    if (self->field_0x1E2 == 4) {
        if (em_alt_mode_ck(self) == 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x80382DB8 */
extern "C" void fn_80382DB8(_ENEMY_WORK* self, u32 flag) {
    if (flag == 1) {
        self->part_0x740.field_0x740 = 1;
    } else {
        self->part_0x740.field_0x740 = 0;
    }
}

/* 0x80382F94 */
extern "C" u32 fn_80382F94(_ENEMY_WORK* self) {
    if (self->action == 13) {
        if ((u8)(self->state_sub - 2) <= 6) {
            return 1;
        }
    }
    return 0;
}

