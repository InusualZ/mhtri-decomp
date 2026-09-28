/* quest/quest_entry.cpp - the quest entry/init band's record counters and item-slot copies.
 *
 * `.text` 0x803AA4A4..0x803B0F98 (27380 B, 70 functions), `extab` 0x80018A6C..0x80018C54 (60 records),
 * `extabindex` 0x80038E08..0x800390E4 (60 x 12 B) - each run is exactly the gap between the bracketing
 * registrations (`menu/multi_result.cpp` below, `Network/fn_803D3CE8.cpp` above).  Registered from
 * `proposal/803AA4A4_fn_803AA4A4.cpp`; the seam is UNPROVEN (the discovery `--max-bytes` cap) and the run
 * is plainly a *sequence* of objects, so the edges are hints, not proven TU boundaries.
 *
 * NAME - which evidence class decided it.  Class 1 (`__FILE__` string) FAILS: the range references no
 * `.cpp`/`.c` string at all (its `.data` references are item-pair tables, jump tables and pool floats -
 * checked by relocating every `.text` data reference of the 70 auto objects).  Class 2 (runtime-dump name)
 * fails for the range itself (`dumpmap.py lookup 0x803AA4A4` answers `zz_03aa4a4_` for 69 of the 70
 * addresses), but it *does* name the band's own globals: `q_result_msg_adrs` 0x806C5528,
 * `quest_ex_condition_tbl` 0x80794C48, `em_bui_tbl`/`em_bui_rem_l,h`/`em_hokaku_rem_l,h` 0x80794C28-0x80794C38,
 * `q_npc_snd_func` 0x80794C00 - the `q_`/`quest_`/`em_` family the module is named from, together with the
 * range's one real function name, `quest_init(unsigned char)` at 0x803AD47C.  Class 3 therefore decides the
 * MODULE (`quest`) and the file name: the range drives the quest entry/start flow (`quest_init`, the
 * entry state machine that sends `lb_entry_start_send`, `lb_quest_work_init`) and owns its counters.
 * MARKED GUESS: the file name itself - the original source file name is not evidenced, so
 * `quest_entry.cpp` is derived from what the band does.
 *
 * WHAT IS WRITTEN (address order, first pass).  The record-count accessors and the item-pair copy family:
 * `quest_record_a_count_get`/`quest_record_b_count_get` (a per-kind count read from the save data plus the
 * live item work, and the `0x24..0x27` index spelling that sums the four group totals), their two
 * `_wide` thunks, `quest_item_pair_copy_block`/`_row`/`_cell` (the three slices of a 0xA0 B item record
 * `item_pair_copy` moves out) and `quest_item_slot_find`.  8 of the 70 symbols carry a body; measured
 * against this unit's own split object: 4 at 100 % (`quest_record_a_count_get`, `..._b_...`,
 * `quest_record_a_count_get_wide`, `quest_record_b_count_get_wide`) and 4 in 89.4-94.0 %
 * (`quest_item_pair_copy_cell` 89.44, `quest_item_pair_copy_block` 89.60, `quest_item_slot_find` 90.15,
 * `quest_item_pair_copy_row` 93.97) - unit 4.56 % fuzzy over 27380 B.
 *
 * Everything else in the range is unwritten (0 %) - the biggest first: `fn_803B01C4` (0xB10),
 * `fn_803ABE44` (0x86C), `fn_803AC6B0` (0x660), `fn_803AEED0` (0x5E4), `fn_803AE424` (0x510),
 * `fn_803AF4B4` (0x4D8).  The 16 functions that need no foreign symbol at all are the cheapest next pass
 * (`fn_803AA94C`-adjacent leaves, `fn_803AB438` 0x1DC, `fn_803AB614` 0x114, `fn_803AB728` 0xE4,
 * `fn_803AB80C` 0x108, `fn_803AEB08`/`fn_803AEB7C`, `fn_803AD8F4`, `fn_803AE934`, `fn_803B0D5C`).
 *
 * RESIDUAL / KNOWN DEBT.
 *   * The seam is unproven; the module claim covers the whole run even though the first third (the
 *     item-pair tables and the item-slot list) reads as an item-object band and the last third (the
 *     entry state machine) as a quest one.
 *   * `quest_item_pair_copy_cell` / `_block` / `_row` stop at 89-94 %: the residual is the callee-saved
 *     register assignment only (the target colours `rec`=r29, `dst`/`flagged`=r30, `kind`=r31; ours is a
 *     cyclic shift of that), plus `quest_item_slot_find`'s mask/load scheduling order.  No source shape
 *     tried moved them further; both were kept at the best measured form.
 *   * Foreign callees whose bands are still unregistered keep names derived from their own bodies and
 *     renamed in the map with this change (`move_work_state_ck`, `move_work_item_work_get`,
 *     `quest_slot_progress_get` - the rename swept their 12 reference sites in the five units that
 *     already called them).  Each name is a GUESS; a pass with the runtime dump's own names can sharpen
 *     them.
 *   * `quest_item_work_notify` (0x803AB190, 0x58 B) is unwritten; only its declaration moved here from
 *     `include/unsplit/menu.h`, whose band no longer owns the address (`Pl/pl_act.cpp` is its one
 *     consumer and keeps a local `extern`).
 *   * The `Q_UserData`/`Q_ItemWork`/`Q_MoveWork` views are partial: only the offsets this unit reads are
 *     named, everything between them is padding, and the sizes are marked approximate.
 *   * `menu/menu_item_page.h` and `lobby/lb_companion_ui.h` still declare this unit's symbols themselves
 *     instead of including this header (rule 2's debt; both are headers, so the lint does not fire).  The
 *     `lobby` one also reads `lb_act_best_keep`'s argument as a u16 (`LbActSel::half_0x00`, added with
 *     this change) where it used to read a byte - the target loads `lhz` there, and the fix took that
 *     function 94.03 -> 99.59 %.
 */

