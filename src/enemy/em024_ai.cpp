/* enemy/em024_ai.cpp - the em024 monster-AI band `.text` 0x8034C1D0..0x80358624
 * Naming note: only the eight `fn_` names this file *references* in OTHER units
 * (`fn_8004D27C`, `fn_8004D70C`, `fn_8004EA24`, `fn_8004EA58`, `fn_8004EAF4`, `fn_8029F818`,
 * `fn_802DA2D4`, `fn_80349914`); every symbol this file *defines* is named below (NAMING).
 *
 * WHAT IT IS.  The bulk of the range (0x8034D124..0x80358624) is the em024 monster's AI: its frames
 * drive the shared `_ENEMY_WORK` record through `em_frame_check`/`em_after_frame_check`/`em_act_ck`/
 * `em_get_mot_no` (the biggest function in the range, `fn_80356664`, is a 0x1A84-byte switch over
 * `em_get_mot_no()`'s 0x72 motions), check the player (`Pl_Skill_ck`, `Pl_master_ck`,
 * `Pl_frame_check`), set the model's TEV material (`MHchar::setTevKColor`/`move`), request sound
 * (`sysSE_req`, `shell_se_req`) and build effect models.  Its `.data` holds the monster's per-motion
 * program tables and the switch jump tables (`jumptable_805E9AD8`, `jumptable_805EBD78`,
 * `jumptable_805EC1C0`, `jumptable_805ECE88`, ...).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable from the
 * range: every `lis`/`addi` pair in the window resolves to a jump table, a `.data` blob or a call,
 * and the only file-name literals in the whole `.data` band (`..\..\menu_note.cpp` at 0x805E91F8) are
 * the registered menu units'.  2. `dumpmap.py lookup` answers `zz_XXXXXXXX_` placeholders for every
 * address probed.  3. The module is `enemy`: the bracketing registered unit after the range is
 * `enemy/fn_8035E034.cpp` and every callee out of the range is enemy-band (`em_frame_check`,
 * `em_get_mot_no`, `em_act_ck`, `get_em_scale`, `fn_8012xxxx`, `fn_80136xxx`).  Classes 1 and 2 give
 * no name, so the FILE NAME and the 14 symbols this file defines are **guesses derived from the
 * range's own tables and bodies**, each one listed with its datum under NAMING.
 *
 * WHY `em024`.  Evidence class 3, the `.data` that names the band.  Of the 32 `em0XX_prog_tbl`
 * monster program tables in the DOL, exactly one references this range: `em024_prog_tbl`
 * (`.data:0x805EBBE0`, 0x70 B), whose entries 0x8034F334, 0x8034F524, 0x803562B4, 0x8034F410,
 * 0x8034F414, 0x80356664 and 0x803580E8 are functions of this band (the same read finds no other
 * table, string or pointer into the range anywhere in `.data`/`.sdata2`).  `em024_prog_tbl` also
 * sits INSIDE the band's own `.data` run: the tables on both sides of it
 * (`jumptable_805EBD78`, `jumptable_805EC1C0`, `jumptable_805ECE88`) are this unit's switch jump
 * tables and relocate only against this range, so the object that emits them emits the monster's
 * program table too.  A neighbour test makes the identification one-directional in the other sense
 * too: the tables of the *other* monsters (`em008_prog_tbl` -> `enemy/fn_8015E854.cpp`, ...) point at
 * their own bands, never at this one.  MARKED GUESS: that em024's table means "this band belongs to
 * monster 024" rather than a shared band the table merely calls into - the table is the monster's
 * own, no second monster references the band, and the band is one 46 KB body of per-motion AI.
 *
 * NAMING (the batch's own symbols, the gate's rule 7 refusal).  The map had a bare `fn_XXXXXXXX`
 * row for all 120 of the range's addresses and the dump answers `zz_` for them, so each of the 14
 * names below is **derived from the function's own body** and marked a guess; a later pass with the
 * screen's own symbols can sharpen them.  All 14 were renamed in `config/RMHE08/symbols.txt` and
 * here in one edit (`symedit.py rename-batch`; playbook 31/48 - objdiff pairs by name, so half a
 * rename measures 0 %).  All 14 sit in the range's menu HEAD (see SEAM), so their scheme is the menu
 * band's own (`get_note_item_slot`, `item_page_option_row_index`) rather than the enemy `em_*` one -
 * the file name follows the band, the symbols follow the code they name:
 *
 *   fn_8034C1D0 -> note_slot_cursor_step   the per-frame step of a note slot's option row: reads the
 *                                          slot, steps `cursor` on the caller's 0x10 flag against
 *                                          `note_slot_cursor_bounds`'s count, then notifies
 *   fn_8034C2B8 -> note_slot_draw_row      draws that slot's row through `item_page_draw_closed_column`
 *   fn_8034C358 -> menu_row_find           finds the 8-row `MenuRowData` table entry matching its argument
 *   fn_8034C4B4 -> menu_row_clear          clears the row at `index`
 *   fn_8034C4F0 -> menu_row_empty_count    counts the table's empty rows
 *   fn_8034C5B4 -> menu_row_reserve        1 = already listed, 2 = a row is free, else evict and 0
 *   fn_8034C614 -> note_slot_cursor_bounds builds a note slot's cursor bounds from the option table
 *                                          `fn_8029F818` returns for its kind
 *   fn_8034C784 -> item_page_option_item_id  maps an item page option id to the item it unlocks, -1
 *                                          when it is not available yet (declared in
 *                                          `include/menu/menu_item_page.h`)
 *   fn_8034C9C0 -> place_list_deactivate_all  clears `active` on every `MenuSlot::place_entries` record
 *   fn_8034CA7C -> place_list_overlap_find   first active record whose rectangle overlaps the box, else -1
 *   fn_8034CB54 -> place_rec_key_ptr         the key word's address, 0 when the list is absent
 *   fn_8034CB84 -> place_list_key_count      counts the active records carrying a key
 *   fn_8034CBE4 -> place_rec_free_get        the first inactive record
 *   fn_8034CC2C -> place_rec_alloc           fills a free record (box, halves, 16-byte body) and
 *                                          returns its index
 *
 * SEAM (brief section 1's warning).  The range is an `attribute.py --max-bytes` cut and the cut is
 * NOT a proven TU boundary - `tudiscover.py at 0x8034C1D0` reports only weak signals and no data
 * ownership.  Evidence collected here: (a) the range's 91 `extabindex` records (0x80036744..
 * 0x80036B88) are contiguous and their function addresses are monotone, and the records just outside
 * belong to the bracketing registered units, so nothing cuts inside; (b) the `.sdata2` pool ownership
 * is monotone across the right edge (0x8079B638 -> `fn_803580E8`, the range's last function;
 * 0x8079B640 -> `fn_803589A8`, the next proposal's first) so the 0x80358624 cut is not visible in the
 * pool either.  What the evidence DOES show is that the range's HEAD is a different subsystem: the 19
 * functions 0x8034C1D0..0x8034D124 are menu-band item-page code (they call `get_note_item_slot`,
 * `item_page_option_*`, `item_page_draw_closed_column` and index the 0x10-byte `MenuRowData` table
 * `fn_8004EA24` returns, and they reference no enemy symbol at all); a `bl` scan over the whole
 * `.text` finds 47 call sites into that head and not one from an enemy unit (`ai/fn_802D44F4.cpp`
 * 18, `menu/menu_infomation.cpp` 9, `ef/eft_slot.cpp` 5, `lobby/fn_80212810.cpp` 4,
 * `lobby/lb_companion_ui.cpp` 3, `menu/menu_item_page.cpp` 1, 7 unregistered).  That makes the registered
 * `menu/menu_note.cpp` (0x8034C0C4..0x8034C1D0, 268 B) a plausible FRAGMENT of a TU that continues
 * into this range - a seam re-draw candidate recorded in this unit's outbox, not resolved here
 * because the head's functions own no data of their own and match either way.  All 14 named symbols
 * are in that head and keep the menu band's scheme (NAMING); if the re-draw lands they move with it,
 * which is why the file name is the band's and not the head's.
 *
 * LANGUAGE AND SECTIONS.  C++ (`-Cpp_exceptions on`, this lib's `cflags_main`): the range reaches
 * genuinely mangled callees (`em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`,
 * `Pl_Skill_ck__FP4_PLWUs`, `MHchar::setTevKColor`, ...) through their real signatures (rule 9), and
 * every plain `fn_` definition below is `extern "C"` so it keeps the map's name (playbook 42).  The
 * target object carries extab 0x80017094..0x8001736C (91 records), extabindex 0x80036744..0x80036B88
 * (91 x 12 B) and `.text` 0x8034C1D0..0x80358624; the object also emits the `.sdata2` pool
 * (0x8079B368..0x8079B640) and the `.data` jump tables its switches use.
 *
 * STATUS / RESIDUALS (measured with `recompile.py --measure` / the official report metric; unit
 * 4.746 % fuzzy over 50260 B of code).  Written and measured, 14 bodies:
 *   * 100.00  `menu_row_empty_count` (196 B), `place_rec_key_ptr` (48 B) - byte-identical.
 *   *  97.50  `place_list_key_count` (96 B);  95.79 `note_slot_draw_row`;  95.21 `menu_row_reserve`;
 *      94.44 `place_rec_alloc`;  88.33 `place_rec_free_get`;  87.02 `place_list_deactivate_all`;
 *      85.24 `item_page_option_item_id`;  84.02 `note_slot_cursor_bounds`;  79.00 `menu_row_clear`.
 *   *  `menu_row_find` 77.85 % - the 8-row match loop.  Retail duplicates the four-halfword compare
 *      block for the kinds 1/2 range and the 3/4/5 range (two identical bodies); this source writes
 *      one `switch` with the two case groups sharing a body, so ours is one block shorter.
 *   *  `place_list_overlap_find` 73.85 % - the rectangle overlap test.  The halving and the four
 *      comparisons match; the loop's record pointer is re-loaded from `place_entries` per iteration
 *      where retail hoists it, so every iteration costs one instruction.
 *   *  `note_slot_cursor_step` 74.55 % - the note-slot cursor step.  The two-byte slot pair is packed
 *      into the HIGH half of `note_slot_cursor_bounds`'s return value in retail (callee: `lhz` +
 *      `slwi r3,r0,16`; caller: `>> 16` + `sth`); this source reproduces that packing exactly, and
 *      the residual is the `fn_802DA2D4`/`return 0` tail's register allocation.
 *   *  `menu_row_clear` 79.00 % - `rows[index].kind = 0`: retail keeps the scaled index and the zero in
 *      separate registers (`slwi r4,r0,4` + `li r0,0` + `stwx r0,r3,r4`), ours reuses one.
 * NOT written (the honest residual, in address order): `fn_8034C350` (8 B), `note_slot_cursor_bounds`'s
 * callee `fn_8034CCBC` (0xB0) and `fn_8034CD6C` (0x70) - the last two read `_SPR_DATA_`'s rectangle and
 * cannot be typed here: `hud/layout.h` (the owner of `_SPR_DATA_`, `fn_802E1978`, `fn_800526F8`) and
 * `menu/menu_item.h` (which `menu/menu_item_page.h` pulls) both define `_mh_ivec2_`, so the two
 * headers collide in one TU - the per-consumer-view split `include/menu/menu_item_page.h` documents.
 * Then `fn_8034CDDC` (0x348) and the whole AI body of the range from `fn_8034D124` on: 106 functions,
 * 47532 B, the largest `fn_80356664` (0x1A84), `fn_8035598C` (0x860), `fn_803544F8` (0x788),
 * `fn_803580E8` (0x53C), `fn_80352E24` (0xD88), `fn_80350D74` (0xBE0).
 * DATA.  No data gap: `datagap.py --unit enemy/em024_ai --mode both` reports only `.shstrtab`
 * (86 vs 81 B) as `ours-extra`, and `.text`/extab/extabindex as `target-extra` (the unwritten
 * bodies).  Our object emits no `.sdata2` pool and no `.data` table yet, so nothing is claimed
 * beyond the three sections in `splits.txt`.
 */

