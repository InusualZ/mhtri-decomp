/* enemy/em_kind.cpp - the enemy kind set (`em_kind_release` and its lookups)
 *
 * `.text` 0x8013BE60..0x801411B8, 119 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): fold of 2 registered units, built from `enemy/fn_8013BE60.c`, `enemy/fn_8013F764.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 *
 * Kept views: the retired sources declared 16 callee(s) with different signatures (`calcVecDistXZ`, `fn_8012B380`, `fn_8013023C`, `fn_80130858`, `fn_80130CDC`, `fn_801321B0`, `fn_8013BDE4`, `fn_8013C36C`, `fn_801406E0`, `fn_80140778`, `fn_801408B4`, `fn_801409C8`, ...); each function keeps its own source's view through a function-pointer cast macro (`<name>_viewN`, `<name>_cN`), which compiles to the same direct call, so the fold does not move any body.
 * Hidden declarations: 7 header declaration(s) that disagree with the kept view are renamed away around their `#include` (`#define <name> <name>_hidden_<header>`): `fn_801406E0`, `fn_80140778`, `fn_801408B4`, `fn_801409C8`, `fn_80140AF8`, `fn_80140B10`, `fn_80140C00`.
 */

/* Retired header of `enemy/fn_8013BE60.c` (kept for its notes and residuals): */
/* auto/8013BE60_fn_8013BE60.c - the enemy parameter interpreter and its handler table,
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * .text 0x8013BE60..0x8013F764 (78 functions).
 *
 * What it is.  `fn_8013BE60` is the generic "apply one parameter record" interpreter: it takes the
 * enemy work record, a byte stream, an action id, a sub-command and a signed value, and dispatches on
 * the stream's command byte and the sub-command.  `fn_8013C244` is its two-argument tail
 * (`fn_8013BE60(self, in, id, 0, 0)`).  The other 76 functions are the per-action entries:
 *   * the two-argument entries - `if (*in == 0) fn_8013BE60(self, in, ID, SUB, VALUE); else
 *     fn_8013C244(self, in, ID);` - about thirty of them, all identical in shape;
 *   * the stream readers (`switch (*in)` over 0 / 2 / 255) that advance the stream through
 *     `fn_801406E0`/`fn_8013BDE4` and report a `s16` back through a stack slot;
 *   * the entries that read a field of the `_ENEMY_DATA` record `enemy_data_find` returns.
 *
 * `self` is the game's enemy work record (`_ENEMY_WORK`, the map's `11_ENEMY_WORK` mangling):
 * `get_enemy_data`, `em_sleep_ck` and `get_move_work_adrs` take it directly, and +0x188/+0x36C are its
 * own and its target position.  The field names are offset-derived, because the record is a large
 * opaque engine struct whose fields are only visible one at a time through the handlers; a field is
 * renamed when its use makes the meaning clear.
 *
 * Flags.  Two scoped pragmas, each measured on this unit:
 *   * `#pragma peephole off` - retail keeps MWCC's unfused forms where the -O3 peephole folds them:
 *     `(x & 8) == 0` stays `rlwinm`+`cmpwi` (fn_8013DD48) and `((x & 0xF) << 8) | y` stays
 *     `clrlwi`+`slwi`+`or` instead of one `rlwimi` (fn_8013D310).  With the pragma:
 *     fn_8013DD48 97.81 -> 100.00, fn_8013D310 75.67 -> 100.00, fn_8013DA7C 99.98 -> 100.00,
 *     fn_8013E388 99.93 -> 100.00; no other symbol moves.
 *   * `#pragma fp_contract off` - the -fp_contract-on default fuses `dx*dx + dz*dz` into `fmadds` in
 *     fn_8013E2B0 (99.81) where retail keeps `fmuls`+`fmuls`+`fadds` (100.00).
 *
 * Residual (65 of 78 functions byte-identical; unit fuzzy_match_percent 72.19, matched_code 8584 of
 * 14596):
 *   * not reconstructed yet: fn_8013C57C (536 B), fn_8013E528 (460 B), fn_8013BE60 (996 B),
 *     fn_8013EFD4 (1508 B), fn_8013F5B8 (428 B);
 *   * fn_8013D1D0 47.50 - a 16-byte tail call where retail computes `in[0]` into a scratch register
 *     and `in + 1` from the unmodified pointer, while our source loads `in[0]` into r4 first;
 *   * fn_8013D588 79.51 - the 16-bit angular-difference idiom: retail emits
 *     `clrlwi`+`cmplwi 0x8000`+`subf 0x10000` (i.e. `abs((s16)diff)`) where our plain
 *     `(s16)diff > tolerance` gives `extsh`+`cmpw`;
 *   * fn_8013C7E0 91.45, fn_8013CAA0 91.04, fn_8013D8C8 98.00 - nested-loop return shapes (retail
 *     carries a second unconditional branch to the shared `return 0` that our source does not emit);
 *   * fn_8013C988 99.69, fn_8013D054 99.66, fn_8013EBC8 99.82 - one allocator or compare instruction
 *     each.
 *
 * The bodies are grouped by shape rather than written in address order; objdiff pairs by symbol name,
 * so the per-symbol score is unaffected.
 *
 * Range and inventory: `python tools/units/ledger.py unit auto/8013BE60_fn_8013BE60.c`.
 * Attribute pass evidence: `.pi/attribution-batch-4.patch.md`, `.pi/notes/attribution-batch-4.md`.
 * Data runs this unit's split records but does not yet claim (playbook 23, docs/plan.md 8.4):
 *   extabindex 0x80027D20..0x80027F60 (48)   .data 0x805A19D0..0x805A1A7C (3)
 *   .bss       0x806BD360..0x806BD808 (1)    .sdata2 0x80796D90..0x80796DBC (9)
 * `extab`/`extabindex` travel with the code unit (`dataqueue.py`'s `FRAGMENT_SECTIONS`); the rest
 * waits for the measured data pass (`tools/units/dataclaim.py`).
 *
 * Types.  `_ENEMY_WORK`, `_ENEMY_DATA`, `_ENEMY_TABLE`/`_ENEMY_LIST_*`/`_ENEMY_QUEUE_ENTRY` are
 * reconstructed here.  `Vec3` comes from `nw4r/math.h` (the same three `f32` as `nw4r::math::VEC3`,
 * now includable from C as well as C++).
 */

