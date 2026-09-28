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
 * The rename's reference half was swept with it: `fn_803B0CD4` -> `quest_entry_active_ck` in
 * `src/enemy/fn_8012EC74.cpp` (2 sites), the only reference site outside this file.
 *
 * DATA / RULE 12.  Two of the picked candidates were deliberately NOT written because the only thing
 * they need is unowned data, and declaring it would be a new rule-12 finding:
 *   * `quest_item_pair_tbl_copy` (0x803AB3BC, 124 B) walks the two 0xC0-byte `.bss` arrays at 0x806C5558
 *     and 0x806C5618 (`fn_803AB3BC`'s loop count 48 = 0xC0/4 proves the stride).  Nothing claims them, so
 *     the function lands with the `.bss` claim 0x806C5558..0x806C5858 (the four 0xC0 arrays
 *     `lbl_806C5558`/`lbl_806C5618`/`lbl_806C56D8`/`lbl_806C5798`, all of them this unit's) reconstructed as
 *     this unit's own object.
 *   * `quest_pair_apply` (0x803AE990, 376 B) additionally needs `fn_803ADF84` (an unnamed neighbour) and
 *     `lbl_805F76A0`.
 *
 * RESIDUAL / KNOWN DEBT.
 *   * `quest_item_pair_copy_cell` / `_block` / `_row` stop at 89-94 %: the residual is the callee-saved
 *     register assignment only (the target colours `rec`=r29, `dst`/`flagged`=r30, `kind`=r31; ours is a
 *     cyclic shift of that), plus `quest_item_slot_find`'s mask/load scheduling order.  No source shape
 *     tried moved them further; both were kept at the best measured form.
 *   * `quest_item_slot_add` 92.50 %: the residual is the `extsh` temp (`extsh r0,r3` in the target
 *     against our in-place `extsh r3,r3`), the target's unsigned `cmplwi r3,5` against our signed
 *     `cmpwi`, and the target's preserved loop counter (`addi r5,r5,1`; ours is dead-code eliminated).
 *     Three shapes moved it and were kept: the `Q_ItemCount* slot = slots;` declaration belongs *inside*
 *     the branch (+3 points), `Q_ItemCount::num` is `s8` and not `u8` - the target's `stb` stores the
 *     `extsb`'d value raw, where a `u8` member costs a `clrlwi` (+3.8 points, and it took
 *     `quest_item_slot_find` 90.15 -> 91.67 with it) - and the `slots[i]` indexed spelling is *worse*
 *     (84.92, measured), so the moving pointer stays.
 *   * `quest_element_build` is 100 %: its `total`/`entry` assignment order and their declaration order
 *     are load-bearing (the target's accumulator is the lower register), recorded here because both
 *     were needed.
 *   * `quest_element_copy` 85.52 %: instruction-identical, and the 6 trailing word copies are load/store
 *     paired in the target (`lwz r5,8; lwz r0,0xc; stw r5,8; stw r0,0xc; ...`) where ours does one word at
 *     a time.  Same source shape, scheduler placement - recorded, not chased.
 *   * `quest_lot_pick_first` 97.32 / `quest_lot_pick_last` 97.20 / `quest_lot_pick` 97.02: the residual is
 *     the callee-saved colouring (the target maps `chance`->r25, `table`->r26, `out`->r27, `count`->r28,
 *     `picked`->r29, `i`->r30, `entry`->r31; ours keeps the same values in a rotated set) plus, in
 *     `quest_lot_pick_first` rows 40/41 and `quest_lot_pick_last` rows 37/38, the weight-sum compare's
 *     operand order and branch polarity - the target's `cmplw r4,r0; bge` against our `cmplw r0,r4; ble`
 *     - and in all three the two narrow-load REPLACEs (target `cmplwi r6,1` / `extsh r0,r0`, ours
 *     `cmpwi` / `extsb`).
 *     Declaring `chance` as the walked pointer (`u8*`, incremented in the `for`) rather than a separate
 *     `const u8* ch` copy was worth +4 points - the target increments the parameter's own register.
 *   * Foreign callees whose bands are still unregistered keep names derived from their own bodies and
 *     renamed in the map (`move_work_state_ck`, `move_work_item_work_get`, `quest_slot_progress_get` -
 *     the rename swept their 12 reference sites in the five units that already called them).  Each name
 *     is a GUESS; a pass with the runtime dump's own names can sharpen them.
 *   * `quest_item_work_notify` (0x803AB190, 0x58 B) is unwritten; only its declaration moved here from
 *     `include/unsplit/menu.h`, whose band no longer owns the address (`Pl/pl_act.cpp` is its one
 *     consumer and keeps a local `extern`).
 *   * `get_move_work_adrs` is declared in this unit's own header although its owner is
 *     `ef/fn_800CDB2C.cpp`: that owner's header cannot carry it, because `include/hud/cockpit_quest.h`
 *     (`u8*`), `include/lobby/lb_companion_ui.h` (`LbMoveWork*`) and `include/unsplit/ef.h` (`void*`, and
 *     at C scope, i.e. the wrong mangling) each spell the same mangling with a different return type, so a
 *     fourth spelling in the owner header breaks the ten units that include it.  Declared at C++ scope so
 *     the front-end reproduces the map's mangling `get_move_work_adrs__FUc` (a C-scope spelling is
 *     `undefined` at flip time).  The declaration carries a rule-11 marker and the file's rule-2 set is
 *     left at its previous size by moving `move_work_state_ck` to that owner's header, where it already
 *     lived.
 *   * `quest_record_state_get` (0x803ADF48) is renamed but unwritten: its callee `quest_record_get` is
 *     declared only inside `src/menu/arena_result.cpp`, and that unit is another lane's this wave, so the
 *     declaration waits for its owner header.
 *   * Small functions still blocked by a *foreign* name in another lane's registered range (writing them
 *     would add a rule-7 finding to this file): `quest_pair_apply`, `quest_element_build`,
 *     `quest_monster_setup`, `quest_list_load_hunt`/`_arena`, `quest_grade_set`.
 *   * The `Q_UserData`/`Q_ItemWork`/`Q_MoveWork` views are partial: only the offsets this unit reads are
 *     named, everything between them is padding, and the sizes are marked approximate.
 *   * `menu/menu_item_page.h` and `lobby/lb_companion_ui.h` still declare this unit's symbols themselves
 *     instead of including this header (rule 2's debt; both are headers, so the lint does not fire).  The
 *     `lobby` one also reads `lb_act_best_keep`'s argument as a u16 (`LbActSel::half_0x00`, added with
 *     this change) where it used to read a byte - the target loads `lhz` there, and the fix took that
 *     function 94.03 -> 99.59 %.
 *   * `include/menu/menu_item.h`'s `ItemDataRecord` byte at +0x003 was `unused_0x003` until this pass;
 *     `quest_item_slot_add` reads it as the signed cap a slot's count is clamped to, so it is named
 *     `max_num_0x003` now.
 *   * `quest_pl_skill_slot_set` (0x803AB438, 476 B) is still unwritten although every callee it needs
 *     (`Pl_Skill_ck`, `Pl_cat_skill_ck`, `ran_suu`) is declared: its 16-element byte fix-up loop is
 *     unrolled by 8 and the target keeps a live counter the source shape for is not obvious
 *     (`addi r4,r4,7` per iteration, so the counter is not the element index).
 */