#include "types.h"
#include "menu/menu_item_page.h"
#include "menu/menu_note.h"
#include "menu/menu_item.h"
#include "fn_8004CAD8.h"
#include "ef/fn_800CDB2C.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/unknown.h"

/* 0x80349914 (ef/eft_slot.cpp) - the two-byte copy behind `pair = table[index]`. */
void fn_80349914(u16* dst, const u16* src);

/* 0x8004EAF4 (fn_8004CAD8.cpp) - the 16-byte struct copy MWCC emits for a 4-word assignment. */
void fn_8004EAF4(void* dst, const void* src);

/* ---------------------------------------------------------------------------------------------- *
 * The note/slot pair `note_slot_cursor_bounds` packs into the high half of its return value: byte 0 is the slot
 * table selector, byte 1 the number of rows the table holds.
 * ---------------------------------------------------------------------------------------------- */
typedef union MenuNoteSlot {
    /* +0x00 */ u16 raw;
    struct {
        /* +0x00 */ u8 table;
        /* +0x01 */ u8 count;
    };
} MenuNoteSlot; /* size: 0x2 */

/* One 0x24-byte record of the placement list `MenuWork::slot[0].place_entries` holds: the record's
 * rectangle, the four halves its creator stores, and the 16-byte body copied in at +0x14.  Its only
 * consumer is this unit, so the definition lives here and `menu/menu_item.h` forward-declares it.
 * size: 0x24 */