/* Retired header of `enemy/fn_8013F764.cpp` (kept for its notes and residuals): */
/* enemy/fn_8013F764.cpp - the enemy program interpreter's second half: the run driver, the stream
 * readers and the command-length/stream-walk helpers, `.text` 0x8013F764..0x801411B8 (45 functions).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x8013F764 0x801411B8`: every one of the 45 rows is a bare
 * `fn_XXXXXXXX = .text:0x...` entry with no real name).
 *
 * What it is.  The unit directly below it, `enemy/fn_8013BE60.c` (0x8013BE60..0x8013F764), is "the
 * enemy parameter interpreter and its handler table": `fn_8013BE60(self, in, id, sub, value)` applies
 * one parameter record and `fn_8013C244(self, in, id)` is its two-argument tail.  This unit is the
 * rest of that same interpreter:
 *
 *   * the run driver `fn_8013F764` - it walks the enemy's program table (`prog_0xA00`, 3-byte entries
 *     `EmProgTbl`), latches the entry through `fn_801262BC`, measures the enemy against its target
 *     position `vec_0x36C` (`calcVecDistXZ`) and dispatches on the two entry bytes through
 *     `fn_8013C36C`; `fn_8013F994` is its "next entry" step.
 *   * the stream readers - the long family `fn_8013F8D8`/`fn_8013F9D8`/`fn_8013FB9C`/`fn_8013FC60`/
 *     `fn_8013FD98`/`fn_8013FF5C`/`fn_8014001C`/`fn_801400BC`/`fn_80140178`/`fn_80140298`/
 *     `fn_80140354`/`fn_801404B8`/`fn_80140580`: same shape as the neighbours' readers - `switch
 *     (*in)` over 0/2/255, `in += fn_801406E0(code, *in)` then `fn_8013BDE4(&in, code, &value)` for
 *     command 0, `in += ...; value = fn_80140778(in, code, 1)` for command 2, `fn_80140AF8(self, 0,
 *     0)` for anything else, and an `s16` back through the frame slot.  The command ids run 0x4E
 *     (fn_8013F8D8) to 0x6A (fn_80140648) in the order the target dispatches them.
 *   * the one-line entries - `fn_8013FD1C`/`fn_801400B0` (field setters), `fn_8013FEF4`,
 *     `fn_8013FD28`/`fn_8013FD50`/`fn_8013FD74`/`fn_8013FF08`/`fn_8013FF38`/`fn_80140244`/
 *     `fn_8014026C`/`fn_80140648`/`fn_80140670` (`if (*in == 0) fn_8013BE60(self, in, id, sub, v);
 *     else fn_8013C244(self, in, id);`, the same two-argument family the neighbour carries).
 *   * the stream helpers themselves: `fn_801406E0` (the command-length table `lbl_805A1530`),
 *     `fn_80140768` (its two-byte form), `fn_80140778` (the recursive `{code, sub}` walker that
 *     yields the offset the readers want), `fn_80140AF8`/`fn_80140B0C`/`fn_80140B10` (the trace
 *     helpers retail compiled to nothing), `fn_801408B4` (the interpreter's save/restore stack),
 *     `fn_801409C8` (the action-timeout setup), `fn_80140B20` (the program-table pointer arm),
 *     `fn_80140C00` (the per-enemy extra-table lookup) and the enemy file loader
 *     `fn_80140CA0`/`fn_80140DAC`/`fn_80140E48`/`fn_80140EE8`/`fn_80140FB0`/`em_kind_release` that
 *     `load_file_req` drives.
 *
 * Naming evidence (brief section 2, in order).  1. No `__FILE__` string is reachable from this range:
 * the only absolute addresses its code builds are `lbl_805A1530` (the 0x6D-byte command-length
 * table), `lbl_805A1B34` (the `emNNN_prog_tbl` pointer array), `lbl_805A1B08`, the two file tables
 * `lbl_80581E20`/`lbl_80582348` and the `.sdata2` float pool (`lbl_80796DBC`/`DC0`/`DC4`) - all data,
 * no source-file name.  2. `python tools/symbols/dumpmap.py lookup` answers `zz_013f764_` for every
 * address in the range (a `zz_` placeholder, not evidence).  3. The code is enemy-band: it calls
 * `get_enemy_data(_ENEMY_WORK*)`, `get_move_work_adrs`/`get_move_work_max`, `fn_8013BE60`/
 * `fn_8013C244` (the neighbour unit's interpreter), and the program tables it walks are
 * `em001_prog_tbl`..`em040_prog_tbl`; both bracketing registered units are `enemy`
 * (`enemy/fn_8013BE60.c` below, `enemy/fn_80149D6C.c` above) and the neighbours' scheme is the map's
 * own stem.  The file therefore keeps the map stem (brief option 4); no name was invented, and the
 * module is `enemy`.
 *
 * Language.  The unit calls four mangled callees (`get_enemy_data__FP11_ENEMY_WORK`,
 * `get_move_work_adrs__FUc`, `get_move_work_max__FUc`, `load_file_req__FPcUllUllPUl`) and rule 9
 * forbids spelling a mangling as the callable identifier, so every one of them is declared at C++
 * scope with the signature its mangling encodes (`tools/units/mangle.py` proves each) and the file is
 * C++.  Every definition keeps the map's plain `fn_XXXXXXXX` name, i.e. the `extern "C"` block below.
 *
 * Types.  `_ENEMY_WORK` is the shared record `include/enemy/ENEMY_WORK.h` owns (rule 1); this unit
 * added the fields it measured (+0x010, +0x360, +0x36C, +0x383/+0x384, the 0x961 stack block and the
 * +0x9E8..+0xA00 interpreter bytes) to that header, and the record types that go with them
 * (`EmProgTbl`/`EmCmdRec`/`EmCmdRecWide`/`EmAreaWork`/`EmActRec`/`EmFileEntry`/`EmFileRow`) live
 * there too, because the same interpreter is spread over this unit and its neighbours.
 *
 * Sections.  Besides `.text` the unit owns `extab` 0x8000D5A4..0x8000D664 and `extabindex`
 * 0x80027F60..0x80028080 - the 24 entries of the functions that carry an exception record, taken
 * from the target's own run boundaries (the previous unit's `extabindex` ends exactly at 0x80027F60
 * and the next one starts exactly at 0x80028080).  No `.ctors` word and no data.
 *
 * Residuals.  `python tools/units/recompile.py enemy/fn_8013F764.cpp` (target object from the
 * unit's own split, 6740 B of `.text`): 27 of the 45 bodies are byte-identical, the unit measures
 * `.text` 95.63 % (`matched_code` 2872 of 6740 B, `matched_functions` 27 of 45).  `extab` comes out
 * **100 %** (192 B, byte-identical) and every function is above the 80 % bar except two:
 *
 *   * `fn_801408B4` 67.70: a 7-way dispatch on `field_0x95C`.  The target lowers it as a *range test*
 *     (`subi r0,r4,3` / `cmplwi r0,4` / `ble` into one body shared with case 0xB) followed by the
 *     0/2/0xB/1/0xA compares, and keeps the field in r4.  `switch (self->field_0x95C)` with the
 *     six cases sharing that body produces the target's exact 276 bytes but MWCC lowers it as a
 *     compare chain (`cmplwi r?,3` / `blt`) instead, which costs the first four rows; an explicit
 *     `if (v - 3 <= 4) { ... }` does emit the range test but then duplicates the 0xB body (316 B).
 *     Both spellings were measured; the switch is kept (right size, 67.70; the if-form's extra copy
 *     of the body makes the object 40 B too long, so it is not kept).
 *   * `fn_80140B10` 75.00: the 4-instruction trace wrapper.  The target's last two instructions are
 *     `b .+4` + `blr` - a branch to an empty body the compiler placed 4 bytes after the function -
 *     and its arguments are the caller's r4/r5 (its own r3 is unused), with r4 masked to a byte and
 *     r5 passed through.  `static void em_prog_trace2(u8, u32)` called as
 *     `em_prog_trace2((u8)a, b)` from a 3-parameter wrapper reproduces all three setup instructions
 *     exactly; only the tail branch differs (ours names the helper, the target's is the local label),
 *     which is what the 100 - 25 % costs.  The helper cannot be emitted inline: with
 *     `-inline noauto` MWCC keeps the named call, and with the helper inlined the mask
 *     survives but `mr r4,r5` does not (measured: 58.75 %).
 *
 * The rest, by size: `fn_80140AF8` 88.00 (an extra `mr r0,r4`: MWCC shuffles the second argument into
 * r4 before masking the first, where retail masks r4 into r3 first; measured with the parameters
 * declared u32/u8/u8 and with the cast pulled out into a local), `fn_801409C8` 88.09,
 * `fn_80140778` 88.23, `em_kind_release` 90.28 (register allocation: retail keeps the id in r30 and its
 * counter in r31 and spends r28/r29 on the index and the sub-command), `fn_80140CA0` 92.00,
 * `fn_8014001C` 94.32, `fn_80140E48`/`fn_80140FB0` 94.50, `fn_80140B20` 96.88, `fn_80140C00` 96.58,
 * `fn_8014026C` 99.09 (retail keeps an `extsh` after the `clrlwi` on the value it forwards;
 * `(s16)(u8)arg` folds to the mask alone in this compiler, `(s16)(arg & 0xFF)` and a `(s32)`
 * parameter were both measured), `fn_80140354` 99.47, `fn_8013F764` 99.62, `fn_8013FD98` 99.63
 * (register allocation of the two loop variables), `fn_801406E0` 99.71, `fn_80140EE8` 99.90.
 * `extabindex` (288 B) lands at 98.61 % because the entries carry each function's own size, so the
 * two functions whose bodies are still short shift their `.4byte` size word.
 *
 * Source shape worth keeping: the unit needs `#pragma peephole off`.  Retail keeps the unfused
 * `rlwinm`/`clrlwi` + `cmpwi` pairs the `(v & 0x80) == 0` and `len & 0x30` tests are built from;
 * with the peephole pass on they become `rlwinm.`/`clrlwi.` and the branch moves.  Measured on the
 * whole unit: `fn_80140AF8` 39.00 -> 88.00, `fn_80140B10` 0.00 -> 75.00, `fn_801406E0` 77.38 ->
 * 99.71, `fn_80140C00` 76.20 -> 96.58 (the signed `(s32)(u8)sel` comparison plus the typed
 * `SystemWorkTables` view),
 * `fn_80140778` 85.76 -> 88.23, `fn_801408B4` 59.57 -> 67.70, `fn_80140B20` 93.30 -> 96.88,
 * `em_kind_release` 79.17 -> 90.28; no symbol moved down.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "enemy/emc_work.h" /* emc_work (rule 2: the owner's header) */
#include "enemy/fn_80131BD4.h" /* fn_80131BD4 (rule 2: the owner's header) */
#include "enemy/fn_80125FF0.h" /* fn_80125FF0 (rule 2: the owner's header) */
#include "enemy/em_act_step_arm.h" /* em_act_step_arm (rule 2: the owner's header) */
#include "enemy/fn_80126278.h" /* fn_80126278 (rule 2: the owner's header) */
#include "enemy/fn_8012B380.h" /* fn_8012B380 (rule 2: the owner's header) */
#include "enemy/em_target_pos_set.h" /* em_target_pos_set (rule 2: the owner's header) */
#include "enemy/fn_80126DAC.h" /* fn_80126DAC (rule 2: the owner's header) */
#include "enemy/fn_80126F80.h" /* fn_80126F80 (rule 2: the owner's header) */
#include "enemy/fn_801272C4.h" /* fn_801272C4 (rule 2: the owner's header) */
#include "enemy/fn_801275F0.h" /* fn_801275F0 (rule 2: the owner's header) */
#include "enemy/fn_80127A7C.h" /* fn_80127A7C (rule 2: the owner's header) */
#include "enemy/em_area_change.h" /* em_area_change (rule 2: the owner's header) */
#include "enemy/fn_801262BC.h" /* fn_801262BC (rule 2: the owner's header) */
#include "enemy/em_break_state_ck.h" /* em_break_state_ck (rule 2: the owner's header) */
#include "enemy/fn_80137704.h" /* fn_80137704 (rule 2: the owner's header) */
#include "enemy/fn_801358D0.h" /* fn_801358D0 (rule 2: the owner's header) */
#include "enemy/em_motion_mode_set.h" /* em_motion_mode_set (rule 2: the owner's header) */
#include "enemy/fn_80126494.h" /* fn_80126494 (rule 2: the owner's header) */
#include "sound/snd_bank_loader.h" /* snd_bank_loader (rule 2: the owner's header) */
#include "types.h"
#include "enemy/ENEMY_WORK.h"
#include "nw4r/math.h"
#include "fn_8004CAD8.h"
#include "stage/stg_w.h"
#include "enemy/fn_80138074.h"
#define fn_801406E0 fn_801406E0_hidden_enemy_h
#define fn_80140778 fn_80140778_hidden_enemy_h
#define fn_801408B4 fn_801408B4_hidden_enemy_h
#define fn_801409C8 fn_801409C8_hidden_enemy_h
#define fn_80140AF8 fn_80140AF8_hidden_enemy_h
#define fn_80140B10 fn_80140B10_hidden_enemy_h
#define fn_80140C00 fn_80140C00_hidden_enemy_h
#include "unsplit/enemy.h"
#undef fn_80140C00
#undef fn_80140B10
#undef fn_80140AF8
#undef fn_801409C8
#undef fn_801408B4
#undef fn_80140778
#undef fn_801406E0
#include "ef/eft_slot.h"     /* eft_slot_effect_key, enemy_data_find, enemy_data_grp */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "unsplit/enemy_pool.h" /* the band's unowned .data pools (rule 2) */
#include "unsplit/em_snd_proc_tbl.h" /* em_snd_proc_tbl (rule 2: the band) */
#include "enemy/em_pop.h" /* quest_flag_100000_ck, quest_flag_10_ck (rule 2: their owner) */
#include "ef/nw_res_manager.h"
/* the call sites use the argument-less view: a cast call is the same direct call. */
#define em_mot_finished_ck_c1 ((u32 (*)(void))em_mot_finished_ck)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_80131BD4_c1 ((u32 (*)(void))fn_80131BD4)
#define fn_80125FF0_c1 ((u32 (*)(u8, u8))fn_80125FF0)
#define fn_80137704_c1 ((u32 (*)(_ENEMY_WORK*, u32))fn_80137704)
#define em_motion_mode_set_c1 ((void (*)(_ENEMY_WORK*, u32))em_motion_mode_set)
/* get_worldworld_pos_out: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define get_worldworld_pos_out ((void (*)(nw4r::math::VEC3*, nw4r::math::VEC3*, u8))get_worldworld_pos)
/* fn_80140C00_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80140C00_view1 ((struct EnemyData* (*)(u8, u8))fn_80140C00)
/* fn_80140B10_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80140B10_view1 ((void (*)(struct _ENEMY_WORK *, u32, u32))fn_80140B10)
/* fn_80140AF8_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80140AF8_view1 ((void (*)(struct _ENEMY_WORK *, u32, u32))fn_80140AF8)
/* fn_801409C8_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_801409C8_view1 ((void (*)(struct _ENEMY_WORK *, u8 *, u32, u32, s32))fn_801409C8)
/* fn_801408B4_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_801408B4_view1 ((s32 (*)(struct _ENEMY_WORK*))fn_801408B4)
/* fn_80140778_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80140778_view1 ((s16 (*)(u8 *, u32, u32))fn_80140778)
/* fn_801406E0_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_801406E0_view1 ((u32 (*)(u32, u32))fn_801406E0)
/* fn_8013C36C_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8013C36C_view1 ((void (*)(_ENEMY_WORK*, u8, u8*))fn_8013C36C)
/* fn_8013BDE4_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8013BDE4_view1 ((void (*)(u8**, u8, s16*))fn_8013BDE4)
/* fn_801321B0_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_801321B0_view1 ((u32 (*)(_ENEMY_WORK*))fn_801321B0)
/* fn_80130CDC_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80130CDC_view1 ((void (*)(_ENEMY_WORK*, s32))fn_80130CDC)
/* fn_80130858_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80130858_view1 ((void (*)(_ENEMY_WORK*, s32))fn_80130858)
/* fn_8013023C_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8013023C_view1 ((u32 (*)(_ENEMY_WORK*))fn_8013023C)
/* fn_8012B380_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8012B380_view1 ((void (*)(_ENEMY_WORK*, u32, u32, u8))fn_8012B380)
/* calcVecDistXZ_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define calcVecDistXZ_view1 ((f32 (*)(nw4r::math::VEC3*, nw4r::math::VEC3*))calcVecDistXZ)

/* --------------------------------------------------------------------------------------------- */
/* The enemy work record.  Fields are listed at their real offsets; `pad_0xNNN` is only what the
 * handlers never touch.  size: at least 0xA04 (the largest field read is +0xA00). */
