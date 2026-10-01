/*
 * em_pop.cpp - the enemy population/roster manager (`enemy` module).
 *
 * `.text` 0x803B465C..0x803B936C (82 symbols), extab 0x80018DC4..0x80018FEC, extabindex 0x8003930C..0x80039648,
 * plus the unit's `.ctors`/`.data`/`.bss`/`.sdata`/`.sbss`/`.sdata2` claims.
 *
 * Phase 4 recut: the registered range was 0x803B465C..0x803BE30C (139 symbols / 40112 B); its tail from 0x803B936C
 * moved to `enemy/em_model.cpp` (with `em_pop_w` and `em_handle_tbl`, which that unit now defines).  The notes
 * below describe the whole former range, whose seam analysis the recut applies.
 *
 * What it is.  The range is the field-side manager of the per-map enemy population: it walks the
 * monster roster (the 0x224-byte `EmPopRec` records), releases and recycles them, and its own string
 * pool names the data it consumes - `05/em_set/em_set_m%02da%02d_%03d.esd` (0x805F81F4),
 * `m%03d_%06d_c_pop.dat` (0x805F84E0) and `B-L-p02-ankou` (0x805F84B0).  Nine registered
 * `src/enemy/*` units call into the range (five of them call 0x803B9BA0; `enemy/fn_80138074.c`
 * drives the three roster helpers this file now defines), and `enemy/fn_8035E034.cpp` already
 * documents the roster record this file's accessor hands back.
 *
 * Naming evidence (brief section 2, in order).  1. No `__FILE__` string covers the range: every
 * `lis`+`addi` pair in 0x803B465C..0x803BE30C resolves to a numeric table, never a bare source-file
 * name (checked against the DOL's `.data`/`.rodata`/`.sdata` bytes).  2. `dumpmap.py lookup` answers
 * `zz_` for every symbol in the range - no real runtime-dump name.  3. The bracketing registered
 * units name different modules (`menu/multi_result.cpp` below, `Network/NetworkSessionManager.cpp` above), so
 * no neighbour scheme reaches the range.  4. **GUESS**, recorded here as the brief requires: module
 * `enemy` and file name `em_pop` are derived from what the range does (it builds, indexes and
 * recycles the per-map enemy population the `em_set`/`_pop.dat` files describe), plus the callers -
 * nine `src/enemy/*` units.  Every function name in this file is likewise derived from its own body.
 *
 * Status / residuals (this pass: 69 of 139 bodies, unit 27.16 % fuzzy; 59 rows at 100 %, 68 at or above 80 %,
 * mean 99.1 % over the 69 written; object .text 10996 B of the target's 40112 B).
 *  - Seam UNPROVEN and almost certainly WRONG (docs/plan.md 8.3): the range is several translation units.
 *    Evidence, all from the target object: (a) the `.sdata2` pool repeats values at separate addresses - 0.0f
 *    at 0x8079C530/C5A8/C5E0, 60.0f at C524/C568, 50.0f at C578/C5D8, the two int-to-float magics at
 *    C528/C610 and C570/C600 - and MWCC keeps one pool per TU; (b) the pool entries from 0x8079C568 on are
 *    monotone in first-use address and tile into three bands, 0x803B7CE8..0x803B8A48 (C568..C5B4, the em_set
 *    loader band), 0x803B9588..0x803B9D74 (C5B8..C5D0, the roster spawn band) and 0x803BA6F0..0x803BE1A8
 *    (C5D8..C628, the model band), while 0x8079C524..C560 is read by `fn_803B6998`/`fn_803B6B14`, before the
 *    first of them, and is shared with `quest/quest_entry` (poolseams fold, 6 shared
 *    literals); (c) the one `.ctors` word names `fn_803B92D4` (`lis`+`b fn_803B92E0`, a static-initializer
 *    thunk over the `.bss` object at 0x806CC420 whose constructor `fn_803B92E0`/`fn_803B9338` follows it),
 *    which closes the em_set TU near 0x803B936C, and `.sdata` 0x80793688..0x807936A0 and `.sbss` 0x80794C60/64
 *    are read across those bands; (d) `tudiscover` answers MATCH SET 0x803B7CE8..0x803B91A0 (16 functions) and
 *    0x803BA6F0..0x803BE30C (35 functions).  Proposed tiling (unmeasured): [0x803B465C, ~0x803B7490) joins
 *    quest_entry + arena_result; [~0x803B7490, 0x803B936C) is the em_set TU (owns `.ctors`, the `.bss` object,
 *    `lbl_80793688`); [0x803B936C, 0x803BA6F0) is the roster spawn TU; [0x803BA6F0, 0x803BE30C) is the model
 *    TU.  The `.sdata`/`.sdata2` claims are blocked on that re-cut (datagap deferred: isolated-run).
 *  - Data claims are PROVISIONAL: seam suspected, the claims are the 19 sole-owned pairs only (`.data` 0x805F7C68..0x805F84E0,
 *    `.bss` 0x806CC420..0x806D2AF8 incl. the em_set TU's object, which only this unit's bodies read, `.sbss` 0x80794C48..0x80794C60);
 *    revisit at the seam recut.  This file defines `em_pop_w`, `em_handle_tbl` and, through its switch, a jump table (`@NNNN`
 *    where the target has `jumptable_805F7C68`); the rest stays target-only.  `.sdata`/`.sdata2` pairs are deferred by the gate.
 *    The `.ctors` word needs the em_set TU's static object and is not emitted here.
 *  - Row residuals: `em_roster_record_result_get` 71.11 % (the target folds the {3,4} arms with one
 *    `subi`+`cmplwi` and takes out-of-line returns; the plain switch emits two compares - 188 B vs 184 B,
 *    if-chain and nested-switch variants measured 49-60 %); `em_roster_record_get` 86.56 % (the target keeps
 *    the record in r4 and moves it to r3 only for the return, ours takes `beqlr` - 56 B vs 64 B; four
 *    spellings measured); `em_roster_record_copy` 98.26 % (copy-pointer registers); `em_weight_table_pick`
 *    95.10 % (the target reloads the weight in both loops and tests the first entry before the loop: 196 B vs
 *    188 B); `quest_all_player_item_count_sum` 95.87 % (the `li` of the element index is scheduled before
 *    the pool loads); `stepStagingDownloadForVersion` 96.81 %, `quest_element_progress_step` 98.54 %,
 *    `quest_slot_byte_get` 99.51 %, `quest_arena_need_get` 98.92 %, `em_work_slot_pair_get` 98.89 % (all
 *    register numbering of loop temporaries or one `lha`/`lhz`; same size).
 *  - GUESS names (evidence: the flag masks, offsets and callers named in each comment; none from the dump):
 *    `quest_flag_*_ck` for the unnamed masks, `quest_slot_byte_get`/`quest_slot_word_get`, `quest_field*_get`,
 *    `quest_objective_result_get`, `quest_element_state_find`/`_404_ck`/`_window_check`/`_progress_step`,
 *    `quest_arena_value_clear_ck`/`_key_clear`, `quest_key_row_ck`/`_key20_flag_ck`, `quest_move_*`,
 *    `quest_pouch_items_settle`, `quest_result_field_text_cur_get`, `stepStagingDownloadForVersion`,
 *    `em_weight_table_pick`, `em_roster_kind_*`, `em_team_damage_under_ck`/`_over_ck` (in `enemy/fn_8012EC74`).
 *    `quest_flag_2000000_ck` and `quest_flag_800000_ck` are two-bit predicates (0x2000000 or 0x100;
 *    0x800000 or exactly 0x40 of 0xC0); their names predate this pass.
 *
 * Flags probed: this unit is built with the address neighbour's `cflags_menu`
 * (`cflags_main` + `-opt nopeephole`).  The range's 139 functions carry `-Cpp_exceptions on` extab
 * (116 records in the target's extab run 0x80018DC4..0x80019164), which `cflags_main` supplies;
 * `-opt nopeephole` is inherited from `menu/multi_result.cpp`; no function here needed a per-function pragma.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad/vec3.h"
#include "ef/fn_800CDB2C.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "enemy/em_pop.h"
#include "enemy/em_model.h"
#include "quest/quest_entry.h"
#include "ef/get_move_work_adrs.h"
#include "g3d/g3d_anmchr.h"
#include "quest/quest_types.h"
#include "enemy/enemy_control.h"
#include "enemy/em020_ai.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/ENEMY_WORK.h"
#include "fn_8004CAD8/get_qResult_work.h"
#include "menu/menu_item.h"
#include "Pl/fn_80273B14.h"
#include "Network/network_pat_control.h"
#include "unsplit/unknown.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "lobby/lb_quest_screen.h"
#include "ef/eft052.h"
#include "enemy/fn_801251D0.h"

extern "C" {


/* The text a result-screen field kind shows for the current row: kinds 0..31 pick one of the row's own text
 * runs or one of the quest band's text getters. */
