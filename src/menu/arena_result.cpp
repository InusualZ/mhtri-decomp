/*
 * menu/arena_result.cpp - the multiplayer/arena quest-result band: the run's point score, its
 * clear-time and rank text, the per-player item counts and the lobby sync that goes with them.
 *
 * `.text` 0x803B0F98..0x803B465C (56 functions, 0x36C4 B), registered whole as the proposal
 * `803B0F98` asked.  The seam is UNPROVEN, and the range is one maximal unclaimed run rather than a
 * proven translation unit:
 *   - the left boundary is the proposal cap and the right one is confirmed: the three private jump
 *     tables `jumptable_805F7B78`/`BB0`/`BE8` (`.data` 0x805F7B78..0x805F7C68) are each cited by
 *     exactly one function of this range, in code order, and `jumptable_805F7C68` belongs to
 *     `fn_803B465C` - the next band (`tudiscover at 0x803B0F98` calls the `.data` run jump at both
 *     edges "strong" evidence).
 *   - no `__FILE__` string covers the range: the DOL's only single-copy menu source name is
 *     `menu_note.cpp` (0x805E91F8), cited from `menu/menu_note.cpp`'s own range, and every other
 *     menu source name (`menu_item.cpp`, `menu_placeinfo.cpp`, `menu_friendlist.cpp`,
 *     `menu_plsearch.cpp`, `arenatask.cpp`, `movie.cpp`) is cited from a band at 0x8043xxxx/0x8031xxxx.
 *   - the runtime dump answers `zz_` for every address of the range, so the NAME is evidence class 4:
 *     module `menu` from the neighbours (`menu/multi_result.cpp` below, the registered `menu/*` band
 *     above) and from the callees (`get_str_tbl`, `msg_str_gen`, `GetItemData`, `my_player_no`,
 *     `get_qResult_work`), the file name a GUESS from what the band does.  Every function name and
 *     every field name here is a GUESS derived from its body.
 *
 * The band is C++ (its objects carry mangled undefined symbols only), and every function it defines is
 * spelled `extern "C"` so the emitted name is the map's (playbook row 42) - the target objects' own
 * symbols are unmangled stems.
 *
 * RESIDUAL: 43 of the 56 functions are written - 6528 B of the range's 14020 B, 27 of them at
 * 100 %, 41 at 80 % or better (unit fuzzy 44.41 %).  The two rows below the 80 % bar are
 * `quest_work_word_get` (79.23 %): the target's loop is `mtctr`/`bdnz` over the list count while
 * ours re-reads the count, and the register pair (`items`/`count` in r3/r4 retail, r4/r5 ours) is
 * the allocator's (playbook 22); and `quest_arena_data_step` (75.40 %, 352 B ours vs 364 target):
 * retail keeps `&quest_work + player*12` as the address base and folds `player_items_0x6A6A` into
 * every displacement, where ours folds that addend into the row pointer instead; direct
 * `[player][i]` indexing measures 73.11 % / 424 B, worse.  `quest_item_count_sum` (85.70 %),
 * `quest_element_value_set` (86.47 %), `quest_players_state_get` (85.47 %), `quest_slot_items_get`
 * (84.48 %), `quest_element_item_count_get` (83.81 %) and `quest_slot_count_get` (82.50 %) keep
 * their measured best shape; the first four are 4-16 B long and all six are the allocator's.
 *  - `quest_arena_time_text_get` (97.15 %, 316 B both) is the only written row with a
 *    *scheduling* residual: the target hoists `rec->field_0x02C - 9000` above the
 *    `system_w.field_0x8af` load (retail `subi r4,r3,-9000` at the branch head, ours after the
 *    load), and its `seconds` numerator stays in r6 (`subf r6,r0,r30`) where ours stages it through
 *    r3.  Both are the same instruction multiset in the same size; the expression forms tried are
 *    recorded in the outbox.
 *  - `quest_clear_time_text_get` (98.25 %) / `quest_elapsed_time_text_get` (98.33 %) keep the
 *    `sprintf(..., minutes, seconds, Screen_w.frame_scale)` form their call sites' float argument
 *    makes; `quest_arena_time_text_get`'s own call site passes two integers only (retail `crclr`
 *    cr1eq, no r7), which is why its text omits the third argument.
 *  - `quest_item_slots_prune` (108 B, the row this pass added) is byte-identical; its shape needed
 *    a named pointer local declared before the index (`s32 i; QuestItemSlot* slot = slots;` -
 *    with `i` or `slots[i]` alone it is 91.41 %, the allocator's callee-saved pair mirrors,
 *    playbook 63).  Its only callee was `fn_802752C8`, renamed `Pl_item_id_usable_ck`: retail
 *    passes a second argument (`li r4,0` here, 1 in `em_pop`, 2 in `ai`) that the callee's body
 *    never reads, so the **declaration** carries it (`include/Pl/fn_80273B14.h`); the Pl unit was
 *    re-measured after the change and every row of it is unchanged (`Pl_item_id_usable_ck` keeps
 *    its own 96.47 % residual - three `cmpwi`/`cmplwi` choices).
 *  - `quest_result_field_text_get` (684 B, this pass's second row) is byte-identical too, and it
 *    is the band's biggest single win: 44.41 % from 39.53 %.  Two shapes had to be read off the
 *    target rather than guessed: the NULL-record guard returns straight to the epilogue while the
 *    `switch` has NO explicit bounds check - its `default` is the `return quest_text_buffer` after
 *    the switch, which is where MWCC puts the `cmplwi kind,31` + `bgt` - and MWCC emits the case
 *    BODIES in source order, so the arms are written in the jump table's own order (0, 22, 1, 16,
 *    17, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 12, 30, 31, 26, 27, 28).  Its five localized
 *    strings and their pointer table are DEFINED here (the target's pool labels are map globals the
 *    unit does not claim); the three accented characters are `\x` byte escapes so the file stays
 *    ASCII and `sjiswrap` cannot re-encode them.  `QuestRecord`'s text runs (+0x000, +0x02E,
 *    +0x08C, +0x0B5, +0x0DE, +0x13C, +0x19A, +0x1C9) are named in `include/unsplit/menu.h` now,
 *    which is also where the field-kind numbering comes from.
 *  - the `QuestElement` size correction to the 0x60 its own target objects use (`addi r5,r5,96` in
 *    `quest_element_value_get`) moved **eight** rows up and none down: `quest_field2E8_text_get`
 *    99.96 -> 100 %, `quest_element_item_apply` 97.68 -> 97.86, `quest_element_value_set`
 *    86.43 -> 86.47, `quest_item_count_sum` 85.64 -> 85.70, `quest_players_state_get`
 *    85.38 -> 85.47, `quest_slot_items_get` 84.38 -> 84.48, `quest_element_item_count_get`
 *    83.79 -> 83.81, `quest_slot_count_get` 82.42 -> 82.50 - the header's union had pushed the
 *    struct to 0x64, shifting every field past +0x94 by 0xC.
 *  - ORDERING, unrecorded until now: the 43 written bodies are in DEFINITION order, not the target's
 *    address order - our `.text` opens with `quest_record_get` (target offset 0x23FC) where the
 *    target opens with `fn_803B0F98` (0).  `flipcheck.py` cannot see it here (it names a permutation
 *    only when the sizes agree; ours is 0x1980 B against the target's 0x36C4), so the source order
 *    has to become the address order before a flip can be attempted (playbook 52's permutation
 *    class, as `src/Pl/pl_act.cpp` and `src/g3d/g3d_resnode.cpp` record it).  Not reordered here:
 *    with 13 rows unpaired nothing measures the move, so a reorder would have to prove itself
 *    score-neutral first.
 *
 * The 13 functions still unwritten are blocked by *naming*, not by evidence: every one of them but
 * `quest_arena_summary_step` calls another band's starter name (`pl_item_add`,
 * `Pl_motion_input_ck`, `countOccupiedServerSlots`/`isServerSelectState`/`isReadyCountOne`,
 * `fn_802D8ABC`..`fn_802D8EA8`, `hud_msg_push`, `fn_8033A920`, `fn_802AFC94`/`AFE08`, ..., and the
 * `fn_800CF280` this list used to carry is the already-named `move_work_state_ck`), and a call to
 * one in this new file is a rule-7 finding; `quest_arena_summary_step` (344 B) is blocked
 * by *ownership* instead - its only foreign callee `quest_element_build` (0x803AD008) is already
 * named, but it is `quest/quest_entry.cpp`'s row, so its declaration belongs in that owner's
 * header.  Naming them means sweeping every reference site in the units that own them
 * (`stage_map_kind_get` alone is cited from 30 files of `enemy`, `Pl`, `sound` and `stage`), so the sweep
 * belongs to the campaign's naming wall - registered in `.pi/notes/naming-backlog.md` (`| 14 | 30 |
 * stage_map_kind_get |`) and carried in this batch's outbox `blockers` (never in `config_requests`, which
 * is all `tools/units/backlog.py` reads) - and not to this lane's diff.  The blocked rows are
 * listed with their sizes in the outbox; the two biggest are `fn_803B0F98` (2020 B) and
 * `fn_803B177C` (1596 B).
 *
 * `quest_element_set` (0x803A9DEC) is declared where rule 2 wants it - in its owner's header
 * `include/lobby/lb_quest_screen.h`, whose unit's registered `.text` 0x803A3A50-0x803AA4A4 covers
 * the address.  The owner's own source does not cite it yet (its row is unwritten); this file's two
 * call sites reach it through that header.
 *
 * Data (measured, `datagap.py --unit menu/arena_result`).  Claimed and byte-identical: `.sdata`
 * 0x8079367C..0x80793684 (8 B) - the " " and "..." string-pool entries
 * `quest_result_field_text_get` is the sole referencer of.  UNCLAIMED, and the unit's open data
 * debt: `.data` 288 B, which is TWO runs in retail, 27 KB apart - the three switch jump tables
 * 0x805F7B78..0x805F7C68 (0xF0 B, cited by `fn_803B0F98`, `fn_803B177C` and this file's
 * `quest_result_field_text_get` respectively) and the five localized "(Quest Name Unavailable)"
 * strings plus their 6-pointer table 0x8060E800..0x8060E8A0 (0xA0 B).  Measured: claiming BOTH
 * `.data` runs dies in `dtk dol split` with `Cyclic dependency encountered while resolving link
 * order: menu/arena_result.cpp -> ... -> quest/arenatask.cpp -> auto_07_806073F0_data`, and the
 * 0x8060E800 run *alone* dies the same way (it sits strictly inside `auto_07_806073F0_data`, so the
 * auto unit's remainder would have to be both before and after this unit).  The 0x805F7B78 run
 * *alone* splits fine but is the wrong claim for this object: our `.data` is one 288 B section, so
 * the linker would place it at 0x805F7B78 and the strings would land in the jump tables' range.
 * Both runs are therefore left to their auto units and our object carries them as `ours-extra
 * .data 288 B` (+ `.rela.data`): a claim-shape gap, not a source defect - the five strings and
 * their padding are byte-identical to the DOL's 0x8060E800..0x8060E888 (checked against the auto
 * data object).  It needs dtk to express several runs of one section, or a data-only unit for the
 * strings run, before this unit can flip; filed as a tooling item.  `quest_name_unavailable_text`
 * is therefore defined TWICE in the link inputs: here (`src/menu/arena_result.o`, `.data` +0x88,
 * 0x18 B, global) and in `obj/auto_07_806073F0_data.o` (`.data` +0x7498, 0x18 B, global) - a
 * duplicate strong symbol that is invisible only because a `NonMatching` object is never linked,
 * and fatal the moment this unit can flip.  The faithful shape is what the target object carries -
 * a DECLARATION (its own `quest_name_unavailable_text` is UND) - and it is measured
 * codegen-neutral: the extern variant's `.text` is byte-identical to this one and its `.text`
 * relocation set identical bar the local pool-label numbering, with `ours-extra .data` shrinking
 * 288 B -> 128 B and `.rela.data` 456 B -> 384 B.  It is nonetheless not landable today:
 * `stylelint.py --diff` counts the declaration as an ADDED finding whichever way it is spelled - in
 * this file as rule 2 + rule 12 (there is no owner's header for it, and no registered range covers
 * 0x8060E888) and in the band header `include/unsplit/menu.h` as two rule-12 findings - because the
 * claim rule 12 asks for is exactly the cycle above.  So the definition stays and the clash is
 * handed to the same tooling item: it disappears with the claim (multi-run support or a data-only
 * unit for the strings run).  The pool constants the band
 * addresses (`frames_per_second_60f`, `percent_scale_100f`, `quest_grade_ratio_*`) are declared
 * `extern` in `include/unsplit/menu.h` and never defined here, so no `.sdata2` is emitted for them.
 * The other data row is `ours-extra .sdata2 16 B`: MWCC's implicit int->float magic
 * (`0x4330000080000000` unsigned / `0x4330000000000000` signed), which the compiler pools per TU and
 * `quest_grade_get`/`quest_grade_rank_get`/`quest_grade_text_cur_get` are the band's first users of.
 * The target's copies are `lbl_8079C528`/`lbl_8079C550`, inside the band's unclaimed `.sdata2` run
 * (0x8079C500.., which also holds `quest_grade_ratio_*`/`percent_scale_100f`), so dtk put them in an
 * `auto_*_sdata2` unit and the target object carries none.  They are shared pool entries - neither
 * claimable nor nameable from source (playbook 58) - so the row is a claim gap, not a source defect.
 *
 * Cross-unit declarations this pass needed: the `quest_flag_*_ck`/`quest_arena_*_get`/
 * `quest_element_value_get`/`quest_element_value_at`/`quest_element_remaining_get`/
 * `quest_all_player_item_count_sum` accessors at 0x803B4BEC..0x803B68F0 are the quest band's own but
 * sit inside `enemy/em_pop.cpp`'s registered range (a `--max-bytes` cap over several bands), so their
 * declarations went into that owner's header (`include/enemy/em_pop.h`), which already carries the
 * same shape for its own unwritten rows.  `str_tbl_33_get` (0x802DFACC) is unowned and stays in its
 * band header `include/unsplit/lobby.h`.  `system_w.field_0x8af` (0x8AF) was split out of
 * `SystemWork`'s `pad_0x898`, and `QuestRecord` gained `field_0x36C` while its +0x30C word became
 * the byte pair `quest_monster_text_get` reads (it was an unused `s32`).
 *
 * Rule-7 renames this pass: `fn_803B421C` -> `quest_name_index_get`, `fn_803B4308` ->
 * `quest_monster_text_get` (both this unit's own), and the six map-only rows the band's callees
 * needed - `fn_803B5FF0` -> `quest_element_value_at`, `fn_803B5F84` -> `quest_element_value_index_get`,
 * `fn_803B6150` -> `quest_element_remaining_get` (also swept in `src/Pl/pl_act.cpp`, its only
 * referrer), `fn_803B61DC` -> `quest_all_player_item_count_sum`, `fn_803B4E50` ->
 * `quest_flag_10000000_ck`, and `fn_802DFACC` -> `str_tbl_33_get` (swept in
 * `include/unsplit/lobby.h` and `src/lobby/fn_801E7530.cpp`).
 *
 * FLAGS: `menu`'s `cflags_menu` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, Wii/1.3).
 */