/* --------------------------------------------------------------------------------------------- */

/* size: 0xC
 * `Vec3` is the C spelling from `nw4r/math.h` (layout x/y/z at +0x00/+0x04/+0x08). */

/* The enemy-data table chain: `get_enemy_data`'s +0x18 word points at the head record, whose +0x00 word
 * is the first 8-byte action entry; an entry's +0x04 word is its own list of 8-byte item entries. */
/* size: 0x8 */
typedef struct _ENEMY_LIST_HEAD {
    /* +0x00 */ struct _ENEMY_ACTION *first;
    /* +0x04 */ u32 pad_0x04;
} _ENEMY_LIST_HEAD;

/* size: 0x8 */
typedef struct _ENEMY_ACTION {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ struct _ENEMY_ITEM *items;
} _ENEMY_ACTION;

/* size: 0x8 */
typedef struct _ENEMY_ITEM {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ u8 *entries; /* 8-byte entries */
} _ENEMY_ITEM;

/* The per-action entry table `fn_8013C254` indexes with the record at +0x9A4. */
/* size: 0xC */
typedef struct _ENEMY_TABLE_ENTRY {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ f32 value_0x04;
    /* +0x08 */ u8 value_0x08;
    /* +0x09 */ u8 pad_0x09[0x3];
} _ENEMY_TABLE_ENTRY;

/* size: 0x8 */
typedef struct _ENEMY_TABLE {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ _ENEMY_TABLE_ENTRY *entries;
} _ENEMY_TABLE;

extern "C" {
/* Callees owned by other translation units.  Addresses and the mangled spellings are the map's. */
/* `enemy_data_grp` (0x803439D4) is declared in its owner's header, `include/ef/eft_slot.h` (rule 2). */

/* --------------------------------------------------------------------------------------------- */
/* The generic parameter interpreter and its two-argument tail. */
/* --------------------------------------------------------------------------------------------- */
s16 fn_8013BE60(_ENEMY_WORK *self, u8 *in, u32 id, u32 sub, s16 value);
void fn_8013C36C(_ENEMY_WORK *self, u8 value, u32 param);
}

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* The enemy-data record `enemy_data_find` returns: only the offsets this unit reads are named. */
/* --------------------------------------------------------------------------------------------- */

/* size: 0x40 (lower bound: the highest field this unit reads is +0x39) */
typedef struct _ENEMY_DATA {
    /* +0x000 */ u8 pad_0x000[0x08];
    /* +0x008 */ u8 field_0x08;
    /* +0x009 */ u8 pad_0x009[0x2];
    /* +0x00B */ u8 field_0x0B;
    /* +0x00C */ u8 pad_0x00C;
    /* +0x00D */ u8 field_0x0D;
    /* +0x00E */ u8 field_0x0E;
    /* +0x00F */ u8 mode_0x0F;
    /* +0x010 */ u8 value_0x10;
    /* +0x011 */ u8 pad_0x011[0x3];
    /* +0x014 */ u8 mode_0x14;
    /* +0x015 */ u8 mode_0x15;
    /* +0x016 */ u8 mode_0x16;
    /* +0x017 */ u8 mode_0x17;
    /* +0x018 */ u32 ptr_0x18;
    /* +0x01C */ u8 pad_0x01C[0x1A];
    /* +0x036 */ u8 field_0x36;
    /* +0x037 */ u8 field_0x37;
    /* +0x038 */ u8 pad_0x038;
    /* +0x039 */ u8 field_0x39;
    /* +0x03A */ u8 pad_0x03A[0x6];
} _ENEMY_DATA;


extern "C" {
/* `enemy_data_find` (0x803438E4): its owner's header, `include/ef/eft_slot.h` (rule 2). */
extern u32 fn_80345A6C(void *a, u8 b, Vec3 *c, f32 d);
}

u32 PlayMode_ck(void);

extern "C" {
extern f32 lbl_80796DA0; /* 0.0f */
extern f32 lbl_80796DA4; /* 0.5f */
extern f32 lbl_80796DA8; /* 65536.0f */
extern f32 lbl_80796DAC; /* 6.2831855f */
extern f32 sqrt_f32(f32 x);
extern f32 atan2f(f32 y, f32 x);
/* `eft_slot_effect_key` (0x8034539C): its owner's header, `include/ef/eft_slot.h` (rule 2). */
extern void fn_8013C57C(_ENEMY_WORK *self, u8 *in, u32 flag);
extern void fn_802B01AC(Vec3 *out, Vec3 *in, u32 idx);
extern f32 fn_802B0430(u32 idx);
}


/* A neighbouring record `fn_80130DF8` returns; only the two bytes this unit reads are named. */
/* size: 0x8 (lower bound: the unit reads +0x05 and +0x06) */
typedef struct _ENEMY_OTHER {
    /* +0x000 */ u8 pad_0x00[0x5];
    /* +0x005 */ u8 field_0x05;
    /* +0x006 */ u8 field_0x06;
    /* +0x007 */ u8 pad_0x07;
} _ENEMY_OTHER;

/* ------------------------------------------------------------------------------------------------ *
 * pooled data owned by other units: declared, never defined (playbook 29), so the load operands pair
 * with the target's pool relocations
 * ------------------------------------------------------------------------------------------------ */

/* the 0x6D-byte command-length table `fn_801406E0` reads */
/* the `em001_prog_tbl`..`em040_prog_tbl` pointer array, NULL-terminated (`fn_80140B20` arms it) */
/* the per-enemy byte table `em_kind_release` reads (`lbzx` + `extsb`, so it is signed) */
/* the two `{key, name}` file tables `em_kind_release` walks */
extern EmFileEntry lbl_80581E20[];
extern EmFileEntry lbl_80582348[];
/* the `.sdata2` pool the range's float work reads */
extern f32 lbl_80796DBC;
extern f32 lbl_80796DC0;
extern f32 lbl_80796DC4;
/* the row base `fn_80140CA0` indexes with `fn_800D4DD8`'s answer */
/* the system work record `fn_80140C00` looks its extra table up in */
extern u8 system_w[];

/* ------------------------------------------------------------------------------------------------ *
 * the callees
 *
 * The four the map carries mangled are declared at C++ scope with the signature the mangling encodes
 * (`tools/units/mangle.py` proves each one), so no call site spells a mangling (rule 9).
 * ------------------------------------------------------------------------------------------------ */

/* `get_enemy_data__FP11_ENEMY_WORK` (owner `enemy/fn_801251D0.cpp`).  `EnemyData`'s full definition
 * lives in `include/enemy.h`, which cannot be included beside `enemy/ENEMY_WORK.h` (both define
 * `_ENEMY_WORK`), so the record is read through the two words the target loads. */
struct EnemyData;
struct EnemyData* get_enemy_data(_ENEMY_WORK* work);

/* `get_move_work_adrs__FUc` / `get_move_work_max__FUc` (owner `ef/fn_800CDB2C.cpp`). */
void* get_move_work_adrs(u8 area);
u16 get_move_work_max(u8 area);

/* `load_file_req__FPcUllUllPUl` (owner `sound/`'s file layer; the same spelling `ef/eft_res.cpp`
 * uses): (name, destination, size, callback, flag, context). */
void load_file_req(char* name, u32 data, s32 size, u32 callback, s32 flag, u32* ctx);

/* the two records this unit's own call sites name */
/* the 0x18-byte per-enemy file slot of `emc_work` (`fn_80140E48`/`fn_80140EE8`/the load callback) */
struct EmcWork {
    /* +0x00 */ u8 field_0x00;   /* the id `fn_800E3358` gets back */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 index_0x02;   /* `fn_80146F10`'s file-table index */
    /* +0x03 */ u8 flags_0x03;   /* bit 0 = the sound proc was requested, bit 1 = the other file */
    /* +0x04 */ u8 unused_0x04[4];
    /* +0x08 */ s32 handle_0x08; /* the first file's handle */
    /* +0x0C */ u8 unused_0x0C[4];
    /* +0x10 */ s32 handle_0x10; /* the second file's handle */
    /* +0x14 */ u8 unused_0x14[4];
}; /* size: 0x18 */

/* the record `fn_801262BC` hands `fn_8013F764` */
struct EmActionInfo {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 unused_0x01[3];
    /* +0x04 */ f32 value_0x04;
}; /* size: 0x08 */

/* the head `fn_80126494` returns: its +0x08 word is the first `EmActRec` of the list `fn_801409C8`
 * scans.
 * size: 0x0C */
struct EmActList {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ EmActRec* entries_0x08;
};

/* `get_enemy_data`'s record, as far as this unit reads it: the two words `fn_801409C8` loads for its
 * timer comparison are the enemy data's own limit (`include/enemy.h`'s `EnemyData`, which cannot be
 * included beside `enemy/ENEMY_WORK.h`).
 * size: 0x0C */
struct EmEnemyDataSub {
    /* +0x00 */ u8 unused_0x00[0x28];
    /* +0x28 */ s16* value_0x28;
};

/* size: 0xA4 */
struct EmEnemyData {
    /* +0x00 */ u8 unused_0x00[0xA0];
    /* +0xA0 */ EmEnemyDataSub* sub_0xA0;
};

/* the system work record `system_w`, as far as this unit reads it: its +0x898 word is the extra table
 * array `fn_80140C00` indexes by enemy id and then by kind.  Deliberately not called `SystemWork`:
 * `enemy/fn_8014A1BC.c` already owns that name, and rule 1 forbids a second definition of it.
 * size: 0x89C */
struct SystemWorkTables {
    /* +0x000 */ u8 unused_0x000[0x898];
    /* +0x898 */ u32** tables_0x898;
};