#include "quest/quest_entry.h"
#include "ef/fn_800CDB2C.h"    /* `move_work_state_ck` - owned by ef/fn_800CDB2C.cpp (rule 2) */
#include "unsplit/menu.h"       /* `quest_work_ptr` - the band data no registered unit claims */
#include "enemy/em_pop.h"       /* `quest_flag_*_ck` - owned by enemy/em_pop.cpp (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"  /* memset (owner: the Runtime.PPCEABI.H lib) */

/* The band's quest-work pointer in this unit's own view of the record.  `quest_work_ptr` itself is
 * `.sbss` band data `include/unsplit/menu.h` declares, and that header cannot take this unit's
 * offsets, so the two views are cast rather than merged. */
#define QUEST_WORK ((Q_ItemWork*)quest_work_ptr)

/* ---- the item-slot family (0x803AA9D0) ---- */

/* Adds `count` of `id` to the five-slot list.  An id already in the list is merged into its slot and
 * clamped to the item record's own cap; a new id takes the first free slot, or the round-robin slot
 * the caller's rotation byte points at. */
s32 quest_item_slot_add(Q_ItemCount* slots, u8* rot, u16 id, s16 count) {
    ItemDataRecord* rec = GetItemData(id);
    s32 result;
    s32 i;

    if (rec == NULL) {
        return 1;
    }
    result = quest_item_slot_find(slots, id);
    if (result == 0) {
        result = 5;
        Q_ItemCount* slot = slots;

        for (i = 0; i < 5; i++, slot++) {
            if (slot->id == 0 && count > 0) {
                slot->id = id;
                slot->num = (s8)count;
                result = 0;
                break;
            }
        }
        if (result == 5) {
            slot = &slots[(s8)*rot];
            slot->id = id;
            slot->num = (s8)count;
            *rot = *rot + 1;
            if ((s8)*rot >= 5) {
                *rot = 0;
            }
        }
    } else {
        Q_ItemCount* slot = slots;

        for (i = 0; i < 5; i++, slot++) {
            if (slot->id == id) {
                s8 cap = (s8)rec->max_num_0x003;

                if (count > 0 && (s8)slot->num >= cap) {
                    slot->num = cap;
                    result = 3;
                    break;
                }
                slot->num = slot->num + (s8)count;
                if ((s8)slot->num <= 0) {
                    slot->id = 0;
                    slot->num = 0;
                    result = 4;
                    break;
                }
                if ((s8)slot->num > cap) {
                    slot->num = cap;
                    result = 2;
                    break;
                }
                result = 1;
                break;
            }
        }
    }
    return result;
}