#include "types.h"
#include "unsplit/menu.h"                 /* the band's own records, tables and callees */
#include "quest/quest_list_values.h"     /* `quest_list_values` - owned by quest/quest_entry.cpp (rule 2) */
#include "enemy/em_pop.h"                 /* the quest accessors inside em_pop's registered range */
#include "unsplit/lobby.h"                /* `str_tbl_33_get`, the band's monster-name string table */
#include "unsplit/unknown.h"              /* `system_w` (+0x09: the message-language index) */
#include "unsplit/Runtime.PPCEABI.H.h"    /* sprintf / strcpy */
#include "unsplit/ef.h"                   /* get_move_work_adrs */
#include "ef/fn_800CDB2C.h"               /* my_player_no */
#include "g3d/g3d_anmchr.h"               /* msg_str_gen / flfntStrLen / flKnjMsgNumPtr (rule 2) */
#include "Pl/fn_80273B14.h"               /* `Pl_item_id_usable_ck`, the id-usable predicate (rule 2) */

extern "C" {

/* The result screen's localized "(Quest Name Unavailable)" rows, indexed by the message-language
 * index `system_w.field_0x09`; the first two entries are the same English row, so `-str reuse`
 * pools one literal for both.  These five strings and the table are this band's `.data`
 * (0x8060E800..0x8060E8A0), UNCLAIMED and left to its auto unit - which is also the second
 * definition of this name, the clash the file header's data paragraph records - emitted here
 * because `quest_result_field_text_get` is their only referencer.  The three accented characters
 * are written as byte escapes: the source has to stay pure ASCII or `sjiswrap` re-encodes it. */
const char* quest_name_unavailable_text[6] = {
    "(Quest Name Unavailable)",
    "(Quest Name Unavailable)",
    "(Nom de qu\xC3\xAAte indispo.)",
    "(Questname nicht verf\xC3\xBCgb.)",
    "(Nome missione non disp.)",
    "(Misi\xC3\xB3n no disponible)",
};

/* Prototypes for the definitions below that an earlier function calls (`quest_item_count_sum` is
 * defined at the address its own body lives at, after its two callers). */
s16 quest_item_count_sum(u16 id, s32 who);
QuestRecord* quest_record_get(void);

/* The current result row, or NULL when the screen has none. */
QuestRecord* quest_record_get(void) {
    QuestRecord* rec = quest_work.record_0x03C;

    if (rec == NULL) {
        return NULL;
    }
    return rec;
}

/* String table 35's `index`-th entry (the item-summary formats). */
char* quest_str_tbl_35_get(u32 index) {
    return (char*)get_str_tbl(35)[index];
}

/* String table 4's `index`-th entry. */
char* quest_str_tbl_4_get(u32 index) {
    return (char*)get_str_tbl(4)[index];
}

/* The pair table's `[row][col]` byte at +0x805F7898. */
u8 quest_pair_table_get(u8 row, u8 col) {
    return *(col + (&quest_pair_table[0] + (row * 2)));
}

/* The byte table's `index`-th entry at +0x805F78B4. */
u8 quest_byte_table_get(u8 index) {
    return *(&quest_byte_table[0] + index);
}

/* The record's name out of string table 5 (an empty string when there is no record). */
char* quest_name_text_get(void) {
    QuestRecord* rec = quest_record_get();
    u8** tbl = get_str_tbl(5);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        strcpy(quest_text_buffer, (char*)tbl[rec->field_0x08B]);
    }
    return quest_text_buffer;
}

