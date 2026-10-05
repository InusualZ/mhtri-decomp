/* menu/menu_row.cpp - the menu row / note slot / placement list code, `.text` 0x8034C1D0..0x8034D2B0
 * (27 functions / 4320 B), with its extab 0x80017094..0x800170F4 (12 records), extabindex
 * 0x80036744..0x800367D4 (12 x 12 B) and `.sdata2` 0x8079B368..0x8079B3A8 (the pool `fn_8034CDDC`
 * loads; the written bodies emit no pool).  Re-cut out of `enemy/em024_ai.cpp`: that
 * unit's range 0x8034C1D0..0x80358624 was three translation units, and this is the first of them.
 *
 * WHAT IT IS.  Menu-band item-page code: it calls `get_note_item_slot`, `item_page_option_*` and
 * `item_page_draw_closed_column`, indexes the 0x10-byte `MenuRowData` table `fn_8004EA24` returns,
 * walks the `MenuSlot::place_entries` placement list and draws a sprite through `draw_sprite`.  It
 * references no enemy symbol.  The last nine functions (0x8034D124..0x8034D2B0, 0x2C B each, no
 * extab, no data) are the `MenuRowData` fillers: each stores its kind 1-9 at +0x00, its halfword
 * arguments at +0x04.. and returns the kind - the record `menu_row_find`'s kind switch reads.
 *
 * MODULE AND NAME (evidence order).  1. No `__FILE__` string is reachable from the range (the one
 * string next to it, `..\..\menu_note.cpp` at 0x805E91F8, is `get_note_item_slot`'s).  2. The dump
 * answers only `zz_` placeholders.  3. Module `menu`: 47 call sites into the head come from menu,
 * lobby-menu and `ai/fn_802D44F4` units (see `.pi/notes/em024-recut.md`), none from an enemy unit,
 * and the fillers' callers are `menu/menu_infomation`, `lobby/fn_80212810`, `lobby/lb_companion_ui`
 * and `lobby/lb_quest_ui`.  4. GUESS: the file name `menu_row` is the dominant type
 * (`MenuRowData`: find/clear/empty_count/reserve plus the nine row fillers).
 *
 * SEAMS.  Right edge 0x8034D2B0: the `extabindex` run breaks there (the 12th record is
 * `fn_8034CDDC`, the next is `fn_8034D2B0`), the `.sdata2` pool run breaks there (0x8079B368..
 * 0x8079B3A0 here, 0x8079B3A8.. the player unit's) and the callee set changes from menu/sprite to
 * `_PLW`.  Left edge 0x8034C1D0: NOT proven - `menu/menu_note.cpp` (0x8034C0C4..0x8034C1D0, one
 * 268 B function `get_note_item_slot`, Matching) may be the head of this same TU
 * (`note_slot_cursor_step` calls it first); the string and jump table it owns are the only data
 * the two could share and neither is cited from here.  Inside the range `tudiscover.py` reports
 * only weak cuts (0x8034CBE4 "codegen fingerprint change", 0x8034CCBC): the placement-list
 * functions could be a fourth TU; kept as one until a body or a data claim says otherwise.
 *
 * NAMING (the 14 written symbols; every one is a guess derived from its body, renamed in
 * `config/RMHE08/symbols.txt` and here in one edit, playbook 31/48).  Scheme: the menu band's own
 * (`get_note_item_slot`, `item_page_option_row_index`).
 *
 *   note_slot_cursor_step   per-frame step of a note slot's option row: steps `cursor` on the caller's
 *                           0x10 flag against `note_slot_cursor_bounds`'s count, then notifies
 *   note_slot_draw_row      draws that slot's row through `item_page_draw_closed_column`
 *   menu_row_find           finds the 8-row `MenuRowData` table entry matching its argument
 *   menu_row_clear          clears the row at `index`
 *   menu_row_empty_count    counts the table's empty rows
 *   menu_row_reserve        1 = already listed, 2 = a row is free, else evict and 0
 *   note_slot_cursor_bounds builds a note slot's cursor bounds from the option table `fn_8029F818`
 *                           returns for its kind
 *   item_page_option_item_id maps an item page option id to the item it unlocks, -1 when not yet
 *                           available (declared in `menu/menu_item_page.h`)
 *   place_list_deactivate_all clears `active` on every `MenuSlot::place_entries` record
 *   place_list_overlap_find first active record whose rectangle overlaps the box, else -1
 *   place_rec_key_ptr       the key word's address, 0 when the list is absent
 *   place_list_key_count    counts the active records carrying a key
 *   place_rec_free_get      the first inactive record
 *   place_rec_alloc         fills a free record (box, halves, 16-byte body), returns its index
 *
 * LANGUAGE.  C++ (`-Cpp_exceptions on`); the range reaches mangled callees
 * (`get_lsp_data__FUsP10_mh_ivec2_`, `draw_sprite__FRC10_SPR_DATA_PC10_mh_ivec2_`) through their
 * real signatures (rule 9), and every plain `fn_` definition is `extern "C"`.
 *
 * FLAGS.  This lib's `cflags_menu` (`-opt nopeephole`) scores 8 of the 14 bodies higher than the
 * `cflags_main` they were measured under in `enemy/em024_ai.cpp` (before -> after): `note_slot_draw_row`
 * 95.79 -> 100.00, `place_rec_alloc` 94.44 -> 100.00, `menu_row_reserve` 95.21 -> 99.58,
 * `note_slot_cursor_bounds` 84.02 -> 91.90, `item_page_option_item_id` 85.24 -> 87.66,
 * `menu_row_clear` 79.00 -> 85.67, `note_slot_cursor_step` 74.55 -> 80.95, `place_rec_free_get`
 * 88.33 -> 88.89.  It scores `place_list_overlap_find` LOWER (73.85 -> 68.39) - retail keeps the
 * folded form there - so that one body sits under a scoped `#pragma peephole on` /
 * `#pragma peephole reset` and stays at 73.85.  No function is worse than before the re-cut.
 *
 * STATUS / RESIDUALS.  Written, 14 bodies (percent under this file's flags):
 *   * 100.00  `menu_row_empty_count`, `place_rec_key_ptr`, `note_slot_draw_row`, `place_rec_alloc`.
 *   *  99.58 `menu_row_reserve`;  97.50 `place_list_key_count`;  91.90 `note_slot_cursor_bounds`;
 *      88.89 `place_rec_free_get`;  87.66 `item_page_option_item_id`;  87.02
 *      `place_list_deactivate_all`;  85.67 `menu_row_clear`.
 *   *  `menu_row_find` 77.85 % - retail duplicates the four-halfword compare block for the kinds
 *      1/2 range and the 3/4/5 range; this source shares one body, so ours is one block shorter.
 *   *  `place_list_overlap_find` 73.85 % - the halving and the four comparisons match; the loop's
 *      record pointer is re-loaded per iteration where retail hoists it.
 *   *  `note_slot_cursor_step` 80.95 % - the slot pair is packed into the HIGH half of
 *      `note_slot_cursor_bounds`'s return value (reproduced); the residual is the
 *      `fn_802DA2D4`/`return 0` tail's register allocation.
 *   *  `menu_row_clear` 85.67 % - retail keeps the scaled index and the zero in separate
 *      registers (`slwi r4,r0,4` + `li r0,0` + `stwx r0,r3,r4`), ours reuses one.
 * NOT written: `fn_8034C350` (8 B), `fn_8034CCBC` (0xB0) and `fn_8034CD6C` (0x70) - the last two
 * read `_SPR_DATA_`'s rectangle and cannot be typed here: `hud/layout.h` (owner of `_SPR_DATA_`)
 * and `menu/menu_item.h` both define `_mh_ivec2_`, so the two headers collide in one TU (the
 * per-consumer-view split `menu/menu_item_page.h` documents); `fn_8034CDDC` (0x348); the
 * nine row fillers `fn_8034D124`..`fn_8034D284`.
 * DATA.  `.sdata2` 0x8079B368..0x8079B3A8 is claimed; no `.data` (the unit owns none).  The
 * `.bss` word `lbl_806AC8C8` the placement functions read is the shared menu work block, unowned.
 */

#include "types.h"
#include "menu/menu_item_page.h"
#include "menu/menu_note.h"
#include "menu/menu_item.h"
#include "fn_8004CAD8.h"
#include "ef/eft_slot.h"
#include "ef/fn_800CDB2C.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/unknown.h"

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
#pragma peephole on
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
#pragma peephole reset

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
/* untyped: a caller-owned 16-byte record copied verbatim into the placement record's body (memcpy-shaped) */
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