/* ---- the band's small state getters ---- */

/* Whether the local slot's move work has its +0x22DC flag byte set. */
u32 quest_move_flag_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return work->flag_0x22DC != 0;
}

/* Whether an item id belongs to the low pair table (every id below 0x64); id 0 is the empty slot. */
u32 quest_item_id_low_ck(u16 id) {
    if (id == 0) {
        return 0;
    }
    return id < 100;
}

/* The quest phase the whole game switches on while no entry is running - 1 unless the slot is in its
 * entry state, in which case the pause gate decides. */
u32 quest_play_state_ck(void) {
    if (move_work_state_ck() == 0) {
        return quest_work_busy_ck();
    }
    return 1;
}

/* The local slot's quest phase byte (+0xE9 of the move work). */
u8 quest_phase_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return work->phase_0xE9;
}

/* Whether the local slot's item work has its arena item count set. */
u32 quest_item_work_flag_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;

    if (work == NULL) {
        return 0;
    }
    item = work->item_work;
    if (item == NULL) {
        return 0;
    }
    return (s8)item->count_0x6A2A != 0;
}

/* Whether the local slot's sub-state byte is the entry pair (6 or 7), or 4. */
u32 quest_move_sub_state_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    u8 state;

    if (work == NULL) {
        return 0;
    }
    state = work->sub_0xFA;
    if (state == 4) {
        return 1;
    }
    return (u8)(state + 250) <= 1;
}

/* ---- the two entry predicates (0x803B0CD4, 0x803B0CFC) ---- */

/* Whether the quest work's entry-send header has been started. */
u32 quest_entry_active_ck(void) {
    if (quest_work_ptr == NULL) {
        return 0;
    }
    return QUEST_WORK->entry_send_0x6A3A[0] != 0;
}