/* The same name for an explicit record. */
char* quest_name_text_get_of(QuestRecord* rec) {
    u8** tbl = get_str_tbl(5);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        strcpy(quest_text_buffer, (char*)tbl[rec->field_0x08B]);
    }
    return quest_text_buffer;
}

/* The record's +0x198 entry out of string table 41. */
char* quest_field198_text_get(void) {
    QuestRecord* rec = quest_record_get();
    u8** tbl = get_str_tbl(41);

    quest_text_buffer[0] = 0;
    if (rec == NULL) {
        return quest_text_buffer;
    }
    strcpy(quest_text_buffer, (char*)tbl[rec->field_0x198]);
    return quest_text_buffer;
}

/* The same +0x198 entry for an explicit record. */
char* quest_field198_text_get_of(QuestRecord* rec) {
    u8** tbl = get_str_tbl(41);

    quest_text_buffer[0] = 0;
    strcpy(quest_text_buffer, (char*)tbl[rec->field_0x198]);
    return quest_text_buffer;
}

/* The record's +0x13A value formatted with string table 6's first format. */
char* quest_field13A_text_get(void) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[0], rec->field_0x13A);
    }
    return quest_text_buffer;
}

/* The same +0x13A value for an explicit record. */
char* quest_field13A_text_get_of(QuestRecord* rec) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[0], rec->field_0x13A);
    }
    return quest_text_buffer;
}