char* quest_result_field_text_cur_get(u8 kind) {
    char buf[256];
    char* brk;
    QuestRecord* rec = quest_record_get();

    quest_text_buffer[0] = 0;
    buf[0] = 0;
    if (rec != NULL) {
        switch (kind) {
        case 0:
        case 22:
            msg_str_gen(rec->field_0x000, quest_text_buffer);
            break;
        case 1:
            msg_str_gen(rec->field_0x02E, quest_text_buffer);
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
        case 7:
            msg_str_gen(rec->field_0x19A, quest_text_buffer);
            break;
        case 8:
            msg_str_gen(rec->field_0x1C9, quest_text_buffer);
            break;
        case 16:
            msg_str_gen(rec->field_0x02E, quest_text_buffer);
            brk = flfntStrChr(quest_text_buffer, 10);
            if (brk != NULL) {
                *brk = 0;
            }
            break;
        case 17:
            if (quest_flag_10000000_ck(rec) == 1) {
                msg_str_gen(rec->field_0x08C, quest_text_buffer);
            } else {
                msg_str_gen(rec->field_0x02E, quest_text_buffer);
                brk = flfntStrChr(quest_text_buffer, 10);
                if (brk != NULL) {
                    strcpy(buf, brk + 1);
                    strcpy(quest_text_buffer, buf);
                } else {
                    quest_text_buffer[0] = 0;
                }
            }
            break;
        case 6:
            return quest_field198_text_get();
        case 9:
            return quest_name_text_get();
        case 10:
            return quest_field13A_text_get();
        case 11:
            return quest_time_text_get(0);
        case 13:
            return quest_time_text_get(1);
        case 14:
            return quest_time_text_get(2);
        case 15:
            return quest_time_text_get(3);
        case 12:
            return quest_field348_text_get();
        case 18:
            return quest_clear_time_text_get();
        case 19:
            return quest_field2E8_text_get();
        case 20:
            return quest_score_text_get();
        case 21:
            return quest_elapsed_time_text_get();
        case 23:
            return quest_arena_items_text_get(0);
        case 24:
            return quest_arena_items_text_get(1);
        case 25:
            return quest_arena_items_text_get(2);
        case 26:
            return quest_arena_time_text_get(rec, 0);
        case 27:
            return quest_arena_time_text_get(rec, 1);
        case 28:
            return quest_arena_time_text_get(rec, 2);
        case 29:
            return quest_grade_text_cur_get();
        case 30:
            return quest_monster_text_get(rec, 0);
        case 31:
            return quest_monster_text_get(rec, 1);
        }
    }
    return quest_text_buffer;
}

