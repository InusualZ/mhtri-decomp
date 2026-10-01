/*
 * hud/cockpit.cpp - phase 4 unit, `.text` 0x802D9EA4..0x802E0740 (132 functions, 26780 bytes).
 *
 * PHASE 4 (docs/splits/phase4, window d).  Recut of fn_802D44F4.cpp: its functions whose address lies in this range,
 * in address order; the rest of the range keeps its original bytes.  1 of 132 functions have a body here.
 *
 * FLAGS.  `cflags_main` (the ai lib's group, the old ai/fn_802D44F4.cpp's); the `.data`/`.bss` of the AI-NPC page
 * state it holds are defined here.
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .data, .sbss, .sdata, .sdata2, .text, extab,
 * extabindex).
 */
/* ---- header inherited from src/ai/fn_802D44F4.cpp (written against its pre-phase-4 range) ---- */
/*
 * ai/fn_802D44F4.cpp - the 0x802D44F4-0x802DDC04 band (165 functions, 38672 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py lookup + the Dolphin dump map at
 * D:/WiiExperiment/DumpSymbols.zip: every defined symbol in the range is a bare `fn_`/`zz_`
 * entry, and no `__FILE__` string covers it), so the file keeps the map's stem.
 *
 * Seam, module and language:
 *   - the range is where a discovery run was cut by `--max-bytes`, so its edge is a size cap and
 *     not a translation-unit boundary (brief section 1).  `tudiscover at 0x802D44F4` finds no
 *     closure edge, no must-link anchor and no labelled data the range references, so the
 *     boundary is unconstrained and the range is worked as one unit; the registered neighbour
 *     below is `ai/fn_802D0DCC.c` and the next proposal starts at 0x802DDC04.
 *   - the range defines `ai_skill_ck__FP8_AINPC_WUc`, `ai_torch_ck__FP8_AINPC_W`,
 *     `ai_taru_move_ck__FP8_AINPC_W`, `ai_taru_range_ck__FP8_AINPC_W` and `ai_demo_stop_ck__Fv`,
 *     i.e. the AI-NPC state checks of the `ai` module the neighbouring unit also builds, so the
 *     module is `ai`.  Its tail also defines the UI helpers `PutPageArrow__F...` and
 *     `get_rare_color__FUc`, which look like a second original file; that seam could not be
 *     proven from any evidence kind (docs/plan.md 8.3) and is recorded here as a hint instead.
 *   - the language is C++ (every defined name in the range is mangled).  The `fn_XXXXXXXX`
 *     definitions are `extern "C"` (the map name is a placeholder stem, not a mangling); the
 *     `ai_*`/`get_rare_color`/`PutPageArrow` ones are C++ free functions at global scope so the
 *     front-end reproduces the map's own mangling (rule 9).
 *
 * Types: the record the whole AI band drives is `_AINPC_W`, whose shared home is
 * `include/ai/ainpc.h` (main already created it as the union of the `ai` band's accessors); this
 * unit added the offsets its own bodies name to that file - +0x000, +0x172, +0x20C..+0x23E,
 * +0x33A, +0x374, +0x3B0..+0x3BC, +0x3D4..+0x3DC, +0x3F4..+0x414, +0x424, +0x431..+0x438,
 * +0x450, +0x46C, +0x47C, +0x483 - rather than carrying a second copy (rule 1).  The four tuning
 * tables it indexes are declared in `include/unsplit/ai.h` (no registered unit owns them, rule 2).
 *
 * Flags: `cflags_main`, plus `#pragma peephole off` for the whole file - retail keeps the unfused
 * `clrlwi`+`slwi` / `clrlwi`+`cmpwi` forms that the peephole pass folds into `clrlslwi` and a
 * masked compare (playbook 39); turning it back on costs `fn_802D773C` 73.3 -> 100 and
 * `fn_802D7754` 85.3 -> 75.5 (measured).
 *
 * Sections: .text 0x802D44F4..0x802DDC04, extab 0x8001496C..0x80014C64 (95 unwind-only records),
 * extabindex 0x80032C88..0x800330FC, .ctors 0x8056F388 (the static constructor in the range), and
 * .data 0x805D5428..0x805D5460 - `jumptable_805D5428`, the 14-entry switch table MWCC emits for
 * `fn_802D77A0` (playbook 53/56: claim exactly the table's own range, never the band around it).
 * The table was recovered by reading the 14 slot values out of `main.elf` (0 -> 0x802D77C4 etc.),
 * which is what turned the switch's arm mapping into source.
 *
 * Registered NonMatching; the bodies below are the part reconstructed so far, in address order:
 * 48 of 165 functions, 23 byte-identical and 40 at or above the 80 % bar (2.62 % of the unit's
 * bytes; the extab/`.data` sections are still unmatched, so the unit's own percent is 9.91).
 *
 * Residuals:
 *   - `fn_802D7B5C`/`fn_802D7C04`/`fn_802D7C6C` (61.7 / 0 / 19.9): the target's search loop stays a
 *     loop, MWCC unrolls ours - the `for (i = 0; i < N; i++) if (x < table[i+1]) break;` shape is
 *     right (conditions and body match) but the constant trip count is unrolled in our build.
 *   - `fn_802D7B24` (11.4): the two range tests want the raw `subi` result in a 32-bit `cmplwi`;
 *     written that way the compiler still inserts the sign-splitting sequence (`subfic`/`orc`).
 *   - `fn_802D6690` (59.0): the `field_0x33A * 4` index is computed before the table base in retail,
 *     after it in ours.
 *   - `fn_802D6B2C` (74.1) / `fn_802D77DC` (71.7) / `fn_802D7A50` (76.7) / `fn_802D7CE4` (75.7):
 *     register-allocation and scheduling differences only - the instruction stream agrees, the
 *     web order does not.
 *   - `fn_802D7688` (89.3), `fn_802D6A00` (89.0), `fn_802D7464` (99.98): same, instruction counts
 *     equal.
 *   - the remaining 117 functions have no body yet; `fn_802D44F4` alone is 8256 B (21 % of the
 *     range) and is a 2064-instruction compare tree on `ai_get_motion_no()`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/fn_800CDB2C.h"
#include "pl.h"
#include "ai/ainpc.h"
#include "ai/ainpc_w.h" /* `ainpc_w`, defined at the foot of this file (rule 2) */
#include "mh3_pad/lb_param_w.h" /* `lb_param_w`, owned by mh3_pad.cpp (rule 2) */
#include "unsplit/ai.h"
#include "ai/fn_802D44F4.h"    /* the owner's own header (rule 2) */
#include "quest/quest_item_slot.h" /* quest_move_state_valid_ck (rule 2: its owner) */

#ifdef __cplusplus

extern "C" {
#endif
void fn_802D9EB4(void);
}

extern "C" {

/* 0x802D9EA4 - forwards to the reaction dispatcher; the AI work pointer stays in the caller's r3. */
void ai_npc_reaction_forward(void)
{
    fn_802D9EB4();
}
}