/* The record's +0x34C / +0x350 / +0x354 timers as text: `which` picks which of the three. */
char* quest_time_text_get(u8 which) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        switch (which) {
        case 0:
            sprintf(quest_text_buffer, tbl[1], rec->field_0x34C);
            break;
        case 1:
            if (rec->field_0x350 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1]);
            }
            break;
        case 2:
            if (rec->field_0x354 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1]);
            }
            break;
        }
    }
    return quest_text_buffer;
}

/* The same three timers for an explicit record. */
char* quest_time_text_get_of(QuestRecord* rec, u8 which) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        switch (which) {
        case 0:
            sprintf(quest_text_buffer, tbl[1], rec->field_0x34C);
            break;
        case 1:
            if (rec->field_0x350 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1]);
            }
            break;
        case 2:
            if (rec->field_0x354 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1]);
            }
            break;
        }
    }
    return quest_text_buffer;
}

/* The record's +0x348 value as text. */
char* quest_field348_text_get(void) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[1], rec->field_0x348);
    }
    return quest_text_buffer;
}

/* The same +0x348 value for an explicit record. */
char* quest_field348_text_get_of(QuestRecord* rec) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[1], rec->field_0x348);
    }
    return quest_text_buffer;
}

/* The work block's +0x2E8 value (clamped up to 0) as text. */
char* quest_field2E8_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 value;

    quest_text_buffer[0] = 0;
    value = quest_work.field_0x2E8;
    if (value < 0) {
        value = 0;
    }
    sprintf(quest_text_buffer, tbl[1], value);
    return quest_text_buffer;
}