/* The row's halfword at +0x37A (0 when there is no row). */
u16 quest_field37A_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x37A;
}

/* The slot's value byte: from the armed byte run when the 0x2000000 test passes, else from the
 * halfword run while the matching state bits are set; 0 when nothing applies. */
u8 quest_slot_byte_get(QuestRecord* rec, u8 slot) {
    QuestRecord* row;
    if (rec == NULL) {
        row = quest_record_get();
        if (row == NULL) {
            return 0;
        }
    } else {
        row = rec;
    }
    if (slot >= 3) {
        return 0;
    }
    if (quest_flag_2000000_ck(row) == 1) {
        if (row->slot_bytes_0x314[slot].armed == 0) {
            return 0;
        }
        return row->slot_bytes_0x314[slot].value;
    }
    QuestRecordSlotWord* word = &row->slot_words_0x330[slot];
    u32 flags = row->flags_0x310;
    if (flags & 5) {
        u32 slot_flags = word->flags;
        if ((slot_flags & 1) || (slot_flags & 0x400) || (slot_flags & 0x4000)) {
            return (u8)word->value;
        }
    }
    if (flags & 0x200000) {
        if (row->slot_bytes_0x314[slot].armed == 0) {
            return 0;
        }
        return row->slot_bytes_0x314[slot].value;
    }
    return 0;
}