#include "quest/quest_entry.h"

/* The recorded count of `kind` in the first record set, plus the live item work's copy of it.  The index
 * spelling 0x24..0x27 sums the set's four group totals instead of reading a count. */
s32 quest_record_a_count_get(u32 kind) {
    Q_UserData* user = get_userdata();
    s32 recorded;

    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = user->set_a.total[0] + user->set_a.total[1];
        sum += user->set_a.total[2];
        sum += user->set_a.total[3];
        recorded = sum;
    } else {
        recorded = user->set_a.count[(u8)kind];
    }
    if (move_work_state_ck() == 1) {
        return recorded;
    }
    Q_ItemWork* work = move_work_item_work_get();
    if (work == NULL) {
        return recorded;
    }
    s32 live;
    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = work->set_c.total[0] + work->set_c.total[1];
        sum += work->set_c.total[2];
        sum += work->set_c.total[3];
        live = sum;
    } else {
        live = work->set_c.count[(u8)kind];
    }
    return (u16)(recorded + live);
}

/* The same accessor for a caller that passes the index wider than a byte. */
s32 quest_record_a_count_get_wide(s32 kind) {
    return quest_record_a_count_get((u8)kind);
}

/* The recorded count of `kind` in the second record set, plus the live item work's copy of it. */
s32 quest_record_b_count_get(u32 kind) {
    Q_UserData* user = get_userdata();
    s32 recorded;

    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = user->set_b.total[0] + user->set_b.total[1];
        sum += user->set_b.total[2];
        sum += user->set_b.total[3];
        recorded = sum;
    } else {
        recorded = user->set_b.count[(u8)kind];
    }
    if (move_work_state_ck() == 1) {
        return recorded;
    }
    Q_ItemWork* work = move_work_item_work_get();
    if (work == NULL) {
        return recorded;
    }
    s32 live;
    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = work->set_d.total[0] + work->set_d.total[1];
        sum += work->set_d.total[2];
        sum += work->set_d.total[3];
        live = sum;
    } else {
        live = work->set_d.count[(u8)kind];
    }
    return (u16)(recorded + live);
}

/* The same accessor for a caller that passes the index wider than a byte. */
s32 quest_record_b_count_get_wide(s32 kind) {
    return quest_record_b_count_get((u8)kind);
}

/* Zeroes the first slot, then copies the item record's leading slice into it: 0x10 slots while the
 * caller's `kind` is 2 or the slot's own progress flag is set, 0x18 otherwise. */
void quest_item_pair_copy_block(Q_ItemPair* dst, u16 id, u8 kind) {
    dst[0].id = 0;
    dst[0].num = 0;
    if (id != 0xFFFF) {
        Q_ItemPair* rec;
        s32 flagged = 0;

        if (id >= 0x64) {
            rec = q_item_pair_tbl_high[id - 0x64];
        } else {
            rec = q_item_pair_tbl_low[id];
            if (quest_slot_progress_get(0) == 1) {
                flagged = 1;
            }
        }
        if (kind == 2) {
            flagged = 1;
        }
        if (rec != 0) {
            s32 n = 0x10;
            if (flagged == 0) {
                n = 0x18;
            }
            for (s32 i = 0; i < n; i++) {
                item_pair_copy(dst, rec);
                dst++;
                rec++;
            }
        }
    }
}

/* Copies one 8-slot row of an item record into the caller's buffer, when the row's `kind` admits it. */
void quest_item_pair_copy_row(Q_ItemPair* dst, u16 id, s8 row, u8 kind) {
    if (row < 1) {
        return;
    }
    if (id == 0xFFFF) {
        return;
    }
    if (kind == 1) {
        if (quest_slot_progress_get(0) != 1) {
            return;
        }
    } else if (kind != 2) {
        return;
    }
    if (kind != 2 && id >= 0x64) {
        return;
    }
    Q_ItemPair* rec;
    if (id >= 0x64) {
        rec = q_item_pair_tbl_high[id - 0x64];
    } else {
        rec = q_item_pair_tbl_low[id];
    }
    if (rec == 0) {
        return;
    }
    rec += (row - 1) * 8 + 0x10;
    dst += (row - 1) * 8 + 0x10;
    for (s32 i = 0; i < 8; i++) {
        item_pair_copy(dst, rec);
        dst++;
        rec++;
    }
}

/* Copies one 4-slot cell of an item record into the caller's buffer at the cell's own offset. */
void quest_item_pair_copy_cell(Q_ItemPair* dst, u16 id, s8 col) {
    if (id == 0xFFFF) {
        return;
    }
    Q_ItemPair* rec;
    if (id >= 0x64) {
        rec = q_item_pair_tbl_high[id - 0x64];
    } else {
        rec = q_item_pair_tbl_low[id];
    }
    if (rec == 0) {
        return;
    }
    dst += col * 4 + 0x20;
    rec += col * 4 + 0x20;
    for (s32 i = 0; i < 4; i++) {
        item_pair_copy(dst, rec);
        dst++;
        rec++;
    }
}

/* The count of the five-slot list's entry for `id`, or 0 when the list has none. */
s8 quest_item_slot_find(Q_ItemCount* slots, u16 id) {
    if (slots[0].id == id) {
        return (s8)slots[0].num;
    }
    if (slots[1].id == id) {
        return (s8)slots[1].num;
    }
    if (slots[2].id == id) {
        return (s8)slots[2].num;
    }
    if (slots[3].id == id) {
        return (s8)slots[3].num;
    }
    if (slots[4].id == id) {
        return (s8)slots[4].num;
    }
    return 0;
}