typedef struct MenuPlaceRec {
    /* +0x00 */ u8 active;
    /* +0x01 */ u8 index;          /* the byte `place_rec_alloc` hands back to its caller */
    /* +0x02 */ u8 pad_0x02[0x6];
    /* +0x08 */ s16 pos_x;          /* the rectangle `place_list_overlap_find`'s overlap test reads */
    /* +0x0A */ s16 pos_y;
    /* +0x0C */ s16 width;
    /* +0x0E */ s16 height;
    /* +0x10 */ s16 field_0x10;
    /* +0x12 */ s16 field_0x12;
    /* +0x14 */ u32 key;            /* the id `place_list_key_count` counts and `place_rec_key_ptr` hands out */
    /* +0x18 */ u8 body[0x0C];      /* the 16-byte body `fn_8004EAF4` copies over +0x14 */
} MenuPlaceRec; /* size: 0x24 */

extern "C" u32 note_slot_cursor_bounds(u8 kind, u8 index);

/* Rejects the note slot's cursor move, then notifies the option row the player picked. */
extern "C" s32 note_slot_cursor_step(u8 note_entry, u8* cursor, u16 flags, u16* word) {
    u8 slot_tbl = 0;
    u8 slot_idx = 0;
    u16 sel = flags | (u16)(*word & 0xF);
    MenuNoteSlot slot;
    s32 row;

    get_note_item_slot(note_entry, &slot_tbl, &slot_idx);
    row = item_page_option_row_index(slot_tbl, slot_idx, (s8)*cursor);
    slot.raw = (u16)(note_slot_cursor_bounds(slot_tbl, slot_idx) >> 16);
    if ((sel & 0x10) != 0) {
        sysSE_req(6);
        u8 next = (u8)(*cursor + 1);
        if ((s8)next < (s8)slot.count) {
            *cursor = next;
        } else {
            fn_802DA2D4(1);
            return 0;
        }
    }
    if ((s8)row >= 0) {
        item_page_option_available((s8)row);
    }
    return 1;
}