/* The slot's halfword value when both the row and the slot carry state bit 2; else 0. */
u16 quest_slot_word_get(QuestRecord* rec, u8 slot) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (slot >= 3) {
        return 0;
    }
    QuestRecordSlotWord* word = &rec->slot_words_0x330[slot];
    if (rec->flags_0x310 & 2) {
        if (word->flags & 2) {
            return word->value;
        }
    }
    return 0;
}

/* Whether the row carries flag 0x100 or has its +0x32C byte set. */
u32 quest_field32C_ck(QuestRecord* rec) {
    QuestRecord* row;
    if (rec == NULL) {
        row = quest_record_get();
        if (row == NULL) {
            return 0;
        }
    } else {
        row = rec;
    }
    if (quest_flag_100_ck(row) == 1) {
        return 1;
    }
    return row->field_0x32C != 0;
}

/* The row's byte at +0x32E (0 when there is no row). */
u8 quest_field32E_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x32E;
}

/* Whether the result row carries flag 0x8. */
u32 quest_flag_8_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x8) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x80000. */
u32 quest_flag_80000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x80000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x2000000 or flag 0x100. */
u32 quest_flag_2000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x2000000) {
        return 1;
    }
    return (rec->flags_0x310 & 0x100) != 0;
}

/* Whether the result row carries flag 0x80000000. */
u32 quest_flag_80000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x80000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x4000000. */
u32 quest_flag_4000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x4000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x100. */
u32 quest_flag_100_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x100) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x10000000. */
u32 quest_flag_10000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x10000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x100000. */
u32 quest_flag_100000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x100000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x40000000. */
u32 quest_flag_40000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x40000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x1000000. */
u32 quest_flag_1000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x1000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x10. */
u32 quest_flag_10_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x10) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x200000. */
u32 quest_flag_200000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x200000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x800000, or has exactly flag 0x40 of the 0xC0 pair. */
u32 quest_flag_800000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x800000) {
        return 1;
    }
    return (rec->flags_0x310 & 0xC0) == 0x40;
}

/* Whether the result row carries flag 0x40000. */
u32 quest_flag_40000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x40000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x80. */
u32 quest_flag_80_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x80) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x2000000 or flag 0x80000000. */
u32 quest_flag_2000000_or_80000000_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x2000000) {
        return 1;
    }
    return (rec->flags_0x310 & 0x80000000) != 0;
}

/* Whether the result row carries flag 0x2000000 (the single-bit twin of the composite test above). */
u32 quest_flag_2000000_only_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x2000000) {
        return 1;
    }
    return 0;
}

/* Whether the result row carries flag 0x800. */
u32 quest_flag_800_ck(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    if (rec->flags_0x310 & 0x800) {
        return 1;
    }
    return 0;
}

/* The objective code of a result row (the current one when `rec` is NULL), from its flag word: 1, 9, 2,
 * 3 or 4, and 0 while the slot is in its entry state or no row exists. */
u8 quest_objective_get(QuestRecord* rec) {
    u8 objective = 0;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec != NULL || (rec = quest_record_get()) != NULL) {
        u32 flags = rec->flags_0x310;
        if (flags & 2) {
            objective = 1;
        } else if (flags & 8) {
            objective = 9;
        } else {
            u32 pair = flags & 5;
            if (pair == 5) {
                objective = 2;
            } else {
                if (pair == 1) {
                    objective = 3;
                }
                if (pair == 4) {
                    objective = 4;
                }
            }
        }
    }
    return objective;
}

/* The same objective code with the 0x200000 state reported as 7 (the result screen's own reading). */
u8 quest_objective_result_get(QuestRecord* rec) {
    u8 objective = 0;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec != NULL || (rec = quest_record_get()) != NULL) {
        u32 flags = rec->flags_0x310;
        if (flags & 8) {
            objective = 9;
        } else if (flags & 0x200000) {
            objective = 7;
        } else if (flags & 2) {
            objective = 1;
        } else {
            u32 pair = flags & 5;
            if (pair == 5) {
                objective = 2;
            } else {
                if (pair == 1) {
                    objective = 3;
                }
                if (pair == 4) {
                    objective = 4;
                }
            }
        }
    }
    return objective;
}