/* Whether the entry may start: the header must be live and the quest's 0x800000 condition satisfied. */
u32 quest_entry_ready_ck(void) {
    if (quest_work_ptr == NULL) {
        return 0;
    }
    if (QUEST_WORK->entry_send_0x6A3A[0] == 0) {
        return 0;
    }
    if (quest_flag_800000_ck(NULL) == 0) {
        return 0;
    }
    return quest_entry_active_ck();
}

/* ---- the two block copies (0x803AEB7C, 0x803AEB08) ---- */

/* Copies the 8-byte slot pair a result row is built from. */
void quest_pair_copy(Q_SlotPair* dst, const Q_SlotPair* src) {
    *dst = *src;
}

/* Copies the 0x20-byte key block one of those rows carries. */
void quest_element_copy(Q_ElementBlock* dst, const Q_ElementBlock* src) {
    *dst = *src;
}

/* ---- the weighted lot-table picks (0x803AB614, 0x803AB728, 0x803AB80C) ---- */

/* Walks `count` rows of `chance`, and for each row whose chance byte admits it rolls a `total`-wide
 * number into the weighted table and appends the entry the roll lands on.  The first row always
 * rolls 0, so it always takes the table's first non-empty entry. */
s32 quest_lot_pick_first(u8* chance, const Q_LotEntry* table, Q_PickPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;

    for (i = 0; i < count; i++, chance++) {
        const Q_LotEntry* entry = table;
        u16 roll;
        u16 acc;
        s32 hit;

        if (entry->id == 0) {
            break;
        }
        if (*chance <= ((u16)ran_suu(0) & 0x1F)) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        if (i == 0) {
            roll = 0;
        }
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (acc > roll) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->value = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The same walk without the first-roll override: every row rolls its own number. */
s32 quest_lot_pick(u8* chance, const Q_LotEntry* table, Q_PickPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;
    const Q_LotEntry* entry;

    for (i = 0; i < count; i++) {
        u16 roll;
        u16 acc;
        s32 hit;

        entry = table;
        if (entry->id == 0) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (roll < acc) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->value = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* ---- the arena element builder (0x803AD008) ---- */

/* Clears the element's payload, then fills it from the item work's own weighted lot table: a table of
 * 3 or 5 rows, chosen by the element's own sub-flag, and only when the caller passed the arena kind. */
/* untyped: caller-owned context payload the target never reads */
void quest_element_build(void* owner, u32 kind, Q_ArenaElement* element) {
    u8 chance[6];
    u16 total;
    Q_LotEntry* entry;
    s32 rows;

    memset(element->payload_0x3F4, 0, sizeof(element->payload_0x3F4));
    if ((u8)kind != 4) {
        return;
    }
    {
        Q_ItemWork* item = move_work_item_work_get();

        memset(chance, 0, sizeof(chance));
        if (element->flag_0x434 == 0) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 12;
            rows = 3;
        } else {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 22;
            chance[3] = 12;
            chance[4] = 12;
            rows = 5;
        }
        if ((u8)kind != 4) {
            return;
        }
        entry = item->lot_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total == 0) {
            return;
        }
        quest_lot_pick_first(chance, item->lot_0x9C, element->payload_0x3F4, rows, total);
    }
}

/* The same walk for the band that gates on the row's own chance byte only. */
s32 quest_lot_pick_last(u8* chance, const Q_LotEntry* table, Q_PickPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;

    for (i = 0; i < count; i++, chance++) {
        const Q_LotEntry* entry = table;
        u16 roll;
        u16 acc;
        s32 hit;

        if (entry->id == 0) {
            break;
        }
        if (*chance <= ((u16)ran_suu(0) & 0x1F)) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (acc > roll) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->value = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

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
s16 quest_item_slot_find(Q_ItemCount* slots, u16 id) {
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