/* Draws the note slot's row through the item page's closed-column renderer. */
extern "C" void note_slot_draw_row(u8 note_entry, s8 light, u16 word) {
    u8 slot_tbl = 0;
    u8 slot_idx = 0;
    MenuNoteSlot slot;

    get_note_item_slot(note_entry, &slot_tbl, &slot_idx);
    slot.raw = (u16)(note_slot_cursor_bounds(slot_tbl, slot_idx) >> 16);
    set_blendmode(4, 5, 1);
    item_page_draw_closed_column(slot_tbl, slot_idx, light, (s8)slot.count, 0, word, 4);
}

/* Finds the row whose kind and key words match `entry` in the 8-row item table. */
extern "C" u32 menu_row_find(const MenuRowData* entry) {
    MenuRowData* rows = fn_8004EA24();
    u32 found = 0;

    for (u32 i = 0; i < 8; i++) {
        if (rows[i].kind != 0 && rows[i].kind == entry->kind) {
            switch (rows[i].kind) {
            case 1:
            case 2:
                if (rows[i].equip_kind == entry->equip_kind && rows[i].equip_id == entry->equip_id &&
                    rows[i].item_id == entry->item_id && rows[i].field_0x0A == entry->field_0x0A) {
                    found = 1;
                }
                break;
            case 3:
            case 4:
            case 5:
                if (rows[i].equip_kind == entry->equip_kind && rows[i].equip_id == entry->equip_id &&
                    rows[i].item_id == entry->item_id && rows[i].field_0x0A == entry->field_0x0A) {
                    found = 1;
                }
                break;
            case 6:
                if (rows[i].equip_kind == entry->equip_kind && rows[i].equip_id == entry->equip_id &&
                    rows[i].item_id == entry->item_id && rows[i].field_0x0A == entry->field_0x0A &&
                    rows[i].field_0x0C == entry->field_0x0C) {
                    found = 1;
                }
                break;
            }
        }
    }
    return found;
}