/* Whether the quest is over: the move work is in state 1, flag 0x4000000 is set, or the current row has its
 * +0x32C byte set. */
u32 quest_field32C_or_4000000_ck(void) {
    QuestRecord* rec;
    if (move_work_state_ck() == 1) {
        return 1;
    }
    rec = quest_record_get();
    if (rec == NULL) {
        return 0;
    }
    if (quest_flag_4000000_ck(0) == 1) {
        return 1;
    }
    return rec->field_0x32C != 0;
}

/* The leading word of entry `index` of the current row's third run (0 when there is no row). */
u32 quest_row394_get(u8 index) {
    QuestRecord* rec = quest_record_get();
    if (rec == NULL) {
        return 0;
    }
    return rec->rows_0x394[index].field_0x00;
}

/* Fills the result work's stat block from the item work when a finished quest's record is in state 7: cleared
 * when the item work carries a stat, else the em020 hit info. */
void quest_result_stat_fill(QuestWork* item) {
    Q_ResultWork* result = get_qResult_work();
    _ENEMY_WORK* work = NULL;
    if (item->record_0x03C->field_0x08B == 7) {
        em_get_unique_work(0, &work, NULL);
        if (work != NULL) {
            if (item->stat_gate_0x4B8 != 0) {
                memset(&result->stat_0x3E0, 0, 16);
            } else if (work->team == 20) {
                em020_hit_info_get(work, (Em020HitInfo*)&result->stat_0x3E0);
                result->stat_0x3E0.word_0x0C = item->stat_word_0x10;
            }
        }
    }
}

/* Scans the three elements for the em020 hunt goal, fills the result stat when one matches, and arms the
 * element whose team window has closed; 1 when one was armed. */
u32 quest_element_window_check(QuestWork* work) {
    QuestElement* el;
    s32 i;
    s32 hit;
    get_qResult_work();
    hit = 0;
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x10) && ((el->flags & 0x400) || (el->flags & 0x4000)) && (u8)el->id == 20) {
            hit = 1;
            quest_result_stat_fill(work);
        }
    }
    if (hit) {
        work->entry_send_0x6A3A[1] = 2;
    } else {
        work->entry_send_0x6A3A[1] = 1;
    }
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if (el->flags & 0x10) {
            if ((el->flags & 0x4000) == 0x4000 && em_team_damage_over_ck((u8)el->id, (u8)el->target_count) == 1) {
                quest_element_set(work, i, 0x400);
                work->entry_send_0x6A3A[0] = 1;
                return 1;
            }
            if ((el->flags & 0x404) == 0x404 && em_team_damage_under_ck((u8)el->id, (u8)el->target_count) == 1) {
                quest_element_set(work, i, 0x400);
                work->entry_send_0x6A3A[0] = 1;
                return 1;
            }
        }
    }
    work->entry_send_0x6A3A[1] = 0;
    return 0;
}

/* The state (0x4014, 0x414 or 0x404) of the element whose low id byte is `key`; 0 when none matches. */
u32 quest_element_state_find(u8 key) {
    QuestWork* work = quest_work_ptr;
    u32 flags;
    QuestElement* el;
    s32 i;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (work == NULL) {
        return 0;
    }
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        flags = el->flags;
        if ((flags & 0x4014) == 0x4014 && (u8)el->id == key) {
            return 0x4014;
        }
        if ((flags & 0x414) == 0x414 && (u8)el->id == key) {
            return 0x414;
        }
        if ((flags & 0x404) == 0x404 && (u8)el->id == key) {
            return 0x404;
        }
    }
    return 0;
}

/* Whether any of the three elements carries state 0x404. */
u32 quest_element_404_ck(void) {
    QuestWork* work = quest_work_ptr;
    QuestElement* el;
    s32 i;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (work == NULL) {
        return 0;
    }
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x404) == 0x404) {
            return 1;
        }
    }
    return 0;
}

