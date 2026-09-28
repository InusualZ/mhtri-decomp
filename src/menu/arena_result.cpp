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
 * RESIDUAL: 32 of the 56 functions are written - 3656 B of the range's 14020 B, 17 of them at
 * 100 %, 31 at 80 % or better, mean 96.13 % over the 32 scored (unit fuzzy 24.54 %).  The one row
 * below the 80 % bar is `quest_work_word_get` (79.27 %): the target's loop is `mtctr`/`bdnz` over
 * the list count while ours re-reads the count, and the register pair (`items`/`count` in r3/r4
 * retail, r4/r5 ours) is the allocator's (playbook 22).  `quest_item_count_sum` (85.64 %) and
 * `quest_element_value_set` (86.43 %) keep their measured best shape; both are 4-12 B long.
 *
 * The 24 functions still unwritten are blocked by *naming*, not by evidence: each calls another
 * band's starter name (`fn_8029F73C`, `fn_80272E30`, `fn_8027BC48`, `fn_8005C7E0`, `fn_800CF280`,
 * `fn_8042C850`/`CB9C`/`CC20`, `fn_802D8ABC`..`fn_802D8EA8`, `fn_8035B5FC`, `fn_8033A920`,
 * `fn_802AFC94`/`AFE08`, ...), and a call to one in this new file is a rule-7 finding.  Naming them
 * means sweeping every reference site in the units that own them (`fn_802B0668` alone is cited from
 * 30 files of `enemy`, `Pl`, `sound` and `stage`), so the sweep is the batch's `config_requests`
 * entry and not this lane's diff.  The blocked rows are listed with their sizes in the outbox; the
 * two biggest are `fn_803B0F98` (2020 B) and `fn_803B177C` (1596 B).
 *
 * `quest_element_set` is declared in the band header although `lobby/lb_quest_screen.cpp`'s
 * registered range covers its address: that unit landed after this branch was cut, its source does
 * not cite the symbol (0 reference sites in `src/` and `include/`), and the map row is still the
 * starter name - the rename is filed for the landing, and the declaration moves to that unit's
 * header when its body is written.
 *
 * Data (measured, `datagap.py --unit menu/arena_result`): the unit's own `.data` is the three jump
 * tables 0x805F7B78..0x805F7C68 (0xF0 B), claimed in `splits.txt`; the pool constants the band
 * addresses (`frames_per_second_60f`, `percent_scale_100f`, `quest_grade_ratio_*`) are declared
 * `extern` in `include/unsplit/menu.h` and never defined here, so no `.sdata2` is emitted for them.
 *
 * FLAGS: `menu`'s `cflags_menu` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, Wii/1.3).
 */

#include "types.h"
#include "unsplit/menu.h"                 /* the band's own records, tables and callees */
#include "unsplit/unknown.h"              /* `system_w` (+0x09: the message-language index) */
#include "unsplit/Runtime.PPCEABI.H.h"    /* sprintf / strcpy */
#include "unsplit/ef.h"                   /* get_move_work_adrs */
#include "ef/fn_800CDB2C.h"               /* my_player_no */

extern "C" {

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
    f32 frames = frames_per_second_60f * Screen_w.field_0x14;
    s32 minutes = time / (s32)frames;
    s32 seconds = (time - minutes * (s32)frames) / (s32)Screen_w.field_0x14;

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds, Screen_w.field_0x14);
    return quest_text_buffer;
}

/* The elapsed time (`quest_work` +0x1C minus +0x20 frames) as the same text. */
char* quest_elapsed_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = quest_work.field_0x01C - quest_work.field_0x020;
    f32 frames = frames_per_second_60f * Screen_w.field_0x14;
    s32 minutes = time / (s32)frames;
    s32 seconds = (time - minutes * (s32)frames) / (s32)Screen_w.field_0x14;

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds, Screen_w.field_0x14);
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

}  /* extern "C" */
