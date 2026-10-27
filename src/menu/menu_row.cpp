/*
 * menu/menu_row.cpp - the menu's item-page row code: the note slots' cursor and draw steps, the 8-row `MenuRowData`
 *   table (`fn_8004EA24`'s; find/clear/empty count/reserve), the `MenuSlot::place_entries` help-record list (find,
 *   allocate, highlight draw) and the nine `PlaceRec` fillers 0x8034D124-0x8034D2B0, each storing its kind 1-9 at +0x00
 *   and its halfwords from +0x04.  C++; every definition is `extern "C"` (the map rows are plain names).
 * RANGE. .text 0x8034C1D0-0x8034D2B0 (27 functions, all written); extab, extabindex, .sdata2 0x8079B368-0x8079B3A8
 *   (the pool `place_list_highlight_draw` loads, emitted by its literals).  The right edge is where the extabindex and
 *   `.sdata2` runs break and the callees turn to `_PLW`.  The left edge is open: `menu/menu_note.cpp`'s
 *   `get_note_item_slot` (0x8034C0C4), which `note_slot_cursor_step` calls first, may be the head of this TU;
 *   `tudiscover.py` reports only weak cuts inside (0x8034CBE4, 0x8034CCBC).
 * FLAGS. `cflags_menu` (configure.py) plus a file-wide `#pragma optimization_level 4`: retail's switches test case
 *   ranges with `subi`+`cmplwi` (`menu_row_find`, `place_list_highlight_draw`; playbook 108), which level 3 lowers to
 *   `cmplwi lo`/`blt` pairs.  `place_list_highlight_draw` sits under `#pragma fp_contract off` (retail keeps
 *   `fmul`+`fadd`, never `fmadd`).
 * NAMES. Module `menu`: no `__FILE__` string and only `zz_` dump names, but the head's callers are menu, lobby-menu and
 *   `hud/cockpit.cpp` units and no enemy unit, and the fillers' callers are `menu/menu_infomation`, `lobby/lb_companion_ui`
 *   and `lobby/lb_quest_ui`.  The file name (the dominant type `MenuRowData`) and every symbol name are GUESSes from the
 *   bodies, in the band's scheme (`get_note_item_slot`, `item_page_option_row_index`).
 *   GUESS (from each body and its callers): `place_rec_alloc_spr`, `place_rec_alloc_idx`, `menu_row_rec_init`,
 *   `place_list_highlight_draw`, `menu_row_monster_index_get`.
 * RESIDUALS. Three rows short, all register allocation with the instruction stream otherwise equal:
 *  - `place_list_overlap_find`: retail keeps `menu_work` in r9 and the record pointer in r7 (ours r8/r7), so the
 *    half-extent temporaries and the loop counter take other registers; no declaration order reaches it;
 *  - `menu_row_clear`: retail keeps the scaled index and the zero in separate registers (`slwi r4,r0,4` + `li r0,0`;
 *    ours `li r4,0` first); five spellings, scheduling off and optimisation levels 1/2 give the same stream;
 *  - `note_slot_cursor_step`: the cursor reload before `+ 1` lands in r3 where retail reuses r4.
 *   `menu_row_monster_index_get` calls `monster_icon_cell_get` through an `s8 (*)(u8)` view: the owner returns `u8`,
 *   and only an `s8` callee lets the 8-byte body tail-call without an `extsb`.
 * SHAPES. `note_slot_cursor_bounds` returns the slot pair packed into the high half of its result.  `menu_row_find` and
 *   the place-list loops walk a pointer (`rows++`/`rec++`), not an index; `menu_row_empty_count` returns `u16` (both
 *   callers narrow its result with `clrlwi 16`).
 */