/* Whether any element whose bit 0x10 is set is in its entry state (0x404 or 0x414): the gate the pair
 * rolls in `quest_pair_roll_all` wait on. */
u32 quest_element_state_ck(void) {
    QuestWork* work = quest_work_ptr;
    u32 flags;
    QuestElement* el;
    s32 i;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (work == NULL) {
        return 0;
    }
    el = work->elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        flags = el->flags;
        if (flags & 0x10) {
            if ((flags & 0x404) == 0x404) {
                return 1;
            }
            if ((flags & 0x414) == 0x414) {
                return 1;
            }
        }
    }
    return 0;
}

/* The arena element `id`'s stored value into `out`; 0 when no element matches. */
s32 quest_element_value_get(u16 id, s16* out) {
    QuestElement* el = quest_work.elements_0x0094;
    s32 i;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x2000) == 0x2000 && el->id == id) {
            if (out != NULL) {
                *out = el->value;
            }
            return 1;
        }
    }
    return 0;
}

/* Element `index`'s stored value, ignoring its id, else -1. */
s32 quest_element_value_index_get(u16 index) {
    QuestElement* el;
    if (move_work_state_ck() == 1) {
        return -1;
    }
    el = &quest_work.elements_0x0094[index];
    if ((el->flags & 0xA) == 0xA) {
        return el->target_count;
    }
    return -1;
}

/* Element `index`'s stored value when `id` matches its own id, else -1. */
s32 quest_element_value_at(u16 id, u16 index) {
    QuestElement* el;
    if (move_work_state_ck() == 1) {
        return -1;
    }
    el = &quest_work.elements_0x0094[index];
    if ((el->flags & 0xA) == 0xA && el->id == id) {
        return el->target_count;
    }
    return -1;
}

/* Fills `out` with element `index`'s id and what is still missing of its target; returns the id. */
u16 em_work_slot_pair_get(u16 index, s16* out) {
    QuestElement* el;
    if (move_work_state_ck() == 1 && out != NULL) {
        out[0] = 0;
        out[1] = 0;
        return 0;
    }
    el = &quest_work.elements_0x0094[index];
    if ((el->flags & 0xA) == 0xA) {
        if (out != NULL) {
            s16 remaining;
            out[0] = el->id;
            remaining = el->target_count - quest_item_count_sum(el->id, 0);
            if (remaining < 0) {
                remaining = 0;
            }
            out[1] = remaining;
        }
        return el->id;
    }
    if (out != NULL) {
        out[0] = 0;
        out[1] = 0;
    }
    return 0;
}

/* Element `index`'s target minus the local player's held count, clamped up to 0. */
s32 quest_element_remaining_get(u16 index) {
    QuestElement* el;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    el = &quest_work.elements_0x0094[index];
    if ((el->flags & 0xA) == 0xA) {
        s32 remaining = el->target_count - quest_item_count_sum(el->id, 0);
        if (remaining < 0) {
            remaining = 0;
        }
        return remaining;
    }
    return 0;
}

/* The item `id`'s count over every player the system work covers; 0 when no element carries the id. */
s32 quest_all_player_item_count_sum(u16 id) {
    s32 i;
    s32 total = 0;
    s32 player;
    QuestItemSlot* slots;
    for (i = 0; i < 3; i++) {
        QuestElement* el = &quest_work.elements_0x0094[i];
        if ((el->flags & 0xA) == 0xA && el->id == id) {
            break;
        }
    }
    if (i >= 3) {
        return 0;
    }
    slots = quest_work.player_items_0x6A6A[0];
    for (player = 0; player < (s8)system_w.field_0x28; player++, slots += 3) {
        if (slots[0].id == id) {
            total += slots[0].count;
        }
        if (slots[1].id == id) {
            total += slots[1].count;
        }
        if (slots[2].id == id) {
            total += slots[2].count;
        }
    }
    return total;
}

/* Whether arena element `id` carrying `value` still has that item recorded; clears the element when the
 * record is the single-use kind. */
u32 quest_arena_value_clear_ck(u16 id, u16 value) {
    s32 i;
    u32 result = 0;
    QuestElement* el;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    el = quest_work.elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x200) == 0x200 && el->id == id && el->target_count == value) {
            if (quest_work.item_bytes_0x6803[el->id * 8 + value] == 1) {
                quest_element_set(&quest_work, i, 0x200);
            }
            if (quest_work.item_bytes_0x6803[el->id * 8 + value] >= 1) {
                result = 1;
            }
        }
    }
    return result;
}