/* The run's clear time (`quest_work` +0x24 frames) as `mm'ss"ff` text. */
char* quest_clear_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = quest_work.field_0x024;
    f32 frames = frames_per_second_60f * Screen_w.frame_scale;
    s32 minutes = time / (s32)frames;
    s32 seconds = (time - minutes * (s32)frames) / (s32)Screen_w.frame_scale;

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds, Screen_w.frame_scale);
    return quest_text_buffer;
}

/* The elapsed time (`quest_work` +0x1C minus +0x20 frames) as the same text. */
char* quest_elapsed_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = quest_work.field_0x01C - quest_work.field_0x020;
    f32 frames = frames_per_second_60f * Screen_w.frame_scale;
    s32 minutes = time / (s32)frames;
    s32 seconds = (time - minutes * (s32)frames) / (s32)Screen_w.frame_scale;

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds, Screen_w.frame_scale);
    return quest_text_buffer;
}

/* The elapsed time in frames. */
s32 quest_elapsed_time_get(void) {
    return quest_work.field_0x01C - quest_work.field_0x020;
}

/* The run's score pair (+0x90/+0x91) as text. */
char* quest_score_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[3], quest_work.field_0x091, quest_work.field_0x090);
    return quest_text_buffer;
}

/* The arena item tally as text: the "N of M" string table 35's format 26 carries when the player
 * still holds items and the run is short of them, else the "none" format 27.  `index` 0 sums every
 * arena element, otherwise it reads element `index` alone; the `quest_flag_800000_ck` case reports
 * the whole set at once. */
char* quest_arena_items_text_get(u8 index) {
    s32 have;
    s32 missing;
    s32 count;
    s32 i;
    s32 need;
    s32 value;

    quest_text_buffer[0] = 0;
    have = 0;
    if (index != 0 && quest_flag_800000_ck(NULL) == 1) {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(27));
        return quest_text_buffer;
    }
    if (quest_flag_80000_ck(NULL) == 1 || quest_flag_2000000_ck(NULL) == 1 ||
        quest_flag_80000000_ck(NULL) == 1 || quest_flag_4000000_ck(NULL) == 1) {
        count = (quest_flag_100_ck(NULL) == 1) ? 2 : 3;
        need = 0;
        for (i = 0; i < count; i++) {
            value = quest_arena_count_get(i);
            if (value >= 0) {
                have += value;
            }
            value = quest_arena_need_get(i);
            if (value >= 0) {
                need += value;
            }
        }
        missing = have - need;
    } else {
        have = quest_arena_count_get(index);
        missing = have - quest_arena_need_get(index);
    }
    if (missing < 0 || have <= 0) {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(27));
    } else {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(26), missing, have);
    }
    return quest_text_buffer;
}

