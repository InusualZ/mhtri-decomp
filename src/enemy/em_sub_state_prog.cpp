/*
 * enemy/em_sub_state_prog.cpp - phase 4 unit, `.text` 0x802F9994..0x802FA9A0 (11 functions, 4108 bytes).
 *
 * PHASE 4 (docs/splits/phase4, window d).  Recut of fn_802F5138.cpp: its functions whose address lies in this range,
 * in address order; the rest of the range keeps its original bytes.  5 of 11 functions have a body here.
 *
 * FLAGS.  `cflags_main`.  GUESS (rule 7): the stem names the sub-state dispatchers (`state_sub`, +0x1E6) the old
 * header documents.
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .data, .sdata, .sdata2, .text, extab, extabindex).
 */
/* ---- header inherited from src/enemy/fn_802F5138.cpp (written against its pre-phase-4 range) ---- */
/* enemy/fn_802F5138.cpp - an enemy action-program band, `.text` 0x802F5138..0x802FA9A0.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: every address resolves to a `zz_XXXXXXXX_` dump
 * placeholder and a bare `.text` entry in config/RMHE08/symbols.txt, and no `__FILE__` source-name
 * literal is reachable from the range's `.data`/`.rodata` references).
 *
 * WHAT IT IS.  The per-action state machines of one enemy's AI.  Every body takes the shared
 * `_ENEMY_WORK` record: the action dispatcher `fn_802F96B4` switches on its `action` (+0x1E5) and
 * the sub-state dispatchers (`fn_802F5D24`, `fn_802F92E8`, `fn_802F9678`, `fn_802F9BF0`,
 * `fn_802FA964`) on `state_sub` (+0x1E6); the bodies drive the enemy motion API
 * (`em_move_mode_set`, `em_mot_set`, `em_mot_set_ck`, `em_mot_end_ck`, `fn_80128030`), place the effects
 * (`res_eft_UV_model_create_name`, `em_se_tbl_play`/`em_se_tbl_play_alt` with the record tables
 * `lbl_805D6F70`..`lbl_805D7300`) and step the work record's own state bytes.  The band's `.data`
 * holds those record tables and the compiler-emitted switch tables (`jumptable_805D6E50`,
 * `jumptable_805D6E70`, `jumptable_805D89EC`, `jumptable_805D8A48`, `jumptable_805D8AA4`,
 * `jumptable_805D8C44`).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string: every `lis`/`addi`
 * pair in the range resolves to the `.sdata2` float pool, a switch table or one of the band's record
 * tables.  2. `dumpmap.py lookup` answers `zz_XXXXXXXX_` for every row but the unrelated 4-byte
 * `fn_802F9714`.  3. The code places the unit in `enemy`: of the range's 194 distinct callees the
 * largest block is the `enemy` module (`em_die_ck`, `em_work_die_ck`, `em_get_mot_no`,
 * `fn_8012EC74.cpp`'s motion band), and the `.data` `em0XX_prog_tbl` program tables bracket the
 * band.  The file therefore keeps the map's `fn_802F5138` stem; no name and no module was invented.
 *
 * LANGUAGE AND SECTIONS.  C++ - the range reaches genuinely mangled callees (`setVector3`,
 * `calcVecAng2`, `rotVecX`, `ran_suu`, `res_eft_UV_model_create_name`) through their real
 * signatures (rule 9); every plain `fn_` definition is `extern "C"` so it keeps the map's name
 * (playbook 42).  Sections: `.text` 0x802F5138..0x802FA9A0, `extab` 0x80015514..0x800156B4
 * (52 records), `extabindex` 0x80033E04..0x80034074 (52 x 12 B) and the `.ctors` word at
 * 0x8056F390 (the file-scope static initialiser `fn_802F9898`).  The `.sdata2` pool the range reads
 * is `extern` here and never defined (playbook 29).
 *
 * SEAM (unproven).  One maximal unclaimed run, registered whole; it sits between `ef/eft035.cpp`
 * (ends 0x802F5138) and the unclaimed run that starts at 0x802FA9A0, and both edges are weak (no
 * `__FILE__` name reaches either).  `tudiscover`'s model spans the range from `hud/cockpit_quest.cpp`'s
 * `__FILE__` anchor with an un-taken candidate seam inside, so the extent may still be more than one
 * original TU; it settles as the functions match.
 *
 * RESIDUALS (measured 2026-09-26; unit 16.14 %, 2248/22632 bytes, 26 of 72 functions at 100 %).
 *  - 41 functions have no body yet: the placement block 0x802F5D60..0x802F8290 (26 functions) and
 *    the tail block 0x802F8B9C..0x802FA8D8 (including `fn_802F9C2C` 0x710 and `fn_802FA33C` 0x4B0).
 *  - `fn_802F5B98` 76.9 % / `fn_802F5C58` 78.2 %: the target's `mode` test is an unsigned range fold
 *    whose A-block the compiler lays out *after* the whole compare chain; the if/else-if, the
 *    `>= 3 && <= 4` and the dense `switch` spellings were all measured and lose more.
 *  - `fn_802FA7EC` 0 %: 4-byte `b fn_80463EE0` thunk; the only available declaration of that callee
 *    (`ef/fn_80114E34.cpp`'s `f32 fn_80463EE0(s16, f32)`) forces an `extsh` the target does not have.
 *  - Our object emits one extra 8-byte `.sdata2` (MWCC's `(f32)(s32)` conversion double).  The
 *    address the target loads is the shared `lbl_8079AA90`, referenced DOL-wide, so it is a shared
 *    pool entry: claim nothing (playbook 58).  `datagap.py --unit enemy/fn_802F5138` is the measure.
 *  - Levers this band needed, all measured: `switch` on a byte where retail keeps a signed `cmpwi`
 *    chain; `(f32)(s32)` where retail keeps the `xoris` conversion; a scoped `#pragma peephole off`
 *    per function (`fn_802F8B28`, `fn_802F9774`, `fn_802F96B4`, `fn_802F51DC`) - file-wide off costs
 *    `fn_802F5B98` 62.5 -> 20.8 %.
 *  - `include/enemy/fn_801251D0.h` gained the four-argument C++ view of 0x801251D0 and
 *    `enemy/em009_act.cpp`'s 28 call sites moved to it (both targets set r6 = the id):
 *    `fn_8038BD28` 96.729 -> 100.0 %.  The outbox asks the orchestrator to confirm that shared-file
 *    edit on the batch.
 * Per-function measurements: `.pi/notes/802f5138-fn-802f5138-046c.md`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "ef.h"
#include "ef/effect.h"
#include "ef/eft_res.h"
#include "enemy/enemy_control.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "lobby/fn_802FA9A0.h"
#include "stage/stg_w.h"
extern "C" void fn_802F9BF0(_ENEMY_WORK* work);
extern "C" void fn_802F9C2C(_ENEMY_WORK* work);
extern "C" void fn_802FA33C(_ENEMY_WORK* work);
extern "C" f32 fn_802FA7EC(s16 a, f32 b);
extern "C" void fn_802FA7F0(_ENEMY_WORK* work);
extern "C" void fn_802FA800(_ENEMY_WORK* work);
extern "C" void fn_802FA964(_ENEMY_WORK* work);

#pragma peephole off

#pragma peephole on

/* Sub-state dispatcher of the band's model-placement action. */
extern "C" void fn_802F9BF0(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        fn_802F9C2C(work);
        break;
    case 1:
        fn_802FA33C(work);
        break;
    case 2:
        fn_802FA7F0(work);
        break;
    case 3:
        fn_802FA800(work);
        break;
    }
}

/* Forwards to the shared release helper (a tail call: the parameters pass straight through). */
extern "C" f32 fn_802FA7EC(s16 a, f32 b) {
    return fn_80463EE0(a, b);
}

/* Advances a byte counter in a record above the work record. */
extern "C" void fn_802FA7F0(_ENEMY_WORK* work) {
    work->state = work->state + 1;
}

/* Releases the second shared resource. */
extern "C" void fn_802FA800(_ENEMY_WORK* work) {
    eft_res_slot_release(work);
}

/* Sub-state dispatcher of the band's last action. */
extern "C" void fn_802FA964(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        fn_802FA9A0(work);
        break;
    case 1:
        fn_802FAB98(work);
        break;
    case 2:
        fn_802FAFB4(work);
        break;
    case 3:
        fn_802FAFC4(work);
        break;
    }
}