/* Clears the first element that has state 0x1000 for `key` while its value's bit is set in the key's
 * mask; 1 when one was cleared. */
u32 quest_arena_key_clear(u8 key) {
    QuestElement* el;
    s32 i;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    el = quest_work.elements_0x0094;
    for (i = 0; i < 3; i++, el++) {
        if ((el->flags & 0x1000) == 0x1000 && el->id == key &&
            ((1 << el->target_count) & quest_work.key_bits_0x694C[el->id])) {
            quest_element_set(&quest_work, i, 0x1000);
            return 1;
        }
    }
    return 0;
}

/* Runs the per-frame countdown of the armed arena elements matching `kind`: flags the finished ones, posts
 * the remaining-count message every fifth frame and stores the first finished index in `first`. */
u32 quest_element_progress_step(u16 kind, u8 mode, s8* first) {
    u32 armed;
    s32 i;
    s32 shown;
    u32 result = 0;
    QuestElement* el;
    char text[0x20];
    *first = -1;
    if (move_work_state_ck() == 1) {
        return 0;
    }
    armed = quest_flag_80000_ck(0);
    if (armed == 0) {
        armed = quest_flag_2000000_ck(0);
    }
    el = quest_work.elements_0x0094;
    shown = 0;
    for (i = 0; i < 3; i++, el++) {
        if (el->flags & 0x60) {
            continue;
        }
        if ((el->flags & 9) != 9) {
            continue;
        }
        if (el->id != kind && enemy_kind_same_ck((u8)el->id, (u8)kind) == 0) {
            continue;
        }
        if ((el->flags & 0x80) && mode != 1) {
            el->flags |= 0x40;
            *first = i;
            return 0;
        }
        if ((el->flags & 0x100) && mode == 1) {
            el->flags |= 0x40;
            *first = i;
            return 0;
        }
        if (el->target_count != 0) {
            el->target_count--;
        }
        if (el->target_count == 0) {
            quest_element_set(&quest_work, i, 0x100);
            if (*first < 0) {
                *first = i;
            }
            result = 1;
        }
        if (shown == 0) {
            if (el->flags & 0x8000) {
                s32 remaining = el->target_count;
                if (remaining != 0 && remaining % 5 == 0) {
                    sprintf(text, quest_str_tbl_35_get(37), remaining);
                    hud_msg_push(0, text);
                    shown = 1;
                }
            }
            if (el->flags & 0x10000) {
                s32 remaining = el->target_count;
                if (remaining != 0 && remaining % 5 == 0) {
                    sprintf(text, quest_str_tbl_35_get(38), remaining);
                    hud_msg_push(0, text);
                    shown = 1;
                }
            }
            if (el->flags & 0x20000) {
                u32 k;
                s32 total = 0;
                u32 count = (quest_flag_100_ck(0) == 1) ? 2 : 3;
                for (k = 0; k < count; k++) {
                    s32 need = quest_arena_need_get(k);
                    if (need >= 0) {
                        total += need;
                    }
                }
                if (total != 0) {
                    sprintf(text, quest_str_tbl_35_get(37), total);
                    hud_msg_push(0, text);
                    shown = 1;
                }
            }
        }
        if (armed == 1) {
            break;
        }
    }
    return result;
}

/* How many of arena element `index`'s item the player still needs (-1 when the element is not live). */
s32 quest_arena_need_get(s32 index) {
    QuestElement* el;
    u32 flags;
    if (move_work_state_ck() == 1) {
        return -1;
    }
    el = &quest_work.elements_0x0094[index];
    flags = el->flags;
    if (flags != 0) {
        if (flags & 0x200) {
            u16 count = el->target_count;
            if (count != 0) {
                s32 held = quest_work.item_bytes_0x6803[el->id * 8 + count];
                if (held > 1) {
                    held = 1;
                }
                return 1 - held;
            }
        }
        if ((flags & 0x1000) || (flags & 0x2000)) {
            if (flags & 0x20) {
                return 0;
            }
            return 1;
        }
        if (flags & 2) {
            return quest_element_remaining_get(index);
        }
        return el->target_count;
    }
    return -1;
}