/* Clears the row at `index` in the 8-row item table. */
extern "C" void menu_row_clear(u16 index) {
    MenuRowData* rows = fn_8004EA24();

    rows[index].kind = 0;
}

/* Counts the empty rows of the 8-row item table. */
extern "C" u8 menu_row_empty_count(void) {
    MenuRowData* rows = fn_8004EA24();
    u8 count = 0;

    for (u32 i = 0; i < 8; i++) {
        if (rows[i].kind == 0) {
            count++;
        }
    }
    return count;
}

/* Reserves the caller's row: 1 when it is already listed, 2 when a row is free, else evict. */
extern "C" u32 menu_row_reserve(MenuRowData* entry) {
    if (menu_row_find(entry) != 0) {
        return 1;
    }
    if (menu_row_empty_count() != 0) {
        return 2;
    }
    fn_8004EA58(entry);
    return 0;
}

/* Builds a note slot's cursor bounds from the item page's option table for its kind. */
extern "C" u32 note_slot_cursor_bounds(u8 kind, u8 index) {
    u16* table;
    MenuNoteSlot slot;
    u8 remain;
    s32 i;

    slot.raw = 0;
    switch (kind) {
    case 0:
        table = (u16*)fn_8029F818(0);
        break;
    case 1:
        table = (u16*)fn_8029F818(1);
        break;
    case 2:
        table = (u16*)fn_8029F818(2);
        break;
    case 3:
        table = (u16*)fn_8029F818(3);
        break;
    default:
        table = 0;
        break;
    }
    if (table == 0) {
        return (u32)slot.raw << 16;
    }
    fn_80349914(&slot.raw, table + index);
    remain = slot.count;
    for (i = 0; i < slot.count; i++) {
        if (slot.table != 0xFF) {
            if (item_page_option_item_id((u8)(kind + 1), (u8)(slot.table + i)) == -1) {
                remain--;
            }
        } else {
            s8 out = 0;

            item_page_option_value_string(kind, index, (s8)i, &out);
            if (out == 0) {
                remain--;
            }
        }
    }
    if (remain != 0 && remain != slot.count) {
        slot.count = remain;
    }
    return (u32)slot.raw << 16;
}

/* Maps an item page option id to the item it unlocks, or to -1 when it is not available yet. */
extern "C" s32 item_page_option_item_id(u8 kind, u8 index) {
    s32 value = 1;

    if (kind == 2) {
        switch (index) {
        case 10:
        case 32:
            if (game_ready_ck() == 0) {
                value = fn_8004D27C(14);
            }
            break;
        case 11:
        case 33:
            if (game_ready_ck() == 0) {
                value = fn_8004D27C(15);
            }
            break;
        case 42:
            if (game_ready_ck() == 0) {
                value = fn_8004D70C(2004);
            } else {
                value = fn_8004D70C(14001);
            }
            break;
        case 50:
            value = fn_8004D27C(17);
            break;
        case 74:
            if (game_ready_ck() == 0) {
                value = -1;
            } else {
                value = fn_8004D27C(16);
            }
            break;
        case 104:
            if (game_ready_ck() == 0) {
                value = fn_8004D27C(25);
            } else {
                value = -1;
            }
            break;
        case 105:
            if (game_ready_ck() == 0) {
                value = fn_8004D27C(26);
            } else {
                value = -1;
            }
            break;
        case 106:
        case 107:
            if (game_ready_ck() == 0) {
                value = fn_8004D27C(28);
            } else {
                value = -1;
            }
            break;
        case 108:
        case 109:
        case 110:
        case 111:
            if (game_ready_ck() == 0) {
                value = fn_8004D27C(31);
            } else {
                value = -1;
            }
            break;
        }
    } else if (kind == 1 && index == 12) {
        if (game_ready_ck() != 0) {
            value = 1;
        } else {
            value = -1;
        }
    }
    return value;
}