/* The arena run's time as the same `mm'ss"ff` text `quest_clear_time_text_get` builds, from the
 * record's clear time instead of the work block's.  The clear time is first mapped through the
 * arena's own tables: the fixed `multi_arena_clr_time` pair when the 0x100000 flag is set and the
 * time is below 10000, else `arena_time_table`'s per-rank table scaled by 30 when 0x10 is set. */
char* quest_arena_time_text_get(QuestRecord* rec, s32 which) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = which;
    f32 frames;
    s32 minutes;
    s32 seconds;

    if (quest_flag_100000_ck(rec) == 1 && rec->field_0x02C < 10000) {
        time = multi_arena_clr_time[(rec->field_0x02C - 9000) * 2 + (system_w.field_0x8af != 0)];
    } else if (quest_flag_10_ck(rec) == 1) {
        time = arena_time_table[rec->field_0x02C - 60000][(u8)which] * 30;
    }
    frames = frames_per_second_60f * Screen_w.frame_scale;
    minutes = time / (s32)frames;
    seconds = (time - minutes * (s32)frames) / (s32)Screen_w.frame_scale;
    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds);
    return quest_text_buffer;
}

/* The grade (0 = no record) as text: the grade's own format for 1..4, else the language's
 * "no record" string. */
char* quest_grade_text_get(u8 grade) {
    char** tbl = (char**)get_str_tbl(6);
    u32 slot;

    switch (grade) {
    case 0:
        quest_text_buffer[0] = 0;
        sprintf(quest_text_buffer, quest_grade_none_text_table[system_w.field_0x09]);
        return quest_text_buffer;
    case 1:
    case 2:
    case 3:
    case 4:
        slot = grade + 5;
        break;
    }
    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[slot]);
    return quest_text_buffer;
}

/* The current run's grade as text: the same ratio `quest_grade_get` scores, rendered by
 * `quest_grade_text_get` (1 when no element matches, so the "no record" string comes out). */
char* quest_grade_text_cur_get(void) {
    s16 total;
    f32 ratio;
    u8 grade = 1;

    quest_text_buffer[0] = 0;
    if (quest_element_value_get(4, &total) == 0) {
        return quest_text_buffer;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    if (ratio <= quest_grade_ratio_10f) {
        grade = 4;
    } else if (ratio <= quest_grade_ratio_30f) {
        grade = 3;
    } else if (ratio <= quest_grade_ratio_50f) {
        grade = 2;
    }
    return quest_grade_text_get(grade);
}

/* The run's grade: the score as a percentage of the arena element 4 target, banded at the three
 * `quest_grade_ratio_*f` thresholds (0 when the element is not live). */
s32 quest_grade_get(void) {
    s16 total;
    f32 ratio;

    if (quest_element_value_get(4, &total) == 0) {
        return 0;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    if (ratio <= quest_grade_ratio_10f) {
        return 4;
    }
    if (ratio <= quest_grade_ratio_30f) {
        return 3;
    }
    if (ratio <= quest_grade_ratio_50f) {
        return 2;
    }
    return 1;
}

/* Whether the same percentage reaches `grade` - the rank the caller's threshold asks for. */
s32 quest_grade_rank_get(u8 grade) {
    s16 total;
    f32 ratio;

    if (quest_element_value_get(4, &total) == 0) {
        return 0;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    return ratio <= (f32)grade;
}

/* The record's +0x36C slot progress byte, or 0 while the work state is "moving" and for a missing
 * record. */
u8 quest_slot_progress_get(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x36C;
}

/* The record's name index (string table 5's key), or 0 for a missing record; 1 while the work
 * state is "moving". */
u8 quest_name_index_get(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 1;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x08B;
}

/* The record's +0x372 word, or the current record's. */
u16 quest_field372_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x372;
}

/* The "no time yet" text (string table 6's fifth format). */
char* quest_no_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[5]);
    return quest_text_buffer;
}

/* The record's +0x780 `which`-th monster id rendered through string table 33, the "no time yet"
 * text when the id is 0 or the record is missing, and a lone space for the second slot while both
 * ids are still unset. */
char* quest_monster_text_get(QuestRecord* rec, u8 which) {
    if (rec == NULL) {
        return quest_no_time_text_get();
    }
    if (which == 1 && rec->field_0x30C[0] == 0 && rec->field_0x30C[1] == 0) {
        quest_text_buffer[0] = ' ';
        quest_text_buffer[1] = 0;
        return quest_text_buffer;
    }
    if (rec->field_0x30C[which] == 0) {
        return quest_no_time_text_get();
    }
    strcpy(quest_text_buffer, (char*)str_tbl_33_get(rec->field_0x30C[which]));
    return quest_text_buffer;
}