/* How many of arena element `index`'s item the player still holds (-1 when the element is not live). */
s32 quest_arena_count_get(s32 index) {
    QuestElement* el;
    u32 flags;
    if (move_work_state_ck() == 1) {
        return -1;
    }
    el = &quest_work.arena_elements_0x01B4[index];
    flags = el->flags;
    if (flags != 0) {
        if (flags & 0x200) {
            return 1;
        }
        if ((flags & 2) || (flags & 1)) {
            return el->target_count;
        }
        if (flags & 0x10) {
            return -1;
        }
        return 1;
    }
    return -1;
}

/* The work block's run at +0x6AA4 (0 while the block is absent or the move work is in state 1). */
u8* quest_field6AA4_get(void) {
    QuestWork* work = quest_work_ptr;
    if (move_work_state_ck() == 1) {
        return NULL;
    }
    if (work == NULL) {
        return NULL;
    }
    return work->field_0x6AA4;
}

/* Whether any of the work block's three key rows holds `key`. */
u32 quest_key_row_ck(u8 key) {
    QuestKeyRow* row;
    s32 i;
    if (&quest_work == NULL) {
        return 0;
    }
    row = quest_work.key_rows_0x0319;
    for (i = 0; i < 3; i++, row++) {
        if (row->key == key) {
            return 1;
        }
    }
    return 0;
}

/* With key 0x14 present, whether flag 0x200000 is set (`invert` 0) or clear (`invert` non-zero). */
u32 quest_key20_flag_ck(u8 invert) {
    if (quest_key_row_ck(0x14) == 0) {
        return 0;
    }
    if (invert == 0) {
        return quest_flag_200000_ck(0) != 0;
    }
    return quest_flag_200000_ck(0) != 1;
}

/* Finds the staged file whose header carries `version` and steps its download; -1 when none does. */
s32 stepStagingDownloadForVersion(u16 version) {
    NetCtrlWk* wk = net_ctrl_wk;
    s32 file;
    s32 i;
    if (wk != NULL) {
        for (i = 0; i < 10; i++) {
            if (((StagingFileHeader*)wk->file_buffers_0xC39C[i])->version_0x2C == version) {
                file = i;
                break;
            }
        }
    }
    if (i >= 10) {
        return -1;
    }
    return NetCtrlWk::stepStagingDownloadIfUp(file, version);
}

/* Whether `buffer`'s staged file header carries `version`. */
u32 matchesFileVersion(const u8* buffer, u16 version) {
    return ((const StagingFileHeader*)buffer)->version_0x2C == version;
}

/* The entry sub-state of the local slot's move work: 1 when it is 4, 0 otherwise, -1 without a work. */
s32 quest_move_sub_state_4_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return -1;
    }
    return work->sub_0xFA == 4;
}

/* Sets the local slot's sub-state to 4 (and its 0x22D9 flag when `flag` is 1). */
void quest_move_sub_state_4_set(u8 flag) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work != NULL) {
        work->sub_0xFA = 4;
        if (flag == 1) {
            work->sub_flag_0x22D9 = 1;
        }
    }
}

/* Settles the carried-item pouch: sells the category-1 pairs (ten each), clears the category-2 pairs (raising
 * `flag`) and drops the ones the player cannot use; returns the credit. */
s32 quest_pouch_items_settle(Q_ItemPair* pouch, u8* flag) {
    s32 i;
    s32 credit = 0;
    *flag = 0;
    for (i = 0; i < 35; i++, pouch++) {
        if (pouch->id != 0 && (s16)pouch->num != 0) {
            if (item_category_ck(pouch->id, 1)) {
                credit += (s16)pouch->num * 10;
                pouch->id = 0;
                pouch->num = 0;
            } else if (item_category_ck(pouch->id, 2)) {
                pouch->id = 0;
                pouch->num = 0;
                *flag = 1;
            } else if (Pl_item_id_usable_ck(pouch->id, 1) == 0) {
                pouch->id = 0;
                pouch->num = 0;
            }
        }
    }
    return credit;
}

} /* extern "C" */