/* Deactivates every record of the `.bss` placement list. */
extern "C" void place_list_deactivate_all(void) {
    MenuPlaceRec* rec = lbl_806AC8C8.slot[0].place_entries;
    s32 count = lbl_806AC8C8.slot[0].place_count;

    if (rec == 0) {
        return;
    }
    for (s32 i = 0; i < count; i++) {
        rec[i].active = 0;
    }
}

/* Returns the index of the first active record whose rectangle overlaps the given box. */
extern "C" s32 place_list_overlap_find(s16 x, s16 y, s16 width, s16 height) {
    MenuPlaceRec* rec = lbl_806AC8C8.slot[0].place_entries;
    s16 half_w = (s16)(width / 2);
    s16 half_h = (s16)(height / 2);
    s16 left = (s16)(x - half_w);
    s16 right = (s16)(x + half_w);
    s16 top = (s16)(y - half_h);
    s16 bottom = (s16)(y + half_h);

    if (rec == 0) {
        return -1;
    }
    for (s32 i = 0; i < lbl_806AC8C8.slot[0].place_count; i++) {
        if (rec[i].active != 0) {
            s16 rec_x = rec[i].pos_x;
            s16 rec_x2 = (s16)(rec_x + rec[i].width);
            s16 rec_y = rec[i].pos_y;
            s16 rec_y2 = (s16)(rec_y + rec[i].height);

            if (rec_x <= right && rec_x2 >= left && rec_y <= bottom && rec_y2 >= top) {
                return (s8)i;
            }
        }
    }
    return -1;
}

/* Returns the key word of the placement record with the given index. */
extern "C" u32* place_rec_key_ptr(s8 index) {
    if (lbl_806AC8C8.slot[0].place_entries == 0) {
        return 0;
    }
    return &lbl_806AC8C8.slot[0].place_entries[index].key;
}

/* Counts the active placement records carrying the given key. */
extern "C" u8 place_list_key_count(u32 key) {
    u8 count = 0;
    MenuPlaceRec* rec = lbl_806AC8C8.slot[0].place_entries;

    if (rec == 0) {
        return 0;
    }
    for (s32 i = 0; i < lbl_806AC8C8.slot[0].place_count; i++) {
        if (rec[i].active != 0 && rec[i].key == key) {
            count++;
        }
    }
    return count;
}

/* Returns the first free placement record. */
extern "C" MenuPlaceRec* place_rec_free_get(void) {
    MenuPlaceRec* rec = lbl_806AC8C8.slot[0].place_entries;
    s32 count = lbl_806AC8C8.slot[0].place_count;

    if (rec == 0) {
        return 0;
    }
    while (count > 0) {
        if (rec->active == 0) {
            return rec;
        }
        rec++;
        count--;
    }
    return 0;
}

/* Allocates a placement record for the given box and copies `src`'s 16-byte body into it. */
extern "C" u8 place_rec_alloc(const void* src, s16 x, s16 y, s16 width, s16 height, s16 a, s16 b) {
    MenuPlaceRec* rec = place_rec_free_get();

    if (rec == 0) {
        return 255;
    }
    rec->active = 1;
    rec->pos_x = x;
    rec->pos_y = y;
    rec->width = width;
    rec->height = height;
    rec->field_0x10 = a;
    rec->field_0x12 = b;
    fn_8004EAF4(&rec->key, src);
    return rec->index;
}