/* Applies `used` of the arena item `id` to the work block's item table (the entry the table's own
 * count has room for). */
void quest_element_item_apply(u8 id, u16 count, u16 used) {
    QuestWork* work = quest_work_ptr;
    QuestArenaItem* item;
    s8 index;
    s32 rest;

    if (work == NULL) {
        return;
    }
    index = work->count_0x6A2A;
    if (index >= 2) {
        return;
    }
    item = &work->arena_items_0x6A2C[index];
    if (item->id != id) {
        return;
    }
    if (item->count != count) {
        return;
    }
    if (item->remaining == 0) {
        return;
    }
    rest = item->remaining - used;
    if (rest < 0) {
        rest = 0;
    }
    item->remaining = rest;
}

/* The number of u16 entries the work block's item stack `index` holds (0 when out of range). */
s32 quest_slot_count_get(u8 index) {
    QuestWork* work = quest_work_ptr;

    if (work == NULL) {
        return 0;
    }
    if (index >= 8) {
        return 0;
    }
    return work->slot_counts_0x66B8[index];
}

/* Copies the work block's item stack `index` into `out`. */
void quest_slot_items_get(u8 index, u16* out) {
    QuestWork* work = quest_work_ptr;
    u16* src;
    s32 i;

    if (work == NULL) {
        return;
    }
    if (index >= 8) {
        return;
    }
    src = work->slot_values_0x6698[index];
    for (i = 0; i < work->slot_counts_0x66B8[index]; i++) {
        out[i] = src[i];
    }
}

/* Clears both halves of every entry of the run's 35-slot item list whose id is not a usable one. */
void quest_item_slots_prune(QuestItemSlot* slots) {
    s32 i;
    QuestItemSlot* slot = slots;

    for (i = 0; i < 35; i++, slot++) {
        if (Pl_item_id_usable_ck(slot->id, 0) == 0) {
            slot->id = 0;
            slot->count = 0;
        }
    }
}

/* The u16 the `quest_list_items` entry whose +0x2C key is `key` pairs with (0 when there is none). */
u16 quest_work_word_get(u16 key) {
    QuestListItem** items = quest_list_items;
    s32 count = quest_list_count;
    s32 i;

    for (i = 0; i < count; i++) {
        if (items[i]->field_0x02C == key) {
            break;
        }
    }
    if (i >= count) {
        return 0;
    }
    return quest_list_values[i];
}

/* Whether exactly one of the three key bytes of entry 6 is set and all of them are finished (3). */
s32 quest_players_state_get(void) {
    QuestWork* work = quest_work_ptr;
    s32 done = 0;
    s32 active = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        u8 state = work->player_state_0x2D4[i];

        if (state == 3) {
            done = 1;
        }
        if (state != 0) {
            active++;
        }
    }
    if (done == 1 && active <= 1) {
        return 1;
    }
    return 0;
}

/* Clears element `index` of the work block's element array. */
void quest_element_reset(u8 index) {
    if (quest_work_ptr != NULL) {
        quest_element_set(quest_work_ptr, index, NULL);
    }
}

/* Sets element `index`'s value to 0 once `id`'s own count has caught up with its target. */
void quest_element_value_set(s32 index, u16 id) {
    QuestWork* work = quest_work_ptr;
    s32 i;

    if (work == NULL) {
        return;
    }
    for (i = index; i < 3; i++) {
        QuestElement* el = &work->elements_0x0094[i];

        if ((el->flags & 0x8) != 0 && (el->flags & 0x2) != 0) {
            if (el->id == id) {
                if (quest_item_count_sum(id, 0) >= el->value) {
                    quest_element_set(work, (u16)i, NULL);
                }
            }
        }
    }
}

/* How many of the item `id`'s count `element`'s value still asks for, returning the element's id
 * (`out` gets the remainder, or 0 when the element is not live). */
u16 quest_element_item_count_get(s32 index, u16* out) {
    QuestElement* el;
    s16 rest;

    if (quest_work_ptr == NULL) {
        return 0;
    }
    el = &quest_work_ptr->elements_0x0094[index];
    *out = 0;
    if ((el->flags & 0x8) == 0) {
        return 0;
    }
    if ((el->flags & 0x2) == 0) {
        return 0;
    }
    rest = el->value - quest_item_count_sum(el->id, 0);
    if (rest < 0) {
        rest = 0;
    }
    *out = rest;
    return el->id;
}

/* The item `id`'s count: `who` 0 sums every player's slot, 1 the local player's, 2 and up the
 * player whose slot follows. */