/* the handle record `fn_800D5418` returns, as `fn_80140CA0`/`fn_80140EE8` read it (its +0x44 word
 * is handed to `res_file_assign`) */
struct EmHandleRec {
    /* +0x00 */ u8 unused_0x00[0x44];
    /* +0x44 */ u32 field_0x44;
}; /* size: 0x48 */

extern "C" {
/* ------------------------------------------------------------------------------------------------
 * The plain `fn_XXXXXXXX` callees.  The map spells them as C symbols, so the whole set - the map's
 * own definitions included - is `extern "C"`: without it this C++ front-end would mangle them and
 * objdiff would pair nothing (docs/matching.md row 42).  Their owning units' headers do not carry
 * them yet, so they are declared here (rule 2's interim home for the enemy band, the same one
 * `enemy/fn_80137604.cpp` uses).
 * ------------------------------------------------------------------------------------------------ */

/* this unit's own entry points, used before their definitions below */
u8 fn_801406E0(u32 code, u32 sub);
u8 fn_80140768(u8* in);
s16 fn_80140778(u8* in, u8 code, u8 mode);
void fn_80140AF8(_ENEMY_WORK* self, u32 a, u8 b);
void fn_80140B0C(u8 a, u8 b, u8 c);
void fn_80140DAC(EmFileRow* dst, EmFileRow* src);
s32 fn_80140E48(char* name, u32 size, u8 flag);
void fn_80140EE8(char* name, u32 size, u32 a, u32 b, u32 c, u32* ctx);
s32 fn_80140FB0(char* name, u32 size, u8 flag);
void fn_8013F994(_ENEMY_WORK* self);

/* `enemy/fn_8013BE60.c` - the interpreter this unit drives */
s16 fn_8013BE60(_ENEMY_WORK* self, u8* in, u32 id, u32 sub, s16 value);
void fn_8013C244(_ENEMY_WORK* self, u8* in, u32 id);

/* the enemy band's other units */
void fn_8013AA00(_ENEMY_WORK* self);
u32 fn_8027D530(EmAreaWork* rec);
u32 fn_802B0998(u8 id);
char* fn_80146F10(u8 id);
void fn_80146B98(s32 id);
u32 fn_80146DF4(u8 id);
u8 fn_80141470(u8 id);
u8 fn_801414D4(u8 id);
void fn_801414C8(void* work);
void fn_80147160(u8 id);
u8 fn_80147550(u8 id);
s32 fn_80147684(u8 id, s32 a);
void fn_801411B8(void* str);   /* owner: `enemy/fn_801411B8.cpp`'s range, the next proposal */
void fn_800FA9B8(u8* rec, u8 id);

/* the shared layers the range's helpers call */
void* res_file_ctor(void* out, u32 a);
void res_file_assign(void* dst, void* src);
void fn_8007B878(void* out, u32 a);
void fn_8007BA08(void* a, void* b);
s32 fn_800D56F4(char* name, u32 data);
void* fn_800D5418(s32 handle);
s32 fn_800D4DD8(char* name);
void fn_800D5D60(void* work);
void fn_800D5E30(void* work);
void fn_800E38BC(void* a);
void fn_800E3358(u32 a, u8 b, void* c);
s32 fn_800924CC(void* a, u32 b);

extern "C" void fn_801409C8(_ENEMY_WORK* self);
extern "C" void fn_80140B10(u32 unused, u32 a, u32 b);

/* retail's second two-argument trace entry: the callee is a static helper whose body the
 * release build compiled to nothing (the branch lands on its `blr`, 4 bytes after this
 * function's) - the same shape `fn_80140AF8` reaches through the global `fn_80140B0C`. */
static void em_prog_trace2(u8 a, u32 b) {
}
}

#pragma fp_contract off
#pragma peephole off

extern "C" {
/* The two-argument tail: sub-command 0 and a value of 0. */
void fn_8013C244(_ENEMY_WORK *self, u8 *in, u32 id) {
    fn_8013BE60(self, in, (u8)id, 0, 0);
}

/* --------------------------------------------------------------------------------------------- */
/* Byte-set entries: a one-byte record copied straight into the work record. */
/* --------------------------------------------------------------------------------------------- */
void fn_8013C97C(_ENEMY_WORK *self, u8 *in) {
    self->state_0x9F9 = in[0];
}

void fn_8013E05C(_ENEMY_WORK *self, u8 *in) {
    self->state_0x95F = in[0];
}

void fn_8013E9C8(_ENEMY_WORK *self, u8 *in) {
    self->state_0x960 = in[0];
}

void fn_8013D684(_ENEMY_WORK *self, u8 *in) {
    self->timer_0x424 = in[0] * 100;
}

void fn_8013E6F4(_ENEMY_WORK *self) {
    self->value_0x452 = 0;
}

void fn_8013DED4(_ENEMY_WORK *self, u8 *in) {
    self->field_0x1BC = in[0] << 8;
    self->field_0x1C0 = in[1] << 8;
    self->field_0x1C4 = in[2] << 8;
}

/* --------------------------------------------------------------------------------------------- */
/* Two-argument parameter entries: `in[0] == 0` applies the value, anything else re-sends the id. */
/* --------------------------------------------------------------------------------------------- */
void fn_8013CBE4(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 7, 0, self->area_no);
    } else {
        fn_8013C244(self, in, 7);
    }
}

void fn_8013CC0C(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 8, 0, self->field_0x43B);
    } else {
        fn_8013C244(self, in, 8);
    }
}

void fn_8013CC34(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 9, 0, self->field_0x43C);
    } else {
        fn_8013C244(self, in, 9);
    }
}

void fn_8013CC5C(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 10, 0, self->field_0x43D);
    } else {
        fn_8013C244(self, in, 10);
    }
}

void fn_8013CC84(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 11, 4, 0);
    } else {
        fn_8013C244(self, in, 11);
    }
}

void fn_8013CCA8(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 12, 0, self->field_0x1E2);
    } else {
        fn_8013C244(self, in, 12);
    }
}

void fn_8013D2C8(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 22, 5, 0);
    } else {
        fn_8013C244(self, in, 22);
    }
}

void fn_8013D2EC(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 23, 6, 0);
    } else {
        fn_8013C244(self, in, 23);
    }
}

void fn_8013D310(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 24, 7, (s16)(u16)(((self->area_no & 0xF) << 8) | in[1]));
    } else {
        fn_8013C244(self, in, 24);
    }
}

void fn_8013D990(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 35, 0, self->field_0x00C);
    } else {
        fn_8013C244(self, in, 35);
    }
}

void fn_8013DE08(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 41, 0, self->field_0x00A);
    } else {
        fn_8013C244(self, in, 41);
    }
}

void fn_8013DE30(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 68, 9, self->field_0x00A);
    } else {
        fn_8013C244(self, in, 68);
    }
}

void fn_8013DE58(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 42, 15, self->field_0x1E0);
    } else {
        fn_8013C244(self, in, 42);
    }
}

void fn_8013E9D4(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 61, 0, self->state_0x960);
    } else {
        fn_8013C244(self, in, 61);
    }
}

void fn_8013EBA0(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 64, 0, self->field_0x1FE);
    } else {
        fn_8013C244(self, in, 64);
    }
}

void fn_8013EF14(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 70, 0, self->team);
    } else {
        fn_8013C244(self, in, 70);
    }
}

void fn_8013EF3C(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 71, 0, self->field_0x1E3);
    } else {
        fn_8013C244(self, in, 71);
    }
}

void fn_8013EF64(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 73, 0, self->field_0x9F7);
    } else {
        fn_8013C244(self, in, 73);
    }
}

void fn_8013EF8C(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 74, 1, self->field_0x1F2 % 100);
    } else {
        fn_8013C244(self, in, 74);
    }
}

/* A three-argument entry: the caller supplies a value whose low 16 bits are reduced modulo 100. */
void fn_8013C794(_ENEMY_WORK *self, u8 *in, u32 value) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 2, 1, (u16)value % 100);
    } else {
        fn_8013C244(self, in, 2);
    }
}

/* Forwards the stream one byte in and the first byte as the value. */
void fn_8013D1D0(_ENEMY_WORK *self, u8 *in) {
    u8 value = in[0];

    fn_8013C36C(self, value, (u32)(in + 1));
}

/* --------------------------------------------------------------------------------------------- */
/* The stream readers: `switch (in[0])` over the command byte, each case advancing the stream
 * through `fn_801406E0`/`fn_8013BDE4` and reporting a `s16` back through the stack slot. */