#include "types.h"
#include "menu/menu_row.h"
#include "menu/menu_row_clear.h"
#include "menu/place_rec_make_set.h"
#include "menu/place_rec_item_set.h"
#include "menu/place_list_highlight_draw.h"
#include "menu/menu_item_page.h"
#include "menu/menu_note.h"
#include "menu/menu_item.h"
#include "fn_8004CAD8.h"
#include "ef/eft_slot.h"
#include "ef/fn_800CDB2C.h"
#include "sound/fn_800D7F54.h"
#include "hud/monster_icon_cell_get.h"
#include "hud/spr_data.h"
#include "lobby/spr_data_copy.h"
#include "mh3_pad/system_w.h"
#include "pad_hold_ck.h"
#include "MSL/mathf.h"

#pragma optimization_level 4

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


extern "C" u32 note_slot_cursor_bounds(u8 kind, u8 index);

/* Rejects the note slot's cursor move, then notifies the option row the player picked. */
extern "C" s32 note_slot_cursor_step(u8 note_entry, u8* cursor, u16 flags, u16* word) {
    u16 sel = flags | (u16)(*word & 0xF);
    u8 slot_tbl = 0;
    u8 slot_idx = 0;
    MenuNoteSlot slot;
    s8 row;

    get_note_item_slot(note_entry, &slot_tbl, &slot_idx);
    row = item_page_option_row_index(slot_tbl, slot_idx, *(s8*)cursor);
    slot.raw = (u16)(note_slot_cursor_bounds(slot_tbl, slot_idx) >> 16);
    if ((sel & 0x10) != 0) {
        sysSE_req(6);
        if ((s8)(*cursor + 1) < slot.count) {
            *cursor += 1;
        } else {
            ainpc_entry_kill_kind(1);
            return 0;
        }
    }
    if (row >= 0) {
        item_page_option_available(row);
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

/* Returns the menu row of monster `monster`, or a negative value when the monster has none. */
extern "C" s8 menu_row_monster_index_get(u8 monster) {
    return ((s8 (*)(u8))monster_icon_cell_get)(monster);
}

/* Finds the row whose kind and key words match `entry` in the 8-row item table. */
extern "C" u32 menu_row_find(const MenuRowData* entry) {
    u32 found = 0;
    MenuRowData* rows = userdata_item_rows_get();

    for (u32 i = 0; i < 8; i++, rows++) {
        u32 kind = rows->kind;

        if (kind != 0 && kind == (u32)entry->kind) {
            switch (kind) {
            case 1:
            case 2:
                if (rows->equip_kind == entry->equip_kind && rows->equip_id == entry->equip_id &&
                    rows->item_id == entry->item_id && rows->field_0x0A == entry->field_0x0A) {
                    found = 1;
                }
                break;
            case 3:
            case 4:
            case 5:
                if (rows->equip_kind == entry->equip_kind && rows->equip_id == entry->equip_id &&
                    rows->item_id == entry->item_id) {
                    found = 1;
                }
                break;
            case 6:
                if (rows->equip_kind == entry->equip_kind && rows->equip_id == entry->equip_id &&
                    rows->item_id == entry->item_id && rows->field_0x0A == entry->field_0x0A &&
                    rows->field_0x0C == entry->field_0x0C) {
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
    MenuRowData* rows = userdata_item_rows_get();

    rows[index].kind = 0;
}

/* Counts the empty rows of the 8-row item table. */
extern "C" u16 menu_row_empty_count(void) {
    MenuRowData* rows = userdata_item_rows_get();
    u8 count = 0;

    for (u32 i = 0; i < 8; i++) {
        if (rows[i].kind == 0) {
            count++;
        }
    }
    return count;
}

/* Lists the caller's row: 1 when it is already listed, 2 when no row is free, else pushes it and returns 0. */
extern "C" u32 menu_row_reserve(MenuRowData* entry) {
    if (menu_row_find(entry) != 0) {
        return 1;
    }
    if (menu_row_empty_count() == 0) {
        return 2;
    }
    userdata_item_row_push(entry);
    return 0;
}

/* Builds a note slot's cursor bounds from the item page's option table for its kind. */
extern "C" u32 note_slot_cursor_bounds(u8 kind, u8 index) {
    u16* table = 0;
    MenuNoteSlot slot;
    u8 remain;
    s32 i;

    slot.table = 0;
    slot.count = 0;
    switch (kind) {
    case 0:
        table = (u16*)menu_row_limit_get(0);
        break;
    case 1:
        table = (u16*)menu_row_limit_get(1);
        break;
    case 2:
        table = (u16*)menu_row_limit_get(2);
        break;
    case 3:
        table = (u16*)menu_row_limit_get(3);
        break;
    }
    if (table == 0) {
        return (u32)slot.raw << 16;
    }
    note_slot_pair_copy(&slot.raw, table + index);
    remain = slot.count;
    for (i = 0; i < slot.count; i++) {
        if (slot.table != 0xFF) {
            if (item_page_option_item_id((u8)(kind + 1), (u8)(slot.table + i)) == -1) {
                remain--;
            }
        } else {
            s8 out;

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
                value = userdata_progress_flag_ck(14);
            } else {
                value = 1;
            }
            break;
        case 11:
        case 33:
            if (game_ready_ck() == 0) {
                value = userdata_progress_flag_ck(15);
            } else {
                value = 1;
            }
            break;
        case 42:
            if (game_ready_ck() == 0) {
                value = userdata_flag_ck(2004);
            } else {
                value = userdata_flag_ck(14001);
            }
            break;
        case 50:
            value = userdata_progress_flag_ck(17);
            break;
        case 74:
            if (game_ready_ck() == 0) {
                value = -1;
            } else {
                value = userdata_progress_flag_ck(16);
            }
            break;
        case 104:
            if (game_ready_ck() == 0) {
                value = userdata_progress_flag_ck(25);
            } else {
                value = -1;
            }
            break;
        case 105:
            if (game_ready_ck() == 0) {
                value = userdata_progress_flag_ck(26);
            } else {
                value = -1;
            }
            break;
        case 106:
            if (game_ready_ck() == 0) {
                value = userdata_progress_flag_ck(28);
            } else {
                value = -1;
            }
            break;
        case 107:
            if (game_ready_ck() == 0) {
                value = userdata_progress_flag_ck(28);
            } else {
                value = -1;
            }
            break;
        case 108:
        case 109:
        case 110:
        case 111:
            if (game_ready_ck() == 0) {
                value = userdata_progress_flag_ck(31);
            } else {
                value = -1;
            }
            break;
        }
    } else if (kind == 1 && index == 12) {
        if (game_ready_ck() == 0) {
            value = -1;
        } else {
            value = 1;
        }
    }
    return value;
}

/* Deactivates every record of the `.bss` placement list. */
extern "C" void place_list_deactivate_all(void) {
    s32 i;
    s32 count;
    MenuPlaceRec* rec = menu_work.slot[0].place_entries;

    if (rec == 0) {
        return;
    }
    count = menu_work.slot[0].place_count;
    for (i = 0; i < count; i++, rec++) {
        rec->active = 0;
    }
}

/* Returns the index of the first active record whose rectangle overlaps the given box. */
extern "C" s32 place_list_overlap_find(s16 x, s16 y, s16 width, s16 height) {
    s16 left;
    s16 right;
    MenuPlaceRec* rec = menu_work.slot[0].place_entries;
    s16 top;
    s16 bottom;

    if (rec == 0) {
        return -1;
    }
    left = (s16)(x - width / 2);
    right = (s16)(x + width / 2);
    top = (s16)(y - height / 2);
    bottom = (s16)(y + height / 2);
    for (s32 i = 0; i < menu_work.slot[0].place_count; i++, rec++) {
        if (rec->active != 0) {
            s16 rec_x2 = (s16)(rec->pos_x + rec->width);
            s16 rec_y2 = (s16)(rec->pos_y + rec->height);
            s16 rec_x = rec->pos_x;
            s16 rec_y = rec->pos_y;

            if (rec_x <= right && rec_x2 >= left && rec_y <= bottom && rec_y2 >= top) {
                return (s8)i;
            }
        }
    }
    return -1;
}

/* Returns the key word of the placement record with the given index. */
extern "C" u32* place_rec_key_ptr(s8 index) {
    if (menu_work.slot[0].place_entries == 0) {
        return 0;
    }
    return &menu_work.slot[0].place_entries[index].key;
}

/* Counts the active placement records carrying the given key. */
extern "C" u8 place_list_key_count(u32 key) {
    MenuPlaceRec* rec;
    u8 count;
    s32 i;

    count = 0;
    rec = menu_work.slot[0].place_entries;

    if (rec == 0) {
        return 0;
    }
    for (i = 0; i < menu_work.slot[0].place_count; i++, rec++) {
        if (rec->active != 0 && rec->key == key) {
            count++;
        }
    }
    return count;
}

/* Returns the first free placement record. */
extern "C" MenuPlaceRec* place_rec_free_get(void) {
    MenuPlaceRec* rec = menu_work.slot[0].place_entries;
    s32 count;

    if (rec == 0) {
        return 0;
    }
    count = menu_work.slot[0].place_count;
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
extern "C" u8 place_rec_alloc(const PlaceRec* src, s16 x, s16 y, s16 width, s16 height, s16 a, s16 b) {
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
    place_rec_copy(&rec->key, src);
    return rec->index;
}

/* Allocates a placement record for the box of sprite record `spr`, moved by `pos` when one is given. */
extern "C" u8 place_rec_alloc_spr(const PlaceRec* src, const _SPR_DATA_* spr, const _mh_ivec2_* pos, s16 a, s16 b) {
    _mh_ivec2_ at;

    uv_pair_copy(&at, spr);
    if (pos != 0) {
        at.x += pos->x;
        at.y += pos->y;
    }
    at.x += spr->ofs_x;
    at.y += spr->ofs_y;
    return place_rec_alloc(src, at.x, at.y, spr->width, spr->height, a, b);
}

/* Allocates a placement record for the box of layout sprite `id` at `pos`. */
extern "C" void place_rec_alloc_idx(const PlaceRec* src, u16 id, const _mh_ivec2_* pos, s16 a, s16 b) {
    place_rec_alloc_spr(src, (const _SPR_DATA_*)get_lsp_data(id, 0), pos, a, b);
}

#pragma fp_contract off
/* Draws a pulsing marker over every active help record, enlarging the one at `*index`. */
extern "C" void place_list_highlight_draw(u8 mode, const u32* index) {
    s32 sel = *index;
    MenuPlaceRec* rec = menu_work.slot[0].place_entries;
    s32 i;
    s32 count;

    if (rec == 0) {
        return;
    }
    count = menu_work.slot[0].place_count;
    for (i = 0; i < count; i++, rec++) {
        if (rec->active != 0) {
            u16 id = 0;
            s16 dy = 0;

            switch (rec->key) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                id = 0x466;
                dy = 3;
                break;
            case 7:
            case 8:
            case 9:
                id = 0x465;
                break;
            default:
                break;
            }
            if (id != 0 && count != 0) {
                _mh_ivec2_ pos;
                _SPR_DATA_ spr;
                f32 wave;

                pos.x = (s16)(rec->pos_x + rec->field_0x10);
                pos.y = (s16)(rec->pos_y + rec->field_0x12 + dy);
                spr_data_copy(&spr, (const _SPR_DATA_*)get_lsp_data(id, 0));
                if (pad_hold_ck() == 0 && system_w.field_0x05 == 0) {
                    wave = (f32)((system_w.field_0x0c * 16 + i * (360 / count)) % 360);
                } else {
                    wave = (f32)((i * (360 / count)) % 360);
                }
                wave = sinf(6.2831855f * wave / 360.0f);
                if (sel == i) {
                    spr.v_scale = (u16)(spr.v_scale + 100.0 * (0.75 + 0.2 * wave));
                    spr.u_scale = (u16)(spr.u_scale + 100.0 * (0.5 + 0.2 * wave));
                    spr.color_0x1C = 0xE05F1BFF;
                } else if (sel == 255) {
                    spr.v_scale = (u16)(spr.v_scale + 100.0 * (0.25 * wave));
                }
                draw_sprite(spr, &pos);
            }
        }
    }
}
#pragma fp_contract reset

/* Fills help record `rec` as kind 1, a make recipe. */
extern "C" s32 place_rec_make_set(PlaceRec* rec, u16 a, u16 b, u16 item, u16 count) {
    rec->kind = 1;
    rec->value_0x04 = a;
    rec->value_0x06 = b;
    rec->value_0x08 = item;
    rec->value_0x0A = count;
    rec->value_0x0C = 0;
    rec->value_0x0E = 0;
    return 1;
}

/* Fills help record `rec` as kind 2, an upgrade recipe. */
extern "C" s32 place_rec_upgrade_set(PlaceRec* rec, u16 a, u16 b, u16 item, u16 count) {
    rec->kind = 2;
    rec->value_0x04 = a;
    rec->value_0x06 = b;
    rec->value_0x08 = item;
    rec->value_0x0A = count;
    rec->value_0x0C = 0;
    rec->value_0x0E = 0;
    return 2;
}

/* Fills help record `rec` as kind 3, a material. */
extern "C" s32 place_rec_material_set(PlaceRec* rec, u16 a, u16 item, u16 count) {
    rec->kind = 3;
    rec->value_0x04 = a;
    rec->value_0x06 = item;
    rec->value_0x08 = count;
    rec->value_0x0A = 0;
    rec->value_0x0C = 0;
    rec->value_0x0E = 0;
    return 3;
}

/* Fills help record `rec` as kind 4, an item cost. */
extern "C" s32 place_rec_item_set(PlaceRec* rec, u16 source, u16 item, u16 count) {
    rec->kind = 4;
    rec->value_0x04 = source;
    rec->value_0x06 = item;
    rec->value_0x08 = count;
    rec->value_0x0A = 0;
    rec->value_0x0C = 0;
    rec->value_0x0E = 0;
    return 4;
}

/* Fills help record `rec` as kind 5, a zenny cost. */
extern "C" s32 place_rec_zenny_set(PlaceRec* rec, u16 source, u16 item, u16 amount) {
    rec->kind = 5;
    rec->value_0x04 = source;
    rec->value_0x06 = item;
    rec->value_0x08 = amount;
    rec->value_0x0A = 0;
    rec->value_0x0C = 0;
    rec->value_0x0E = 0;
    return 5;
}

/* Fills help record `rec` as kind 6, a trade. */
extern "C" s32 place_rec_trade_set(PlaceRec* rec, u16 source, u16 item, u16 count, u16 cost, u16 cost_count) {
    rec->kind = 6;
    rec->value_0x04 = source;
    rec->value_0x06 = item;
    rec->value_0x08 = count;
    rec->value_0x0A = cost;
    rec->value_0x0C = cost_count;
    rec->value_0x0E = 0;
    return 6;
}

/* Fills help record `rec` as kind 7, a skill tree. */
extern "C" s32 menu_row_rec_init(PlaceRec* rec, u16 tree) {
    rec->kind = 7;
    rec->value_0x04 = tree;
    rec->value_0x06 = 0;
    rec->value_0x08 = 0;
    rec->value_0x0A = 0;
    rec->value_0x0C = 0;
    rec->value_0x0E = 0;
    return 7;
}

/* Fills help record `rec` as kind 8, a skill. */
extern "C" s32 place_rec_skill_set(PlaceRec* rec, u16 skill) {
    rec->kind = 8;
    rec->value_0x04 = skill;
    rec->value_0x06 = 0;
    rec->value_0x08 = 0;
    rec->value_0x0A = 0;
    rec->value_0x0C = 0;
    rec->value_0x0E = 0;
    return 8;
}

/* Fills help record `rec` as kind 9, a course. */
extern "C" s32 place_rec_course_set(PlaceRec* rec, u16 course) {
    rec->kind = 9;
    rec->value_0x04 = course;
    rec->value_0x06 = 0;
    rec->value_0x08 = 0;
    rec->value_0x0A = 0;
    rec->value_0x0C = 0;
    rec->value_0x0E = 0;
    return 9;
}