s16 quest_item_count_sum(u16 id, s32 who) {
    QuestWork* work;
    QuestItemSlot* slots;
    s16 total = 0;
    s32 player;
    s32 i;
    s32 j;

    if (get_move_work_adrs(0) == NULL) {
        return 0;
    }
    switch (who) {
    case 0:
        work = quest_work_ptr;
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++) {
                if (work->player_items_0x6A6A[i][j].id == id) {
                    total += work->player_items_0x6A6A[i][j].count;
                }
            }
        }
        break;
    case 1:
        player = (s8)my_player_no();
        slots = &quest_work_ptr->player_items_0x6A6A[player][0];
        for (i = 0; i < 3; i++) {
            if (slots[i].id == id) {
                return slots[i].count;
            }
        }
        break;
    default:
        player = (s8)(who - 2);
        slots = &quest_work_ptr->player_items_0x6A6A[player][0];
        for (i = 0; i < 3; i++) {
            if (slots[i].id == id) {
                return slots[i].count;
            }
        }
        break;
    }
    return total;
}

/* Stores `value` into player `player`'s slot holding arena item `id` (taking a free slot when none
 * does), then clears every element whose remaining count has run out. */
void quest_arena_data_step(u8 player, u16 id, s16 value) {
    QuestItemSlot* slots = quest_work.player_items_0x6A6A[player];
    s16 count = 0;
    s16 rest;
    s32 i;

    if (slots[0].id == id) {
        count = slots[0].count;
    }
    if (slots[1].id == id) {
        count = slots[1].count;
    }
    if (slots[2].id == id) {
        count = slots[2].count;
    }
    if (count == 0) {
        for (i = 0; i < 3; i++) {
            if (slots[i].id == 0) {
                slots[i].id = id;
                slots[i].count = value;
                break;
            }
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (slots[i].id == id) {
                slots[i].count = value;
                break;
            }
        }
    }
    for (i = 0; i < 3; i++) {
        rest = (s16)quest_element_value_at(id, (u16)i);
        if (rest < 0) {
            continue;
        }
        rest = rest - (s16)quest_all_player_item_count_sum(id);
        if (rest <= 0) {
            quest_element_reset((u8)i);
        }
    }
}

/* The text a result-screen field kind shows: kinds 0..31 pick one of the record's own text runs or
 * one of this band's text getters, and a null record falls back to the "name unavailable" table. */
char* quest_result_field_text_get(QuestRecord* rec, u8 kind) {
    char buf[256];
    char* brk;

    buf[0] = 0;
    quest_text_buffer[0] = 0;
    if (rec == NULL) {
        if (kind == 0) {
            strcpy(quest_text_buffer, quest_name_unavailable_text[system_w.field_0x09]);
        } else {
            sprintf(quest_text_buffer, " ");
        }
        return quest_text_buffer;
    }
    switch (kind) {
    case 0:
        msg_str_gen(rec->field_0x000, quest_text_buffer);
        break;
    case 22:
        msg_str_gen(rec->field_0x000, buf);
        if ((u32)flfntStrLen(buf) > 22) {
            brk = flKnjMsgNumPtr(buf, 19);
            if (brk != NULL) {
                *brk = 0;
                strcat(buf, "...");
            }
        }
        strcpy(quest_text_buffer, buf);
        break;
    case 1:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        break;
    case 16:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        brk = flfntStrChr(quest_text_buffer, 10);
        if (brk != NULL) {
            *brk = 0;
        }
        break;
    case 17:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        brk = flfntStrChr(quest_text_buffer, 10);
        if (brk != NULL) {
            strcpy(buf, brk + 1);
            strcpy(quest_text_buffer, buf);
        } else {
            quest_text_buffer[0] = 0;
        }
        break;
    case 2:
        if (quest_flag_10000000_ck(rec) == 1) {
            return quest_grade_text_get(0);
        }
        msg_str_gen(rec->field_0x08C, quest_text_buffer);
        break;
    case 3:
        msg_str_gen(rec->field_0x0B5, quest_text_buffer);
        break;
    case 4:
        msg_str_gen(rec->field_0x0DE, quest_text_buffer);
        break;
    case 5:
        msg_str_gen(rec->field_0x13C, quest_text_buffer);
        break;
    case 6:
        return quest_field198_text_get_of(rec);
    case 7:
        msg_str_gen(rec->field_0x19A, quest_text_buffer);
        break;
    case 8:
        msg_str_gen(rec->field_0x1C9, quest_text_buffer);
        break;
    case 9:
        return quest_name_text_get_of(rec);
    case 10:
        return quest_field13A_text_get_of(rec);
    case 11:
        return quest_time_text_get_of(rec, 0);
    case 13:
        return quest_time_text_get_of(rec, 1);
    case 14:
        return quest_time_text_get_of(rec, 2);
    case 15:
        return quest_time_text_get_of(rec, 3);
    case 12:
        return quest_field348_text_get_of(rec);
    case 30:
        return quest_monster_text_get(rec, 0);
    case 31:
        return quest_monster_text_get(rec, 1);
    case 26:
        return quest_arena_time_text_get(rec, 0);
    case 27:
        return quest_arena_time_text_get(rec, 1);
    case 28:
        return quest_arena_time_text_get(rec, 2);
    default:
        break;
    }
    return quest_text_buffer;
}

}  /* extern "C" */