/* --------------------------------------------------------------------------------------------- */
s16 fn_8013CCD0(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C) == 0) {
            in += (u8)fn_801406E0_view1(14, in[0]);
            fn_8013BDE4(&in, 14, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(14, in[0]);
        result = fn_80140778_view1(in, 14, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013CEB0(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x382 == 255) {
            in += (u8)fn_801406E0_view1(17, in[0]);
            fn_8013BDE4(&in, 17, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(17, in[0]);
        result = fn_80140778_view1(in, 17, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013D4C4(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (em_alt_mode_ck(self) == 0) {
            in += (u8)fn_801406E0_view1(28, in[0]);
            fn_8013BDE4(&in, 28, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(28, in[0]);
        result = fn_80140778_view1(in, 28, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013D694(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->value_0x44C > 0) {
            in += (u8)fn_801406E0_view1(31, in[0]);
            fn_8013BDE4(&in, 31, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(31, in[0]);
        result = fn_80140778_view1(in, 31, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013D80C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x916 <= 0) {
            in += (u8)fn_801406E0_view1(33, in[0]);
            fn_8013BDE4(&in, 33, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(33, in[0]);
        result = fn_80140778_view1(in, 33, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013D9B8(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (fn_8012EC3C(self) == 0) {
            in += (u8)fn_801406E0_view1(36, in[0]);
            fn_8013BDE4(&in, 36, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(36, in[0]);
        result = fn_80140778_view1(in, 36, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013DA7C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0: {
        _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
        if (data == 0 || data->mode_0x14 != self->area_no) {
            in += (u8)fn_801406E0_view1(37, in[0]);
            fn_8013BDE4(&in, 37, &result);
        }
        break;
    }
    case 2:
        in += (u8)fn_801406E0_view1(37, in[0]);
        result = fn_80140778_view1(in, 37, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013DC58(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0: {
        _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
        if (data == 0 || data->mode_0x17 == 255) {
            in += (u8)fn_801406E0_view1(39, in[0]);
            fn_8013BDE4(&in, 39, &result);
        }
        break;
    }
    case 2:
        in += (u8)fn_801406E0_view1(39, in[0]);
        result = fn_80140778_view1(in, 39, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013DD48(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if ((self->field_0x1C8 & 0x8) == 0) {
            in += (u8)fn_801406E0_view1(40, in[0]);
            fn_8013BDE4(&in, 40, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(40, in[0]);
        result = fn_80140778_view1(in, 40, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013DF78(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if ((u16)(calcVecAng2(&self->pos, &self->vec_0x36C) -
                  self->field_0x1C0) >= 0x8000) {
            in += (u8)fn_801406E0_view1(46, in[0]);
            fn_8013BDE4(&in, 46, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(46, in[0]);
        result = fn_80140778_view1(in, 46, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013E180(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0: {
        _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
        in += (u8)fn_801406E0_view1(50, in[0]);
        if (data == 0) {
            fn_80140AF8_view1(self, 8, 50);
            fn_8013BDE4(&in, 50, &result);
        } else if (fn_80345A6C(data, data->field_0x36, &self->pos, lbl_80796DA0) == 0) {
            fn_8013BDE4(&in, 50, &result);
        }
        break;
    }
    case 2:
        in += (u8)fn_801406E0_view1(50, in[0]);
        result = fn_80140778_view1(in, 50, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013E388(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->value_0x452 < (s16)(in[2] * 30 + in[1] * 1800)) {
            in += (u8)fn_801406E0_view1(52, in[0]);
            fn_8013BDE4(&in, 52, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(52, in[0]);
        result = fn_80140778_view1(in, 52, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013E464(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (em_mot_finished_ck_c1() == 0) {
            in += (u8)fn_801406E0_view1(53, in[0]);
            fn_8013BDE4(&in, 53, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(53, in[0]);
        result = fn_80140778_view1(in, 53, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013E700(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->value_0x452 < self->value_0x44E) {
            in += (u8)fn_801406E0_view1(56, in[0]);
            fn_8013BDE4(&in, 56, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(56, in[0]);
        result = fn_80140778_view1(in, 56, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

/* --------------------------------------------------------------------------------------------- */
/* The stream readers: `switch (in[0])` over the command byte, each case advancing the stream
 * through `fn_801406E0`/`fn_8013BDE4` and reporting a `s16` back through the stack slot. */
/* --------------------------------------------------------------------------------------------- */
s16 fn_8013E900(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if ((u8)PlayMode_ck() != 2) {
            in += (u8)fn_801406E0_view1(59, in[0]);
            fn_8013BDE4(&in, 59, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(59, in[0]);
        result = fn_80140778_view1(in, 59, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013EAE4(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x1FC == 0) {
            in += (u8)fn_801406E0_view1(63, in[0]);
            fn_8013BDE4(&in, 63, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(63, in[0]);
        result = fn_80140778_view1(in, 63, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013EBC8(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x8A2 > in[1] * 10 && fn_8012EC3C(self) == 0) {
            in += (u8)fn_801406E0_view1(65, in[0]);
            fn_8013BDE4(&in, 65, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(65, in[0]);
        result = fn_80140778_view1(in, 65, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013ECA4(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (fn_80132184() == 0) {
            in += (u8)fn_801406E0_view1(66, in[0]);
            fn_8013BDE4(&in, 66, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(66, in[0]);
        result = fn_80140778_view1(in, 66, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013ED68(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (fn_80131BD4_c1() == 0) {
            in += (u8)fn_801406E0_view1(67, in[0]);
            fn_8013BDE4(&in, 67, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(67, in[0]);
        result = fn_80140778_view1(in, 67, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

/* Forwards its arguments to the second parameter interpreter. */
void fn_8013E068(_ENEMY_WORK *self, u8 *in, u32 id, u32 sub, s32 value) {
    fn_801409C8_view1(self, in, id, sub, value);
}

/* --------------------------------------------------------------------------------------------- */
/* The record-based entries. */
/* --------------------------------------------------------------------------------------------- */

/* True when the angle between the record's position and its target is at or below the per-action
 * threshold indexed by the sub-state. */
u32 fn_8013C254(_ENEMY_WORK *self) {
    f32 angle = calcVecDistXZ(&self->pos, &self->vec_0x36C);
    _ENEMY_TABLE *table = (_ENEMY_TABLE *)self->recs_0x9A4;

    return angle <= table->entries[self->field_0x382].value_0x04;
}

/* Finds the action entry matching the current action and returns the item entry the sub-state
 * selects, or 0 when the enemy has no data or either list runs out. */
u32 fn_8013C2B0(_ENEMY_WORK *self) {
    if (self->field_0x1E0 != 255) {
        _ENEMY_DATA *data = (_ENEMY_DATA *)get_enemy_data(self);
        _ENEMY_LIST_HEAD *head = (_ENEMY_LIST_HEAD *)data->ptr_0x18;

        if (head != 0 && head->first != 0) {
            _ENEMY_ACTION *act = head->first;

            for (; act->id != 255; act++) {
                if (fn_80125FF0_c1(self->field_0x1E0, act->id) == 1) {
                    _ENEMY_ITEM *item = act->items;

                    for (; item->id != 255; item++) {
                        if (item->id == self->area_no) {
                            return (u32)&item->entries[self->state_0x9FB * 8];
                        }
                    }
                    break;
                }
            }
        }
    }
    return 0;
}

/* Pushes an 8-byte action slot (the current action pair plus `param`) onto the queue at +0x9AC and
 * re-seeds the current action pair when the push does not take. */
void fn_8013C36C(_ENEMY_WORK *self, u8 value, u32 param) {
    if (self->count_0x9A8 >= 6) {
        fn_80140AF8_view1(self, 15, 0);
        self->count_0x9A8--;
    }
    self->recs_0x9AC[self->count_0x9A8].code = self->field_0x95C;
    self->recs_0x9AC[self->count_0x9A8].sub = self->field_0x95D;
    self->recs_0x9AC[self->count_0x9A8].value = param;
    if (fn_8013AB74(self, 1, value) == 0) {
        fn_80140AF8_view1(self, 3, self->field_0x95C);
        self->field_0x95C = self->recs_0x9AC[self->count_0x9A8].code;
        self->field_0x95D = self->recs_0x9AC[self->count_0x9A8].sub;
    }
    self->count_0x9A8++;
}

/* Chooses the sub-command the current combat state maps to and applies it. */
void fn_8013C458(_ENEMY_WORK *self, u8 *in) {
    u8 value = in[0];
    u8 sub = in[1];

    if (value == 0 && sub == 255) {
        switch (self->field_0x1E2) {
        case 2:
            if (fn_8012EC3C(self) == 1 && (self->field_0x1C8 & 0x40) != 0) {
                sub = 5;
            } else {
                sub = 4;
            }
            break;
        case 1:
            switch (self->field_0x1E3) {
            case 2:
                sub = 8;
                break;
            case 1:
                sub = 9;
                break;
            default:
                sub = 3;
                break;
            }
            break;
        case 3:
            sub = 6;
            break;
        case 4:
            sub = 7;
            break;
        default:
            if (fn_8012EC3C(self) == 1 && (self->field_0x1C8 & 0x40) != 0) {
                sub = 2;
            } else {
                sub = (self->field_0x43D == 1);
            }
            break;
        }
    }
    em_act_step_arm(self, value, sub, 0);
}

/* --------------------------------------------------------------------------------------------- */
/* The remaining action entries. */
/* --------------------------------------------------------------------------------------------- */

/* Recomputes the current action's parameter ids from the type byte and the preset byte. */
void fn_8013C7E0(_ENEMY_WORK *self, u8 *in) {
    switch (in[0]) {
    default:
        self->field_0x9F8 = 255;
        self->field_0x9F7 = 255;
        break;
    case 1:
        self->field_0x9F8 = in[1];
        if (self->field_0x9F8 == 253) {
            self->field_0x9F8 = self->field_0x1FF;
        }
        self->field_0x9F7 = fn_80126DAC(self, self->area_no, self->field_0x9F8);
        break;
    case 2: {
        u8 preset = in[1];
        u32 current;

        if (preset == 253) {
            preset = self->field_0x1FF;
        }
        current = fn_80126DAC(self, self->area_no, preset);
        self->field_0x9F7 = current;
        self->field_0x9F8 = preset;
        while ((u8)current != preset) {
            if (fn_80126F80(self, self->field_0x1E0, (u8)current) == 1) {
                self->field_0x9F8 = current;
                break;
            }
            current = fn_80126DAC(self, (u8)current, preset);
        }
        break;
    }
    case 4: {
        u32 id;
        _ENEMY_DATA *entry = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A),
                                                            self->field_0x46C);

        if (entry != 0) {
            id = eft_slot_effect_key((struct EftSlot *)entry);
            self->field_0x9F8 = id;
            self->field_0x9F7 = fn_80126DAC(self, self->area_no, (u8)id);
        } else {
            id = fn_801272C4(self, &self->pos, self->area_no);
            self->field_0x9F7 = id;
            self->field_0x9F8 = id;
        }
        break;
    }
    case 3: {
        u32 id = fn_801272C4(self, &self->pos, self->area_no);

        self->field_0x9F7 = id;
        self->field_0x9F8 = id;
        break;
    }
    case 5:
        self->field_0x9F7 = 254;
        self->field_0x9F8 = 254;
        break;
    }
}

/* Copies the position the record aims at into the aim vector. */
void fn_8013D41C(_ENEMY_WORK *self, u8 *in) {
    switch (in[0]) {
    case 0:
        copyVec3(&self->aim, &self->pos);
        break;
    case 1:
        copyVec3(&self->aim, &self->vec_0x36C);
        break;
    default:
        fn_80140AF8_view1(self, 9, 0);
        break;
    }
}

/* Sets the aim timer and the motion blend from a parameter record, then re-runs the movement entry. */
void fn_8013D45C(_ENEMY_WORK *self, u8 *in, u32 flag) {
    self->field_0x383 = 1;
    self->field_0x384 = (f32)(in[3] * 100);
    if (in[4] == 1) {
        self->field_0x383 |= 0x80;
    }
    fn_8013C57C(self, in, (u8)flag);
}

/* True while the difference between the stored facing and the vector to the target is greater than the
 * tolerance the record carries. */
void fn_8013D588(_ENEMY_WORK *self, u8 *in, u32 unused) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if ((s16)(calcVecAng2(&self->pos, &self->vec_0x36C) -
                  self->field_0x1C0) > in[1] * 256) {
            in += (u8)fn_801406E0_view1(29, in[0]);
            fn_8013BDE4(&in, 29, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0_view1(29, in[0]);
        result = fn_80140778_view1(in, 29, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
}

/* Queues a sub-action pointer for the movement entry, or re-seeds the current action when the queue
 * is already occupied. */
void fn_8013D750(_ENEMY_WORK *self, u8 *in) {
    u8 value = in[0];
    u8 *next = in + 1;

    if (self->field_0x95C != 0) {
        fn_80140AF8_view1(self, 10, self->field_0x95C);
        self->field_0x958 = (u32)next;
    } else {
        if (self->state_0x962 >= 10) {
            fn_80140AF8_view1(self, 11, 0);
            self->state_0x962--;
        }
        self->bytes_0x963[self->state_0x962] = self->field_0x95D;
        self->values_0x970[self->state_0x962] = (u32)next;
        self->state_0x962++;
        fn_8013AB74(self, 0, value);
    }
}

/* Sends the parameter's remainder modulo the record's step count. */
void fn_8013D8C8(_ENEMY_WORK *self, u8 *in, u32 param) {
    if (in[0] == 0) {
        if (fn_8013BE60(self, in, 34, 11, 1) >= 0) {
            return;
        }
        {
            u8 *next = in + (u8)fn_801406E0_view1(34, in[0]);
            s16 step = fn_80140778_view1(next, 34, 2);

            fn_8013BE60(self, in, 34, 10, (s16)((u16)param % step));
        }
        return;
    }
    fn_8013C244(self, in, 34);
}

/* Moves the converted target position into the record's position and remembers the old one. */
void fn_8013DE80(_ENEMY_WORK *self, u8 *in) {
    fn_80126278(self, (u16)(((self->area_no & 0xF) << 8) | in[0]), &self->pos);
    copyVec3(&self->prev_pos, &self->pos);
}

/* Rebuilds the aim vector from a parameter record and stores the facing angle and zero offsets. */
void fn_8013DEFC(_ENEMY_WORK *self, u8 *in) {
    Vec3 v;

    VEC3_ctor(&v);
    fn_80126278(self, (u16)(((self->area_no & 0xF) << 8) | in[0]), &v);
    self->field_0x1BC = 0;
    self->field_0x1C0 = calcVecAng2(&self->pos, &v);
    self->field_0x1C4 = 0;
}

/* Sends the horizontal bearing to the target as a byte-scaled angle. */
void fn_8013E2B0(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        f32 dx = self->vec_0x36C.x - self->pos.x;
        f32 dz = self->vec_0x36C.z - self->pos.z;
        f32 dist = sqrt_f32(dx * dx + dz * dz);
        f32 angle = atan2f(-(self->vec_0x36C.y - self->pos.y), dist);
        u16 raw = (u16)(s32)(lbl_80796DA8 * angle / lbl_80796DAC + lbl_80796DA4);

        fn_8013BE60(self, in, 51, 8, (s16)(u8)(((s32)raw >> 8) + 64));
    } else {
        fn_8013C244(self, in, 51);
    }
}

/* Starts, advances and retires a repeated parameter record; returns -1 while one is still active. */
s16 fn_8013E06C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        self->state_0x9F4 = in[1];
        self->state_0x9F5 = in[2];
        self->ptr_0x9F0 = (u32)(in + 3);
        break;
    case 3:
        self->state_0x9F4 = 255;
        in += (u8)fn_801406E0_view1(49, in[0]);
        result = fn_80140778_view1(in, 49, 1);
        break;
    case 255:
        switch (self->state_0x9F4) {
        default:
            fn_80140AF8_view1(self, 14, self->state_0x9F4);
            break;
        case 0:
            result = -1;
            self->field_0x958 = self->ptr_0x9F0;
            break;
        case 1:
            self->state_0x9F5--;
            if (self->state_0x9F5 != 0) {
                result = -1;
                self->field_0x958 = self->ptr_0x9F0;
            }
            break;
        case 255:
            break;
        }
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}

/* Reads the data record's action field or, when there is none, the record's own parsed value. */
s16 fn_8013E9FC(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A),
                                                   self->field_0x46C);

    if (data == 0) {
        fn_80140AF8_view1(self, 8, 62);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0_view1(62, in[0]);
            result = fn_80140778_view1(in, 62, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 62, 0, data->field_0x0B);
    } else {
        fn_8013C244(self, in, 62);
    }
}

/* Same reader as `fn_8013E9FC`, for the action id 69 and the data record's +0x0D field. */
s16 fn_8013EE2C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A),
                                                   self->field_0x46C);

    if (data == 0) {
        fn_80140AF8_view1(self, 8, 18);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0_view1(69, in[0]);
            result = fn_80140778_view1(in, 69, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 69, 2, data->field_0x0D);
    } else {
        fn_8013C244(self, in, 69);
    }
}

/* Same reader as `fn_8013E9FC`, for the action id 38 and the data record's +0x08 field. */
s16 fn_8013DB70(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A),
                                                   self->field_0x46C);

    if (data == 0) {
        fn_80140AF8_view1(self, 8, 38);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0_view1(38, in[0]);
            result = fn_80140778_view1(in, 38, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 38, 0, data->field_0x08);
    } else {
        fn_8013C244(self, in, 38);
    }
}

/* Same reader as `fn_8013E9FC`, for the action id 21 and the data record's +0x0F field. */
s16 fn_8013D1E0(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A),
                                                   self->field_0x46C);

    if (data == 0) {
        fn_80140AF8_view1(self, 8, 21);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0_view1(21, in[0]);
            result = fn_80140778_view1(in, 21, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 21, 0, data->mode_0x0F);
    } else {
        fn_8013C244(self, in, 21);
    }
}

/* Selects the combat sub-command the data record's mode byte names. */
void fn_8013D34C(_ENEMY_WORK *self) {
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A),
                                                   self->field_0x46C);

    if (data == 0) {
        fn_80140AF8_view1(self, 8, 25);
        return;
    }
    switch (data->mode_0x0F) {
    case 0:
        fn_8012B380(self, 1, 2, data->value_0x10);
        break;
    case 1:
        fn_8012B380(self, 2, 2, data->value_0x10);
        break;
    case 2:
        fn_8012B380(self, 3, 2, data->value_0x10);
        break;
    default:
        break;
    }
    em_target_pos_set(self, 0);
}

/* --------------------------------------------------------------------------------------------- */
/* The aim and action-queue entries. */
/* --------------------------------------------------------------------------------------------- */

/* Re-projects the position of the aim target and either applies it or sends the failure code. */
void fn_8013C988(_ENEMY_WORK *self, u8 *in) {
    Vec3 a;
    Vec3 b;
    Vec3 c;
    Vec3 d;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    switch (in[0]) {
    case 0:
        if (self->field_0x9F7 != 255) {
            self->field_0x1F9 = 1;
            self->field_0x1FA = 1;
            fn_8012B380(self, 9, self->field_0x9F7, 255);
            em_target_pos_set(self, 0);
        }
        break;
    case 255:
        if (self->field_0x9F7 != self->area_no && self->field_0x9F7 != 255) {
            get_worldworld_pos_out(&c, &self->pos, self->area_no);
            copyVec3(&a, &c);
            fn_802B01AC(&d, &a, self->field_0x9F7);
            copyVec3(&self->pos, &d);
            self->pos.y = fn_802B0430(self->field_0x9F7);
            em_area_change(self, 0);
            em_target_pos_set(self, 0);
        }
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
}

/* Starts a repeated parameter record and restores the action pair when it cannot be served. */
void fn_8013CAA0(_ENEMY_WORK *self, u8 *in) {
    u32 value;

    self->state_0x999 = 0;
    self->recs_0x9A4 = 0;
    self->state_0x998 = in[0];
    self->field_0x99A = in[1];
    self->state_0x99C = self->field_0x95C;
    self->state_0x99D = self->field_0x95D;
    self->ptr_0x9A0 = (u32)(in + 2);
    if (fn_8013A884(self, self->field_0x95C) == 1) {
        fn_8012B380(self, 8, self->state_0x998, 255);
        if (self->recs_0x9A4 != 0) {
            value = ((_ENEMY_TABLE *)self->recs_0x9A4)->entries[self->field_0x382].value_0x08;
        }
        if (fn_8013AB74(self, 2, (u8)value) == 1) {
            em_target_pos_set(self, 0);
            if (self->recs_0x9A4 != 0 &&
                (u32)((*(u8 *)self->recs_0x9A4) - 252) > 3) {
                self->field_0x1F9 = 1;
                self->field_0x1FA = 1;
            }
            return;
        }
    }
    fn_80140AF8_view1(self, 3, self->field_0x95C);
    fn_80140B10_view1(self, 0, self->team);
    if (self->field_0x9F7 != 255) {
        em_area_change(self, 0);
    }
    self->field_0x95C = self->state_0x99C;
    self->field_0x95D = self->state_0x99D;
    self->field_0x958 = self->ptr_0x9A0;
}

/* Applies a motion blend record and asks the movement entry for the blended piece. */
void fn_8013CDB4(_ENEMY_WORK *self, u8 *in) {
    u32 piece;

    self->field_0x1E7 = in[0];
    if (in[1] != 0) {
        self->field_0x383 = 1;
        self->field_0x384 = (f32)(in[1] * 100);
        if (in[2] == 1) {
            self->field_0x383 |= 0x80;
        }
    }
    piece = fn_801275F0(self, 1);
    if ((u8)fn_80127A7C(self, (u8)piece) == 0) {
        fn_8012B380(self, 10, 1, (u8)piece);
    } else {
        fn_8012B380(self, 10, 2, (u8)piece);
    }
    em_target_pos_set(self, 0);
    self->field_0x1E7 = in[0];
}

/* Same reader as `fn_8013D1E0`, for the action id 18 and the data record's +0x0E field. */
s16 fn_8013CF6C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A),
                                                   self->field_0x46C);

    if (data == 0) {
        fn_80140AF8_view1(self, 8, 18);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0_view1(18, in[0]);
            result = fn_80140778_view1(in, 18, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 18, 2, data->field_0x0E);
    } else {
        fn_8013C244(self, in, 18);
    }
}

/* Sends the combat sub-command the data record's aim fields select. */
s16 fn_8013D054(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->team, self->field_0x00A),
                                                   self->field_0x46C);

    if (data == 0) {
        fn_80140AF8_view1(self, 8, 19);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0_view1(19, in[0]);
            result = fn_80140778_view1(in, 19, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        u32 sub;
        s16 value;

        switch (in[1]) {
        case 0:
            value = self->state_0x46D;
            sub = 3;
            break;
        case 1:
            value = self->state_0x46E;
            sub = 3;
            break;
        case 2:
            if (self->state_0x46F == 255 ||
                self->state_0x46E > data->field_0x39 - 1) {
                if (self->state_0x46E > data->field_0x37) {
                    self->state_0x46F = 255;
                } else {
                    self->state_0x46F = 0;
                }
            }
            value = self->state_0x46F;
            sub = 2;
            break;
        default:
            fn_80140AF8_view1(self, 12, 0);
            value = 0;
            sub = 3;
            break;
        }
        fn_8013BE60(self, in, 19, sub, value);
    } else {
        fn_8013C244(self, in, 19);
    }
}

/* Starts or advances a repeated parameter record against the current enemy data. */
s16 fn_8013E7C0(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0: {
        _ENEMY_OTHER *other = (_ENEMY_OTHER *)fn_80130DF8();

        if (other != 0) {
            if (fn_8013A884(self, 10) == 1) {
                self->field_0x9E8 = self->field_0x95C;
                self->field_0x9E9 = self->field_0x95D;
                self->field_0x9EC = (u32)(in + 1);
                fn_8013AB74(self, 10, 0);
            }
            fn_8012B380(self, 7, other->field_0x05, other->field_0x06);
            em_target_pos_set(self, 0);
        } else {
            in += (u8)fn_801406E0_view1(58, in[0]);
            fn_8013BDE4(&in, 58, &result);
        }
        break;
    }
    case 2:
        in += (u8)fn_801406E0_view1(58, in[0]);
        result = fn_80140778_view1(in, 58, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8_view1(self, 0, 0);
        break;
    }
    return result;
}
}

#pragma fp_contract on

extern "C" {
/* ------------------------------------------------------------------------------------------------
 * the run driver
 * ------------------------------------------------------------------------------------------------ */
u8 fn_8013F764(_ENEMY_WORK* self, u8* in) {
    EmProgEntry* entry;
    u8 first = in[0];
    u8* p = in + 1;
    u8 ret = 0;
    EmProgTbl* prog = self->prog_0xA00;

    if (prog != NULL) {
        if (self->field_0x9FD & 1) {
            entry = &prog->entries_0x04[prog->count_0x00 - 1 - self->field_0x9FC];
        } else {
            entry = &prog->entries_0x04[self->field_0x9FC];
        }
        if (self->field_0x9FE == 0) {
            EmActionInfo* info = (EmActionInfo*)(void*)fn_801262BC(self, entry->code);
            self->field_0x383 = 1;
            self->field_0x384 = info->value_0x04;
            fn_8012B380_view1(self, 5, 8, info->field_0x00);
            em_target_pos_set(self, 0);
            self->field_0x9FE++;
        }
        if (calcVecDistXZ_view1(&self->pos, &self->vec_0x36C) < lbl_80796DBC) {
            self->field_0x9FE = 2;
        } else {
            self->field_0x9FE = 1;
        }
        switch (self->field_0x9FE) {
        case 1:
            fn_8013C36C_view1(self, entry->field_0x01, p);
            ret = 1;
            break;
        case 2:
            fn_8013C36C_view1(self, entry->field_0x02, p);
            ret = 1;
            if (first == 0) {
                fn_8013F994(self);
            }
            break;
        }
    }
    self->field_0x9FF = 1;
    return ret;
}

void fn_8013F994(_ENEMY_WORK* self) {
    EmProgTbl* prog = self->prog_0xA00;

    if (prog == NULL) {
        return;
    }
    if (self->field_0x9FE != 2) {
        return;
    }
    self->field_0x9FC++;
    if ((u8)self->field_0x9FC >= prog->count_0x00) {
        fn_8013AA00(self);
        return;
    }
    self->field_0x9FE = 0;
}

/* ------------------------------------------------------------------------------------------------
 * the stream readers (command id in the trailing comment)
 * ------------------------------------------------------------------------------------------------ */
s16 fn_8013F8D8(_ENEMY_WORK* self, u8* in) {                    /* 0x4E */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->prog_0xA00 == NULL) {
            in += fn_801406E0(0x4E, in[0]);
            fn_8013BDE4_view1(&in, 0x4E, &value);
        }
        break;
    case 2:
        in += fn_801406E0(0x4E, in[0]);
        value = fn_80140778(in, 0x4E, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_8013F9D8(_ENEMY_WORK* self, u8* in) {                    /* 0x50 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (em_break_state_ck(self) != 0) {
            break;
        }
        in += fn_801406E0(0x50, in[0]);
        fn_8013BDE4_view1(&in, 0x50, &value);
        break;
    case 2:
        in += fn_801406E0(0x50, in[0]);
        value = fn_80140778(in, 0x50, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_8013FA9C(_ENEMY_WORK* self, u8* in) {                   /* 0x51, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x51, 0, (s16)(u8)fn_8013023C_view1(self));
    } else {
        fn_8013C244(self, in, 0x51);
    }
}

void fn_8013FB08(_ENEMY_WORK* self, u8* in) {                   /* 0x52, two-argument */
    if (in[0] == 0) {
        EmCmdRec* rec = self->recs_0x9A4;
        if (rec != NULL) {
            fn_8013BE60(self, in, 0x52, 0, (s16)rec->code);
        } else {
            fn_80140AF8(self, 0x11, self->field_0x95C);
            in += fn_801406E0(0x52, in[0]);
            fn_80140778(in, 0x52, 1);
        }
    } else {
        fn_8013C244(self, in, 0x52);
    }
}

s16 fn_8013FB9C(_ENEMY_WORK* self, u8* in) {                    /* 0x53 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_801321B0_view1(self) != 0) {
            break;
        }
        in += fn_801406E0(0x53, in[0]);
        fn_8013BDE4_view1(&in, 0x53, &value);
        break;
    case 2:
        in += fn_801406E0(0x53, in[0]);
        value = fn_80140778(in, 0x53, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_8013FC60(_ENEMY_WORK* self, u8* in) {                    /* 0x54 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x1FD != 0) {
            break;
        }
        in += fn_801406E0(0x54, in[0]);
        fn_8013BDE4_view1(&in, 0x54, &value);
        break;
    case 2:
        in += fn_801406E0(0x54, in[0]);
        value = fn_80140778(in, 0x54, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_8013FD1C(_ENEMY_WORK* self, u8* in) {                   /* 0x55, field setter */
    self->field_0x382 = in[0];
}

void fn_8013FD28(_ENEMY_WORK* self, u8* in) {                   /* 0x56, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x56, 0, (s16)self->field_0x9F8);
    } else {
        fn_8013C244(self, in, 0x56);
    }
}

void fn_8013FD50(_ENEMY_WORK* self, u8* in) {                   /* 0x57, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x57, 0x0C, 0);
    } else {
        fn_8013C244(self, in, 0x57);
    }
}

void fn_8013FD74(_ENEMY_WORK* self, u8* in) {                   /* 0x58, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x58, 0x0D, 0);
    } else {
        fn_8013C244(self, in, 0x58);
    }
}

s16 fn_8013FD98(_ENEMY_WORK* self, u8* in) {                    /* 0x59 */
    s16 value = 0;

    switch (in[0]) {
    case 0: {
        u8 found = 0;
        if (self->team != 0x12 || self->field_0x360 <= 0) {
            u16 count = get_move_work_max(2);
            EmAreaWork* rec = (EmAreaWork*)get_move_work_adrs(2);
            for (u16 i = 0; i < count; i++) {
                if (rec->active != 0 && rec->area_no == self->area_no && fn_8027D530(rec) != 0) {
                    found = 1;
                    break;
                }
                rec++;
            }
        }
        if (found == 0) {
            in += fn_801406E0(0x59, in[0]);
            fn_8013BDE4_view1(&in, 0x59, &value);
        }
        break;
    }
    case 2:
        in += fn_801406E0(0x59, in[0]);
        value = fn_80140778(in, 0x59, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_8013FEF4(_ENEMY_WORK* self, u8* in) {                   /* 0x5A, stack push */
    self->stack_0x961[in[0]] = in[1];
}

void fn_8013FF08(_ENEMY_WORK* self, u8* in) {                   /* 0x5B, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x5B, 0, (s16)self->stack_0x961[in[1]]);
    } else {
        fn_8013C244(self, in, 0x5B);
    }
}

void fn_8013FF38(_ENEMY_WORK* self, u8* in) {                   /* 0x5C, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x5C, 0x0E, 0);
    } else {
        fn_8013C244(self, in, 0x5C);
    }
}

s16 fn_8013FF5C(_ENEMY_WORK* self, u8* in) {                    /* 0x5E */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x9F7 == self->area_no) {
            break;
        }
        in += fn_801406E0(0x5E, in[0]);
        fn_8013BDE4_view1(&in, 0x5E, &value);
        break;
    case 2:
        in += fn_801406E0(0x5E, in[0]);
        value = fn_80140778(in, 0x5E, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_8014001C(_ENEMY_WORK* self, u8* in) {                    /* 0x5F, two-argument with a ratio */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x5F, 2,
                    (s16)(s32)(lbl_80796DC0 * ((f32)self->field_0x7A0 / (f32)self->field_0x7A4)));
    } else {
        fn_8013C244(self, in, 0x5F);
    }
}

void fn_801400B0(_ENEMY_WORK* self, u8* in) {                   /* 0x60, field setter */
    self->field_0x1E7 = in[0];
}

s16 fn_801400BC(_ENEMY_WORK* self, u8* in) {                    /* 0x61 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x1F9 != 0) {
            break;
        }
        in += fn_801406E0(0x61, in[0]);
        fn_8013BDE4_view1(&in, 0x61, &value);
        break;
    case 2:
        in += fn_801406E0(0x61, in[0]);
        value = fn_80140778(in, 0x61, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_80140178(_ENEMY_WORK* self, u8* in) {                    /* 0x62 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_802B0998(in[1]) != 0) {
            break;
        }
        in += fn_801406E0(0x62, in[0]);
        fn_8013BDE4_view1(&in, 0x62, &value);
        break;
    case 2:
        in += fn_801406E0(0x62, in[0]);
        value = fn_80140778(in, 0x62, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_80140244(_ENEMY_WORK* self, u8* in) {                   /* 0x63, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x63, 0, (s16)self->field_0x010);
    } else {
        fn_8013C244(self, in, 0x63);
    }
}

void fn_8014026C(_ENEMY_WORK* self, u8* in, u32 arg) {           /* 0x65, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x65, 0, (s16)(arg & 0xFF));
    } else {
        fn_8013C244(self, in, 0x65);
    }
}

s16 fn_80140298(_ENEMY_WORK* self, u8* in) {                    /* 0x66 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (self->field_0x1F8 != 0) {
            break;
        }
        in += fn_801406E0(0x66, in[0]);
        fn_8013BDE4_view1(&in, 0x66, &value);
        break;
    case 2:
        in += fn_801406E0(0x66, in[0]);
        value = fn_80140778(in, 0x66, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_80140354(_ENEMY_WORK* self, u8* in) {                    /* 0x67 */
    s16 value = 0;

    switch (in[0]) {
    case 0: {
        u8 hit_team = in[1];
        u8 found = 0;
        u16 count = get_move_work_max(3);
        _ENEMY_WORK* rec = (_ENEMY_WORK*)get_move_work_adrs(3);
        for (u16 i = 0; i < count; i++) {
            if (self->group != i && rec->active != 0 && rec->team == hit_team &&
                (rec->field_0x1C8 & 8) != 0) {
                found = 1;
                break;
            }
            rec++;
        }
        if (found == 0) {
            in += fn_801406E0(0x67, in[0]);
            fn_8013BDE4_view1(&in, 0x67, &value);
        }
        break;
    }
    case 2:
        in += fn_801406E0(0x67, in[0]);
        value = fn_80140778(in, 0x67, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_801404B8(_ENEMY_WORK* self, u8* in) {                    /* 0x68 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_80137704_c1(self, 1) != 0) {
            break;
        }
        in += fn_801406E0(0x68, in[0]);
        fn_8013BDE4_view1(&in, 0x68, &value);
        break;
    case 2:
        in += fn_801406E0(0x68, in[0]);
        value = fn_80140778(in, 0x68, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

s16 fn_80140580(_ENEMY_WORK* self, u8* in) {                    /* 0x69 */
    s16 value = 0;

    switch (in[0]) {
    case 0:
        if (fn_80137704_c1(self, 2) != 0) {
            break;
        }
        in += fn_801406E0(0x69, in[0]);
        fn_8013BDE4_view1(&in, 0x69, &value);
        break;
    case 2:
        in += fn_801406E0(0x69, in[0]);
        value = fn_80140778(in, 0x69, 1);
        break;
    case 0xFF:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return value;
}

void fn_80140648(_ENEMY_WORK* self, u8* in) {                   /* 0x6A, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x6A, 2, (s16)self->field_0x7C8);
    } else {
        fn_8013C244(self, in, 0x6A);
    }
}

void fn_80140670(_ENEMY_WORK* self, u8* in) {                   /* 0x6C, two-argument */
    if (in[0] == 0) {
        fn_8013BE60(self, in, 0x6C, 0, (s16)(u8)fn_801358D0(self, in[1]));
    } else {
        fn_8013C244(self, in, 0x6C);
    }
}

/* ------------------------------------------------------------------------------------------------
 * the stream helpers
 * ------------------------------------------------------------------------------------------------ */
u8 fn_801406E0(u32 code, u32 sub) {
    u8 len;

    if ((u8)code < 0x6D) {
        len = lbl_805A1530[(u8)code];
        if ((len & 0x80) == 0) {
            return len;
        }
        switch ((u8)sub) {
        case 0:
            return (len + 1) & 0xF;
        case 1:
            switch (len & 0x30) {
            case 0xA0:
                return 4;
            case 0xB0:
                return 5;
            default:
                return 2;
            }
        default:
            return 1;
        }
    }
    return 0xFF;
}

u8 fn_80140768(u8* in) {
    return fn_801406E0(in[0], in[1]);
}

s16 fn_80140778(u8* in, u8 code, u8 mode) {
    s16 result = 0;
    s16 prev = 0;
    u8 stop = 0;
    u16 pos = 0;
    u8 want = code;

    for (;;) {
        if (in[0] == 0xFF) {
            fn_80140B0C(2, code, 0);
            break;
        }
        if (in[0] == want) {
            if (in[1] == 0) {
                u8 skip = fn_80140768(in) + 1;
                s16 off = fn_80140778(in + skip, code, 1) + skip;
                pos += off;
                in += off;
                continue;
            }
            switch (mode) {
            case 0:
                stop = 1;
                result = pos;
                break;
            case 1:
                if (in[1] == 0xFF) {
                    stop = 1;
                    result = pos;
                }
                break;
            case 2:
                if (in[1] == 0xFF) {
                    stop = 1;
                    result = prev;
                } else {
                    prev++;
                }
                break;
            }
        }
        if (stop == 0) {
            u8 skip = fn_80140768(in) + 1;
            pos += skip;
            in += skip;
        }
        if (stop != 0) {
            break;
        }
    }
    return result;
}

void fn_80140AF8(_ENEMY_WORK* self, u32 a, u8 b) {
    u8 a8 = a;
    fn_80140B0C(a8, b, self->team);
}

void fn_80140B0C(u8 a, u8 b, u8 c) {
}

void fn_80140B10(u32 unused, u32 a, u32 b) {
    em_prog_trace2((u8)a, b);
}

u8 fn_801408B4(_ENEMY_WORK* self) {
    switch (self->field_0x95C) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 0xB:
        self->field_0x95C = self->field_0x9DC;
        self->field_0x95D = self->field_0x9DD;
        self->field_0x958 = self->field_0x9E0;
        return 1;
    case 0: {
        u8 depth;
        if (self->stack_0x961[1] == 0) {
            return 0;
        }
        self->stack_0x961[1]--;
        depth = self->stack_0x961[1];
        self->field_0x95D = self->stack_0x961[2 + depth];
        self->field_0x958 = self->values_0x970[depth];
        return 1;
    }
    case 2:
        self->field_0x95C = self->rec_0x99C.code;
        self->field_0x95D = self->rec_0x99C.sub;
        self->field_0x958 = self->rec_0x99C.value;
        return 1;
    case 1: {
        u8 idx;
        self->count_0x9A8--;
        idx = self->count_0x9A8;
        self->field_0x95C = self->recs_0x9AC[idx].code;
        self->field_0x95D = self->recs_0x9AC[idx].sub;
        self->field_0x958 = self->recs_0x9AC[idx].value;
        return 1;
    }
    case 0xA:
        self->field_0x95C = self->field_0x9E8;
        self->field_0x95D = self->field_0x9E9;
        self->field_0x958 = self->field_0x9EC;
        return 1;
    default:
        return 0;
    }
}

void fn_801409C8(_ENEMY_WORK* self) {
    struct EnemyData* data = get_enemy_data(self);
    s32 value;

    if (quest_flag_100000_ck(0) == 1 || quest_flag_10_ck(0) == 1) {
        value = 0x3E8;
    } else {
        EmActList* list = fn_80126494(self);
        if (list == NULL) {
            value = 0x1F4;
        } else {
            u8 act = self->field_0x7C8;
            EmActRec* rec = list->entries_0x08;
            value = 0;
            for (; rec->code != 0xFF; rec++) {
                if (act > rec->code) {
                    value = rec->value;
                }
            }
        }
    }
    fn_80130CDC_view1(self, value);
    em_motion_mode_set_c1(self, 0);
    {
        s16* limit = ((EmEnemyData*)(void*)data)->sub_0xA0->value_0x28;
        if ((f32)self->field_0x8A6 == lbl_80796DC4 * (f32)(limit != NULL ? *limit : 0x258)) {
            fn_80130858_view1(self, 0x258);
        }
    }
}

void fn_80140B20(void) {
    u32* src = lbl_805A1B34;
    u32* dst = (u32*)0x90348000;

    while (*src != 0) {
        *dst++ = *src++;
    }
    {
        u32 i;
        for (i = 0; i < 41; i++) {
            em_snd_proc_tbl[i] = 0;
        }
    }
}

u32 fn_80140C00(u8 id, u8 kind) {
    u8 sel = id;
    u32 sub;

    if ((s32)(u8)sel == 0x20) {
        switch ((u8)kind) {
        case 1:
            sub = 1;
            break;
        case 2:
            sub = 2;
            break;
        case 3:
            sub = 3;
            break;
        case 4:
            sub = 4;
            break;
        case 5:
            sub = 5;
            break;
        case 6:
            sub = 6;
            break;
        default:
            sub = 0;
            break;
        }
    } else {
        sub = 0;
    }
    return ((SystemWorkTables*)system_w)->tables_0x898[sel][sub];
}

/* ------------------------------------------------------------------------------------------------
 * the enemy file loader
 * ------------------------------------------------------------------------------------------------ */
void fn_80140CA0(char* name, u32 data, u32 a, u32 b, u32 c, u32* ctx) {
    EmcWork* work = (EmcWork*)&emc_work[*ctx * 0x18];
    u32 h1;
    u32 h2;
    u32 tmp;
    u32 res;
    EmFileRow row;
    u8* rec;

    res_file_ctor(&h1, 0);
    fn_8007B878(&h2, 0);
    get_move_work_adrs(0);
    work->handle_0x08 = fn_800D56F4(name, data);
    if (work->handle_0x08 == -1) {
        return;
    }
    rec = (u8*)fn_800D5418(work->handle_0x08);
    fn_80140DAC(&row, (EmFileRow*)((u8*)nw_res_manager + fn_800D4DD8(name) * 0x4C + 0x3070));
    if (rec == NULL) {
        return;
    }
    res_file_assign(&h1, res_file_ctor(&tmp, ((struct EmHandleRec*)(rec))->field_0x44));
    fn_801411B8(&h1);
    res = fn_800924CC(&h1, 0);
    fn_8007BA08(&h2, &res);
    fn_800E38BC(&h2);
    fn_800D5D60(work);
}

void fn_80140DAC(EmFileRow* dst, EmFileRow* src) {
    *dst = *src;
}

s32 fn_80140E48(char* name, u32 size, u8 flag) {
    EmcWork* work = (EmcWork*)&emc_work[flag * 0x18];
    char* file;
    u32 ctx;

    work->flags_0x03 |= 1;
    ctx = flag;
    file = fn_80146F10(work->index_0x02);
    if (file == NULL) {
        return 0;
    }
    load_file_req(name, (u32)file, (s32)size, (u32)fn_80140CA0, 1, &ctx);
    return (s32)file;
}

void fn_80140EE8(char* name, u32 size, u32 a, u32 b, u32 c, u32* ctx) {
    EmcWork* work = (EmcWork*)&emc_work[*ctx * 0x18];
    u32 h1;
    u32 tmp;

    res_file_ctor(&h1, 0);
    work->handle_0x10 = fn_800D56F4(name, size);
    if (work->handle_0x10 != -1) {
        u8* rec = (u8*)fn_800D5418(work->handle_0x10);
        if (rec != NULL) {
            res_file_assign(&h1, res_file_ctor(&tmp, ((struct EmHandleRec*)(rec))->field_0x44));
            fn_801411B8(&h1);
            fn_800E3358(2, work->field_0x00, &h1);
            fn_800D5E30(work);
        }
    }
    fn_801414C8(work);
}

s32 fn_80140FB0(char* name, u32 size, u8 flag) {
    EmcWork* work = (EmcWork*)&emc_work[flag * 0x18];
    char* file;
    u32 ctx;

    work->flags_0x03 |= 2;
    ctx = flag;
    file = fn_80146F10(work->index_0x02);
    if (file == NULL) {
        return 0;
    }
    load_file_req(name, (u32)file, (s32)size, (u32)fn_80140EE8, 1, &ctx);
    return (s32)file;
}

s32 em_kind_release(u8 id) {
    s32 loaded = 0;

    if (fn_801414D4(id) != 0xFF) {
        s8 v = lbl_805A1B08[id];
        if (v > 0) {
            fn_80146B98(v);
        }
        return 0;
    }
    {
        u8 sub = fn_80141470(id);
        if (sub == 0xFF) {
            return -1;
        }
        if (fn_80146DF4(id) == 0) {
            return -1;
        }
        if (lbl_80581E20[id].key_0x00 != 0) {
            fn_80140E48(lbl_80581E20[id].name_0x04, lbl_80581E20[id].key_0x00, sub);
            loaded = 1;
        }
        if (lbl_80582348[id].key_0x00 != 0) {
            fn_80140FB0(lbl_80582348[id].name_0x04, lbl_80582348[id].key_0x00, sub);
            loaded++;
        }
    }
    fn_80147160(id);
    if (fn_80147550(id) == 1) {
        s32 idx = fn_80147684(id, 0);
        if (idx >= 0) {
            fn_800FA9B8(&emc_work[0xEA0 + idx * 0x17], id);
        }
    }
    fn_800F0F9C(id);
    return loaded;
}
}
