/*
 * menu/menu_item.cpp - the item menu: the item-name and item-data helpers, the menu work (`menu_work`: the single-screen
 *   and split-screen menu slots, their pad copy, top menu, sub-lists, page dispatchers and draw tasks) and the item page's
 *   action lists.  C++; the map's plain names are `extern "C"`.
 * RANGE. .text 0x8029F61C-0x802A6624 (86 functions); extab 0x80013624-0x800137D4, extabindex 0x80030FB4-0x8003123C,
 *   .data 0x805CDE78-0x805CE00C, .bss 0x806AC8B8-0x806AD698, .sdata 0x80792298-0x807922A0, .sdata2 0x8079A3F0-0x8079A3F8.
 *   The left edge: the item table head `.bss` 0x806AC8B8 is read from `get_item_data_ptr` (0x8029F61C) on, the hit
 *   registry before it only up to 0x8029F60C (`Pl/pl_hit.cpp`, the tail of the Pl hit/land band).
 * FLAGS. `cflags_menu` (configure.py: the lib's `-opt nopeephole`).  The range keeps `bl`s to its tiny same-file helpers
 *   (`GetItemData` from `item_rare_color_get`/`item_category_ck`): `-inline noauto`.  File-wide `#pragma optimization_level 4`
 *   (idea 108: `menu_split_item_page_draw` +9 B, nothing lower).
 * NAMES. Module and file from the `__FILE__` string "menu_item.cpp" (.data 0x805CDFC8), passed by every
 *   `nw4r::db::Panic` of the range.  The runtime dump's real names: `get_item_data_ptr`, `ItemName`, `ItemExp`,
 *   `GetItemData`, `get_menu_tbl_ptr`, `get_menu_lsp_tbl`, `put_menu_cursor`.  Every other named row is a GUESS from its
 *   body and callers: `get_item_species_data`, `get_menu_msg_tbl`,
 *   `get_menu_msg`, `menu_row_table_get`, `menu_row_limit_get`, `menu_slot_entry_b_select`, `menu_slot0_lock_set`,
 *   `menu_slot_lock_set`, `menu_item_close`, `menu_work_update`, `menu_slot_anim_update`, `menu_slot_panel_draw`,
 *   `item_species_name_get`, `item_rare_color_get`, `item_is_53_ck`, `item_kind_color_by_kind` and every `menu_*` row
 *   from 0x8029F834 to 0x802A64B0 (`menu_main_*` the single-screen menu, `menu_split_*` the split-screen one,
 *   `menu_quit_*` the return confirmation (page 5), `menu_lobby_exit_*` the lobby save-and-exit (page 6),
 *   `menu_item_*` the item page (page 0) and its use/move/drop actions).  The page callees' registry names
 *   (`menu_equip_panel_*`, `menu_page02_*`, `movie_page04_*` ... `note_page_*`) are GUESSes keyed by the page kind
 *   `MenuSlot::field_0x014` and the owner unit.  Data: `item_data_head`, `menu_tables` (GUESSes: what
 *   `get_item_data_ptr`/`get_menu_tbl_ptr` return) and `item_count_arrow_rows` (GUESS: the rows
 *   `menu_item_count_arrow_draw` hands `put_page_count`).
 * RESIDUALS. Every row is written; the partial rows:
 *  - `menu_item_frame_update`: retail masks the slot index before `cockpit_msg_slot_busy_ck` (`clrlwi r3,r30,24`),
 *    ours moves it (`mr r3,r30`); `u8`/`u16`/`s32`/`u32` index types and a cast all give the `mr`;
 *  - `menu_slot_panel_draw`, `menu_split_panel_draw`: retail loads `li r3,<801|4073>` after the count compare, ours
 *    before the count load; `if/else`, `?:` and `>=` spellings fold to a branch-free `subfc/adde` (-15 B), and
 *    `#pragma scheduling off` or `optimization_level 3` change nothing - it is the code generator's order;
 *  - `put_menu_cursor`: the colour byte and the blink scale are multiplied in the other operand order (`mullw r0,r4,r0`);
 *  - `menu_lobby_exit_draw`: `title` takes r31 where retail shares r28 with `selected` (declaration orders and
 *    `selected`/`title` types tried);
 *  - `menu_split_item_page_draw`: `get_menu_msg(16, confirm)` - retail copies `confirm` (`mr r4,r24`) before the
 *    `clrlwi`, ours masks it in place (-4 B); `(u16)`/`(s16)`/`(u32)` casts give ours;
 *  - `menu_main_draw`: the loop counters and the hoisted colour constants take other registers (r21/r23/r24/r27);
 *    declaration orders tried;
 *  - data: the `__FILE__` string sits at +0x150 in retail's `.data` behind a 4-byte gap (0x805CDFC4), ours at
 *    +0x14C; the `.sdata`/`.sdata2` tails are one/four bytes short (alignment padding).
 * SHAPES. `MenuSlot` (`menu/menu_item.h`) is the one view of the menu slot at `.bss` 0x806AC8C8.
 *  - `MenuSlot::entry_count_a` is read as `s8` wherever it is an argument (`(s8&)` - an `(s8)` cast loads through
 *    r0); the field stays `u8` because `menu/menu_item_page.cpp` passes it to a `u16` parameter;
 *  - an `s8` field read without a cast loads straight into the argument register (`lbz r4` + `extsb r4,r4`); a cast
 *    `u8` field goes through r0, and an `s8` multiply keeps retail's `mullw` operand order;
 *  - a row-sprite table walked by index (`rec = get_lsp_data(ids[i], 0)`) sets the counter before the table pointer
 *    (idea 111);
 *  - `if (a) x = p; else x = q;` keeps retail's register pair where `x = q; if (a) x = p;` swaps it;
 *  - a sprite record copied by initialisation (`_SPR_DATA_ spr = *get_lsp_data(...)`) is copied word by word; an
 *    assignment copies it member by member;
 *  - `menu_work_update`'s quest case falls through into `default: return;` (idea 232);
 *  - `ItemDataRecord`'s +0x000 is a union of both views (`kind_0x00`/`level_0x01` beside one `u16`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"   /* PlayMode_ck (rule 2: the owner is `ef/system_core.cpp`) */
#include "id_value.h"
#include "menu/menu_item.h"
#include "quest/quest_item_slot.h"   /* quest_select_ready_ck (rule 2) */
#include "Pl/pl_act.h"
#include "unsplit/unknown.h"
#include "unsplit/ef.h"
#include "hud/layout.h"
#include "menu/menu_message.h"
#include "menu/get_pop_dat_ptr.h"
#include "enemy/em020_ai.h"
#include "fn_80047398/lobby_world_block.h"
#include "Pl/fn_80273B14.h"               /* `Pl_item_id_usable_ck`, the id-usable predicate (rule 2) */
#include "Pl/fn_8027D684.h"
#include "font/flfnt.h"
#include "sound/fn_800D7F54.h"
#include "fn_80056F24.h"
#include "Pl/pl_now_area_ck.h"
#include "fn_80047398.h"
#include "fn_8004CAD8.h"
#include "mh3_pad/Psw.h"
#include "mh3_pad/system_w.h"
#include "hud/cockpit_msg_slot_clear.h"
#include "hud/ainpc_page_state_init.h"
#include "menu/menu_effect_slot_open.h"
#include "menu/menu_slot_worker_busy_ck.h"
#include "player_pad_channel_get.h"
#include "hud/put_button_guide_b.h"
#include "hud/put_button_guide_c.h"
#include "menu/item_page_draw.h"
#include "menu/equip_info_update.h"
#include "ef/sys_quest_move.h"
#include "menu/equip_panel_slot_reset.h"
#include "menu/menu_equip_panel_step.h"
#include "menu/menu_equip_panel_draw.h"
#include "menu/menu_page02_open.h"
#include "menu/menu_page02_step.h"
#include "menu/menu_page02_draw.h"
#include "menu/menu_effect_slot_step.h"
#include "menu/movie_page04_open.h"
#include "menu/movie_page04_step.h"
#include "menu/movie_page04_draw.h"
#include "ef/menu_status_page_open.h"
#include "ef/menu_status_page_step.h"
#include "menu/equip_detail_screen_reset.h"
#include "menu/equip_detail_screen_step.h"
#include "menu/equip_detail_screen_draw.h"
#include "menu/menu_item_sub_page09_open.h"
#include "menu/menu_item_sub_page09_step.h"
#include "menu/menu_item_sub_page09_draw.h"
#include "menu/menu_item_sub_page10_open.h"
#include "menu/menu_item_sub_page10_step.h"
#include "menu/menu_item_sub_page10_draw.h"
#include "menu/plsearch_page11_open.h"
#include "menu/plsearch_page11_step.h"
#include "menu/plsearch_page11_draw.h"
#include "menu/placeinfo_page12_open.h"
#include "menu/placeinfo_page12_step.h"
#include "menu/placeinfo_page12_draw.h"
#include "menu/placeinfo_page13_open.h"
#include "menu/placeinfo_page13_step.h"
#include "menu/placeinfo_page13_draw.h"
#include "menu/menu_item_sub_page14_open.h"
#include "menu/menu_item_sub_page14_step.h"
#include "menu/menu_item_sub_page14_draw.h"
#include "menu/menu_item_sub_page15_open.h"
#include "menu/menu_item_sub_page15_step.h"
#include "menu/menu_item_sub_page15_draw.h"
#include "menu/menu_item_sub_page16_open.h"
#include "menu/menu_item_sub_page16_step.h"
#include "menu/menu_item_sub_page16_draw.h"
#include "menu/movie_page17_open.h"
#include "menu/movie_page17_step.h"
#include "menu/movie_page17_draw.h"
#include "menu/movie_page18_open.h"
#include "menu/movie_page18_step.h"
#include "menu/movie_page18_draw.h"
#include "menu/plsearch_page19_open.h"
#include "menu/plsearch_page19_step.h"
#include "menu/plsearch_page19_draw.h"
#include "menu/plsearch_page22_open.h"
#include "menu/plsearch_page22_step.h"
#include "menu/plsearch_page22_draw.h"
#include "enemy/note_page_open.h"
#include "enemy/note_page_step.h"
#include "enemy/note_page_draw.h"
#include "enemy/note_pen_seed.h"
#include "enemy/note_pane_reset.h"
#include "hud/ainpc_page_pos_set.h"
#include "lobby/event_demo_running_ck.h"
#include "fn_80040598.h"
#include "enemy/lobby_state_block.h"
#include "Network/net_session_close.h"
#include "hud/cockpit_msg_slot_draw.h"
#include "system_menu_callback_call.h"
#include "hud/cockpit_blink_get.h"
#include "font/font_print_plain.h"
#include "hud/cockpit_msg_slot_update.h"
#include "hud/cockpit_msg_slot_busy_ck.h"
#include "hud/cockpit_msg_slot_split_draw.h"
#include "enemy/quest_info_page_open.h"
#include "enemy/quest_info_page_step.h"
#include "menu/movie_page04_split_open.h"
#include "menu/movie_page04_split_step.h"
#include "menu/menu_infomation.h"
#include "pad_hold_ck.h"
#include "lobby/lb_quest_work_active_ck.h"
#include "hud/draw_lsp_sprite_variant.h"
#include "sound/set_zmode__FbUcb.h"
#include "hud/put_button_guide.h"
#include "hud/draw_window_box.h"
#include "menu/menu_frame_page_draw.h"
#include "menu/menu_frame_entries_build_row.h"
#include "enemy/quest_info_page_draw.h"
#include "menu/movie_page04_split_draw.h"
#include "menu/Set_equip_column_arrangement.h"
#include "menu/Put_status_equip_status_block.h"
#include "menu/Put_status_equip_element_block.h"
#include "menu/Put_status_invork_skill_sep.h"
#include "hud/draw_layout_window.h"
#include "menu/put_message_sys.h"
#include "menu/menu_plsearch.h"
#include "Network/network_pat_control.h"
#include "lobby/lobby_w.h"
#include "hud/cockpit.h"
#include "Pl/pl_item_add.h"
#include "hud/cockpit_work.h"
#include "hud/quest_mark_toggle.h"
#include "hud/quest_mark_flag_get.h"
#include "hud/quest_slot_arm_all_a.h"
#include "lobby/lb_entry_selected_send.h"

#pragma optimization_level 4
/* ---- the item menu: the item-record helpers, the option and action lists and their draw walk (0x8029F61C-0x802A6624) ---- */

/* nw4r's debug panic: the map name `Panic__Q24nw4r2dbFPCciPCce` is the front-end's spelling of this
 * declaration, so it sits at C++ scope (rule 9). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* ------------------------------------------------------------------------------------------------
 * the unit's own data
 * ------------------------------------------------------------------------------------------------ */

/* The item table head (`.bss` 0x806AC8B8), the menu work (0x806AC8C8) and the menu table set (0x806ACF28). */
ItemDataHead item_data_head;
MenuWork menu_work;
MenuTables menu_tables;

/* The colour of each item kind (`ItemDataRecord::kind`), RGBA. `.data` 0x805CDE78 (0x30 B). */
u32 item_kind_color_tbl[12] = {
    0xF5F5F5FF, 0xE1505CFF, 0x69BF83FF, 0x9FBBFFFF, 0xF7CF65FF, 0xC3A3D1FF,
    0x9BDFF0FF, 0xF79A65FF, 0xF8889DFF, 0xADD56DFF, 0xAAAAAAFF, 0x00000000,
};

/* One 0x130-byte record of the option table `lobby_hunter_cards` (`.bss`): +0x00 says the row is filled,
 * +0x03 is its label and +0x0D its value text.  Only those three fields are named.
 * size: 0x130 */
typedef struct MENU_OPTION_REC {
    /* +0x000 */ u8 valid_0x00;
    /* +0x001 */ u8 unused_0x01[0x02];
    /* +0x003 */ s8 label_0x03[0x0A];
    /* +0x00D */ s8 value_0x0D[0x123];
} MENU_OPTION_REC;

/* One 0xB20-byte record of the move-work array `get_move_work_adrs(2)` returns: the action list reads
 * the item name at +0x5CA and its value text at +0x5DB. size: 0xB20 */
typedef struct MENU_MOVE_WORK {
    /* +0x000 */ u8 unused_0x000[0x5CA];
    /* +0x5CA */ s8 name_0x5CA[0x11];
    /* +0x5DB */ s8 value_0x5DB[0x545];
} MENU_MOVE_WORK;

/* The option-list records `menu_list_fill`/`menu_list_names_set` and the draw walk: 0x130-byte records whose
 * +0x00 byte says the row is filled, +0x03 the label and +0x0D its value text.  `.bss` 0x806BE340. */
extern u8 lobby_hunter_cards[];

/* The folded half's callees (map stems, so C linkage - the front-end emits the map's
 * plain names) plus the two it hands a *pointer* view of the record to.  This unit's own
 * symbols among them - `menu_page_open_ck`, `menu_slot_entry_b_select`, `menu_item_grid_rows_build`, `get_menu_msg_tbl`,
 * `get_menu_msg`, `get_menu_lsp_tbl`, `GetItemData` - are declared once, in
 * `menu/menu_item.h`, and the call sites below follow those declarations (see
 * VIEWS in the file header for the two that cross the two views of the record). */
extern "C" {

void menu_item_action_list_build(MenuSlot* self, s16 index);
s32 menu_item_move_step(MenuSlot* self);
s32 menu_item_drop_step(MenuSlot* self);
void menu_list_count_update(MenuSlot* self);
u8 menu_list_mode_get(MenuSlot* self);
u8 menu_list_fill(MenuSlot* self, s8 filter);
s32 menu_list_names_set(MenuSlot* self, u8 index);
void menu_cursor_column_step(void* key);
void menu_frame_entries_build(void* page_a, void* page_b, s8 rows, s32 id, s32 flag);
void menu_frame_draw_page(void* page, void* slots, s8 cols, s8 rows, u16 timer, u32 flags);
void menu_item_pages_resort(_PLW* plw, void* page_a, void* page_b);
s32 menu_slot_index_is_page_b(MenuSlot* self, s16 index);
u32 menu_item_slot_accepts(MenuSlot* self, u16 item_id, s16 index);
IdValue* menu_slot_get(MenuSlot* self, s16 index);
void menu_select_window_draw(u8 value, u16 kind, s8* text, const _mh_ivec2_* pos, s32 flag);
void menu_choice_window_draw(void* panel, s32 index, const _mh_ivec2_* pos);
void draw_font_align(_SPR_DATA_* spr, s8* text, u32 length, const _mh_ivec2_* pos);
void draw_window_frame_preset(s16 x, s16 y, s32 flag);
void put_page_count(u16* rows, s16 a, s16 b, u16 c, const _mh_ivec2_* pos);
s16 ai_npc_item_stock_get(u16 item_id);
char* strcpy(char* dst, const char* src);
s32 menu_item_worker_ck(_PLW* plw);
u32 menu_item_move_target_set(MenuSlot* self, s16 index);
s32 menu_item_use_step(MenuSlot* self);
s32 menu_item_split_use_step(MenuSlot* self);
s32 menu_item_page_step(MenuSlot* self);
void menu_item_page_draw(MenuSlot* self);
u32 menu_item_use_ck(MenuSlot* self);
void menu_item_count_arrow_draw(MenuSlot* self, const _mh_ivec2_* pos);
void menu_top_reset(MenuSlot* slot);

}  // extern "C"

/* 0x8029F61C: the item table head, the block every accessor below reads. */
ItemDataHead* get_item_data_ptr(void)
{
    return &item_data_head;
}

/* 0x8029F628: the display name id of an item; 0 when the id is out of the 747-entry table. */
u32 ItemName(u16 id)
{
    u32* names = item_data_head.names;
    if (id >= 747) {
        id = 0;
    }
    return names[id];
}

/* 0x8029F654: the experience value of an item; 0 when the id is out of the table. */
u32 ItemExp(u16 id)
{
    u32* exp = item_data_head.exp;
    if (id >= 747) {
        id = 0;
    }
    return exp[id];
}

/* 0x8029F680: reports an item's species kind to the caller, through the species table's +0x08 byte. */
extern "C" void item_species_name_get(u16 id)
{
    item_kind_name_get(get_item_species_data(GetItemData(id)->species)->kind);
}

/* 0x8029F6B4: the species record an item's +0x00A id selects; entry 0 when out of the 132-entry table. */
ItemSpeciesRecord* get_item_species_data(u16 id)
{
    if (id >= 132) {
        id = 0;
    }
    return &lbl_805DBFB8[id];
}

/* 0x8029F6DC: the 0x14-byte record of an item; entry 0 when the id is out of the table. */
ItemDataRecord* GetItemData(u16 id)
{
    ItemDataRecord* items = item_data_head.items;
    if (id >= 747) {
        id = 0;
    }
    return &items[id];
}

/* 0x8029F704: the colour the item's +0x05 kind selects. */
extern "C" u32 item_rare_color_get(u16 id)
{
    return item_kind_color_tbl[GetItemData(id)->kind];
}

/* 0x8029F73C: the item record's +0x02 byte masked by the caller's value. */
extern "C" s32 item_category_ck(u16 id, s32 index)
{
    ItemDataRecord* rec = GetItemData(id);
    return rec->field_0x002 & index;
}

/* 0x8029F774: whether the id is the menu's own item. */
extern "C" u32 item_is_53_ck(u16 id)
{
    return id == 53;
}

/* 0x8029F788: the colour an item kind selects, without an item record. */
extern "C" u32 item_kind_color_by_kind(u16 kind)
{
    return item_kind_color_tbl[kind];
}

/* 0x8029F7A0: the menu table set the three accessors below index. */
MenuTables* get_menu_tbl_ptr(void)
{
    return &menu_tables;
}

/* 0x8029F7AC: one entry of the menu table's leading pointer run. */
u32* get_menu_lsp_tbl(u16 idx)
{
    return menu_tables.lsp_tbl[idx];
}

/* 0x8029F7C4: the second table's entry for a menu index. */
extern "C" u32 get_menu_msg_tbl(u16 idx)
{
    return (u32)menu_tables.tab_0x66C[idx];
}

/* 0x8029F7E0: an entry of the table the second run's entry points at. */
extern "C" u32 get_menu_msg(u16 idx, u16 sub)
{
    return (u32)menu_tables.tab_0x66C[idx][sub];
}

/* 0x8029F808: the menu table's +0x754 word. */
extern "C" u32 menu_row_table_get(void)
{
    return menu_tables.field_0x754;
}

/* 0x8029F818: the menu table's +0x758 run, indexed. */
extern "C" u32 menu_row_limit_get(u16 idx)
{
    return menu_tables.array_0x758[idx];
}

/* ------------------------------------------------------------------------------------------------ *
 * The menu work's open/close, pad and entry-list builders.
 * ------------------------------------------------------------------------------------------------ */

/* 0x8029F834 (0x240): opens the first menu slot for the local player once no fade is running; `sound` 1 plays the
 * open sound. */
extern "C" void menu_main_open(u32 sound)
{
    MenuSlot* slot = &menu_work.slot[0];
    _PLW* plw;
    u8 mode;

    mode = GameMode_ck();
    slot->field_0x00F = mode;
    if ((u32)(mode - 1) > 1) {
        return;
    }
    if (get_fade_stat(0) == 1 || get_fade_stat(0) == 2) {
        return;
    }
    if (get_fade_stat(1) == 1 || get_fade_stat(1) == 2) {
        return;
    }
    if (get_fade_stat(2) == 1 || get_fade_stat(2) == 2) {
        return;
    }
    if (get_fade_stat(3) == 1 || get_fade_stat(3) == 2) {
        return;
    }
    plw = (_PLW*)get_move_work_adrs(2);
    if (plw == 0) {
        return;
    }
    plw += (s8)my_player_no();
    slot->worker = plw;
    if (slot->field_0x00F == 2 && game_ready_ck() == 1 && (plw->field_0xB01 & 4) != 0) {
        return;
    }
    if (pl_item_page2_ck(plw) == 1) {
        slot->field_0x010 = 1;
    } else {
        slot->field_0x010 = 0;
    }
    if (slot->field_0x00F == 2) {
        IdValue* page = (IdValue*)userdata_equip_item_slots_get(lobby_world_block);

        slot->page_a = page;
        if (slot->field_0x010 == 0) {
            slot->page_b = 0;
        } else {
            slot->page_b = page + 24;
        }
    } else {
        IdValue* page = (IdValue*)plw->slot_id;

        slot->page_a = page;
        if (slot->field_0x010 == 0) {
            slot->page_b = 0;
        } else {
            slot->page_b = page + 26;
        }
    }
    if ((u32)pl_act_menu_lock_ck(plw) == 1) {
        return;
    }
    slot->active = 1;
    slot->field_0x011 = 0;
    slot->field_0x000 = 0;
    slot->field_0x001 = 0;
    slot->field_0x002 = 0;
    slot->field_0x01A = 0;
    slot->page_index = 0;
    slot->field_0x015 = 0;
    slot->field_0x23A = 4;
    menu_top_reset(slot);
    slot->open_wait = 10;
    slot->field_0x021 = 0;
    slot->field_0x31E = 0;
    if (game_ready_ck() == 1) {
        slot->field_0x31F = get_option_cfg(27);
    }
    if (sound == 1) {
        sysSE_req(5);
    }
}

/* 0x8029FA74: resets a slot's first entry array and both of its selection lists to the state the
 * caller's two stored bytes name. */
extern "C" void menu_top_reset(MenuSlot* slot)
{
    memset(slot->entries_a, 0, sizeof(slot->entries_a));
    if (game_ready_ck() != 0) {
        slot->entry_count_a = 3;
        slot->entries_a[2].field_0x02 = 1;
    } else {
        slot->entry_count_a = 2;
    }
    slot->entries_a[0].field_0x02 = 1;
    slot->entries_a[1].field_0x02 = 1;
    menu_tab_select(slot, (s8)slot->field_0x019);
    menu_slot_entry_b_select(slot, (s8)slot->field_0x01B);
}

/* 0x8029FB00: builds a slot's third entry array from the menu table the slot's state selects. */
extern "C" void menu_sub_rows_build(MenuSlot* slot)
{
    u32* table = 0;
    MenuEntry* entry;
    s32 i;

    memset(slot->entries_c, 0, sizeof(slot->entries_c));
    switch ((s8)slot->field_0x019) {
    case 0:
        if ((s8)slot->field_0x01B != 5) {
            break;
        }
        slot->entry_count_c = 3;
        table = (u32*)get_menu_msg_tbl(7);
        slot->entries_c[0].field_0x02 = 1;
        slot->entries_c[0].page_kind = 6;
        slot->entries_c[1].field_0x02 = 1;
        slot->entries_c[1].page_kind = 6;
        slot->entries_c[2].field_0x02 = 1;
        slot->entries_c[2].page_kind = 6;
        break;
    case 2:
        switch ((s8)slot->field_0x01B) {
        case 2:
            slot->entry_count_c = 3;
            table = (u32*)get_menu_msg_tbl(8);
            slot->entries_c[0].field_0x02 = 1;
            slot->entries_c[0].page_kind = 22;
            slot->entries_c[1].field_0x02 = 1;
            slot->entries_c[1].page_kind = 23;
            slot->entries_c[2].field_0x02 = 1;
            slot->entries_c[2].page_kind = 24;
            break;
        case 4:
            slot->entry_count_c = 3;
            table = (u32*)get_menu_msg_tbl(9);
            slot->entries_c[0].field_0x02 = 1;
            slot->entries_c[0].page_kind = 19;
            slot->entries_c[1].field_0x02 = 1;
            slot->entries_c[1].page_kind = 20;
            slot->entries_c[2].field_0x02 = 1;
            slot->entries_c[2].page_kind = 21;
            break;
        case 5:
            slot->entry_count_c = 4;
            table = (u32*)get_menu_msg_tbl(10);
            if (slot->field_0x00F == 2) {
                slot->entries_c[0].field_0x02 = 1;
                slot->entries_c[1].field_0x02 = 1;
                slot->entries_c[2].field_0x02 = 1;
            } else {
                slot->entries_c[0].field_0x02 = 0;
                slot->entries_c[1].field_0x02 = 0;
                slot->entries_c[2].field_0x02 = 0;
            }
            slot->entries_c[0].page_kind = 14;
            slot->entries_c[1].page_kind = 15;
            slot->entries_c[2].page_kind = 16;
            slot->entries_c[3].field_0x02 = 1;
            slot->entries_c[3].page_kind = 17;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    if (table != 0) {
        for (entry = slot->entries_c, i = 0; i < (s8)slot->entry_count_c; i++, entry++) {
            entry->field_0x08 = *table;
            table++;
        }
    }
    menu_sub_row_select(slot->field_0x01A);
}

/* 0x8029FCFC (0x2BC): builds the first slot's second entry array (the top menu's rows) for the list `field_0x019`
 * selects: which rows may be picked and each row's message word. */
extern "C" void menu_top_rows_build(void)
{
    MenuSlot* slot = &menu_work.slot[0];
    u32* table = 0;
    MenuEntry* entry;
    s32 i;

    memset(slot->entries_b, 0, 0xC0);
    switch ((s8)slot->field_0x019) {
    case 0:
        slot->entry_count_b = 6;
        if (slot->field_0x00F == 1) {
            if (menu_quest_state_ck() == 1) {
                table = (u32*)get_menu_msg_tbl(3);
            } else {
                table = (u32*)get_menu_msg_tbl(2);
            }
            slot->entries_b[1].field_0x02 = menu_slot_worker_busy_ck(slot);
            slot->entries_b[2].field_0x02 = menu_slot_worker_busy_ck(slot);
            slot->entries_b[5].field_0x02 = menu_item_input_ok(slot);
            if (PlayMode_ck() == 2 && system_w.field_0x8af == 0) {
                slot->entries_b[3].field_0x02 = 0;
            } else {
                slot->entries_b[3].field_0x02 = 1;
            }
        } else {
            table = (u32*)get_menu_msg_tbl(1);
            slot->entries_b[1].field_0x02 = menu_slot_worker_busy_ck(slot);
            slot->entries_b[2].field_0x02 = menu_slot_worker_busy_ck(slot);
            slot->entries_b[5].field_0x02 = menu_item_input_ok(slot);
            slot->entries_b[3].field_0x02 = 1;
        }
        slot->entries_b[0].field_0x02 = 1;
        slot->entries_b[4].field_0x02 = 1;
        break;
    case 1:
        slot->entry_count_b = 6;
        if (game_ready_ck() != 0) {
            table = (u32*)get_menu_msg_tbl(5);
        } else {
            table = (u32*)get_menu_msg_tbl(4);
        }
        slot->entries_b[0].field_0x02 = 1;
        slot->entries_b[1].field_0x02 = 1;
        slot->entries_b[2].field_0x02 = 1;
        slot->entries_b[5].field_0x02 = 1;
        if (slot->field_0x00F == 2) {
            slot->entries_b[3].field_0x02 = 1;
            if (game_ready_ck() != 0) {
                slot->entries_b[4].field_0x02 = 1;
            } else if (userdata_progress_flag_ck(25) != 0) {
                slot->entries_b[4].field_0x02 = 1;
            }
        }
        break;
    case 2:
        slot->entry_count_b = 6;
        table = (u32*)get_menu_msg_tbl(6);
        slot->entries_b[0].field_0x02 = 1;
        slot->entries_b[1].field_0x02 = 1;
        slot->entries_b[5].field_0x02 = 1;
        if (slot->field_0x00F == 2) {
            slot->entries_b[2].field_0x02 = 1;
            s8 usable = fn_8027D738(slot->worker) != 1;

            slot->entries_b[3].field_0x02 = usable;
            slot->entries_b[4].field_0x02 = usable;
        } else {
            slot->entries_b[3].field_0x02 = 0;
            slot->entries_b[2].field_0x02 = 0;
            slot->entries_b[4].field_0x02 = 0;
        }
        break;
    default:
        slot->entry_count_b = 0;
        break;
    }
    if ((s8)slot->field_0x01B >= (s8)slot->entry_count_b) {
        slot->field_0x01B = (s8)(slot->entry_count_b - 1);
    }
    for (i = 0, entry = slot->entries_b; i < (s8)slot->entry_count_b; entry++, i++) {
        if (table != 0) {
            entry->field_0x08 = *table;
            table++;
        }
    }
}

/* 0x8029FFB8: marks the one entry `index` names as selected in the first entry array and clears the
 * rest, then re-enters the menu's work initialiser. */
extern "C" void menu_tab_select(MenuSlot* slot, s32 index)
{
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_a; i++) {
        if (i == index) {
            slot->entries_a[i].selected = 1;
        } else {
            slot->entries_a[i].selected = 0;
        }
    }
    return menu_top_rows_build();
}

/* 0x8029FFFC: the same selection over the second entry array. */
extern "C" void menu_slot_entry_b_select(MenuSlot* slot, s32 index)
{
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_b; i++) {
        if (i == index) {
            slot->entries_b[i].selected = 1;
        } else {
            slot->entries_b[i].selected = 0;
        }
    }
}

/* 0x802A0040: the same selection over the first slot's third entry array. */
extern "C" void menu_sub_row_select(s32 index)
{
    MenuSlot* slot = &menu_work.slot[0];
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_c; i++) {
        if (i == index) {
            slot->entries_c[i].selected = 1;
        } else {
            slot->entries_c[i].selected = 0;
        }
    }
}

/* 0x802A008C: whether the slot's displayed player is in a state the item menu accepts input in. */
extern "C" u32 menu_item_input_ok(MenuSlot* slot)
{
    _PLW* worker = slot->worker;

    if (slot->field_0x00F == 1) {
        if (quest_select_ready_ck() != 0 && (u8)lb_unlock_cond5_on_ck() == 0) {
            return 0;
        }
        if (Pl_motion_input_ck(0) == 1) {
            return 0;
        }
        if (pl_alive_act_ck(worker) == 0) {
            return 0;
        }
    } else {
        if ((worker->field_0xB01 & 7) != 0) {
            return 0;
        }
        if (fn_8027D738(worker) == 1) {
            return 0;
        }
    }
    return 1;
}

/* 0x802A0148: whether the menu is in the state `move_work_state_ck` reports or a single area is loaded. */
extern "C" u32 menu_quest_state_ck(void)
{
    if (move_work_state_ck() != 0) {
        return 1;
    }
    return quest_select_ready_ck() == 1;
}

/* 0x802A0188: releases both menu slots and puts the menu back into its unloaded state. */
extern "C" void menu_item_close(void)
{
    MenuSlot* slot;
    s32 i;

    ainpc_entry_kill_kind(1);
    for (i = 0, slot = &menu_work.slot[0]; i < 2; slot++, i++) {
        if (slot->active != 0) {
            menu_slot_items_return(slot);
        }
    }
    menu_work.slot[1].active = 0;
    menu_work.slot[0].active = 0;
    if (menu_work.slot[0].field_0x32D != 0) {
        kbd_close_call();
        menu_work.slot[0].field_0x32D = 0;
        menu_work.slot[1].field_0x32D = 0;
    }
    ainpc_page_state_init();
    note_pane_reset();
    if (game_ready_ck() == 1) {
        if (menu_work.slot[0].field_0x31E == 1 || menu_work.slot[1].field_0x31E == 1) {
            sendUserProfilePart(4, 0, 0);
        }
    }
}

/* 0x802A025C: the same release for the frame the caller's own menu state ends; returns whether the
 * display was handed back. */
extern "C" u32 menu_quest_release(void)
{
    u32 released = 0;

    ainpc_entry_kill_kind(1);
    note_pane_reset();
    ainpc_page_hold_clear();
    if (game_ready_ck() == 1 && menu_work.slot[0].field_0x31E == 1) {
        sendUserProfilePart(4, 0, 0);
        released = 1;
    }
    return released;
}

/* 0x802A02CC: the slot-0 form of the predicate below. */
extern "C" u32 menu_busy_ck(void)
{
    return menu_slot_busy_ck(0);
}

/* 0x802A02D4: whether a slot is in use, else what `cockpit_msg_slot_busy_ck` reports for its index. */
extern "C" u32 menu_slot_busy_ck(u8 idx)
{
    if (menu_work.slot[idx].active != 0) {
        return 1;
    }
    return cockpit_msg_slot_busy_ck(idx);
}

/* 0x802A0304: the same predicate for the slot the frame work selects, which is the current game
 * mode's own slot. */
extern "C" u32 menu_item_frame_update(MenuFrameWork* self)
{
    MenuSlot* slot = &menu_work.slot[0];
    u32 idx = 0;

    if (self != 0) {
        switch ((u8)GameMode_ck()) {
        default:
            return 0;
        case 1:
            if (screen_split_mode_ck() != 0) {
                idx = self->slot_index;
                slot = &menu_work.slot[idx];
            }
            break;
        case 2:
            break;
        }
    }
    if (slot->active != 0) {
        return 1;
    }
    return cockpit_msg_slot_busy_ck(idx);
}

/* 0x802A03A4: whether the first slot is in use in its single-player mode. */
extern "C" u32 menu_top_busy_ck(void)
{
    if (menu_work.slot[0].active != 0 && menu_work.slot[0].field_0x000 < 2) {
        return 1;
    }
    if (cockpit_msg_slot_busy_ck(0) != 0) {
        return 1;
    }
    return 0;
}

/* 0x802A0404: the frame work's own predicate - the slot it selects, or its own two flags. */
extern "C" u32 menu_frame_busy_ck(MenuFrameWork* self)
{
    if (menu_item_frame_update(self) == 1) {
        return 1;
    }
    if (self->field_0x5BC != 0) {
        return 1;
    }
    return self->field_0x5BE != 0;
}

/* 0x802A0464: whether the first slot is in use, in mode 1, with the value 3 stored and its flag
 * cleared - the state the menu's item list accepts input in. */
extern "C" u32 menu_item_list_ready_ck(void)
{
    if (menu_work.slot[0].active != 0 && menu_work.slot[0].field_0x000 == 1 &&
        menu_work.slot[0].field_0x014 == 3 && menu_work.slot[0].field_0x001 == 0) {
        return 1;
    }
    return 0;
}

/* 0x802A04B0: whether the first slot is in use in mode 2 with its flag set to 2. */
extern "C" u32 menu_closing_ck(void)
{
    if (menu_work.slot[0].active != 0 && menu_work.slot[0].field_0x000 == 2 &&
        menu_work.slot[0].field_0x001 == 2) {
        return 1;
    }
    return 0;
}

/* 0x802A04EC: whether a slot is in use in mode 1 and carries `value` as its stored byte. */
extern "C" u32 menu_page_open_ck(s8 value, u8 idx)
{
    if (menu_work.slot[idx].active != 0 && menu_work.slot[idx].field_0x000 == 1 &&
        menu_work.slot[idx].field_0x014 == value) {
        return 1;
    }
    return 0;
}

/* 0x802A053C: stores the first slot's +0x021 byte. */
extern "C" void menu_slot0_lock_set(u8 value)
{
    menu_work.slot[0].field_0x021 = value;
}

/* 0x802A054C: the same store for the slot the caller names. */
extern "C" void menu_slot_lock_set(u8 idx, u8 value)
{
    menu_work.slot[idx].field_0x021 = value;
}

/* 0x802A0568 (0xE8): clears the menu work, numbers both slots and clears each slot's placement records. */
extern "C" void menu_item_work_init(void)
{
    MenuSlot* slot;
    s32 i;
    MenuPlaceRec* rec;
    s32 j;

    memset(&menu_work, 0, sizeof(menu_work));
    menu_work.slot[0].field_0x011 = 0;
    menu_work.slot[1].field_0x011 = 1;
    cockpit_msg_slot_clear(0);
    cockpit_msg_slot_clear(1);
    ainpc_page_state_init();
    for (i = 0, slot = menu_work.slot; i < 2; slot++, i++) {
        slot->place_entries = (MenuPlaceRec*)get_move_work_adrs(1);
        slot->place_count = (u16)get_move_work_max(1);
        rec = slot->place_entries;
        if (rec != 0) {
            for (j = 0; j < slot->place_count; j++, rec++) {
                memset(rec, 0, sizeof(MenuPlaceRec));
                rec->index = j;
            }
        }
    }
}

/* 0x802A0650 (0x150): runs the open page's frame handler by the slot's page kind; returns its result (2 back to
 * the top menu, 3/4 close the menu). */
extern "C" s32 menu_page_step(MenuSlot* slot)
{
    s32 result = 0;

    switch (slot->field_0x014) {
    case 0:
        result = menu_item_page_step(slot);
        break;
    case 1:
        result = menu_equip_panel_step(slot);
        break;
    case 2:
        result = menu_page02_step(slot);
        break;
    case 3:
        result = menu_effect_slot_step(slot);
        break;
    case 5:
        result = menu_quit_step(slot);
        break;
    case 6:
        result = menu_lobby_exit_step(slot);
        break;
    case 10:
        result = menu_item_sub_page10_step(slot);
        break;
    case 7:
        result = menu_status_page_step(slot);
        break;
    case 8:
        result = equip_detail_screen_step(slot);
        break;
    case 9:
        result = menu_item_sub_page09_step(slot);
        break;
    case 4:
        result = movie_page04_step(slot);
        break;
    case 11:
        result = plsearch_page11_step(slot);
        break;
    case 12:
        result = placeinfo_page12_step(slot);
        break;
    case 13:
        result = placeinfo_page13_step(slot);
        break;
    case 14:
        result = menu_item_sub_page14_step(slot);
        break;
    case 15:
        result = menu_item_sub_page15_step(slot);
        break;
    case 16:
        result = menu_item_sub_page16_step(slot);
        break;
    case 17:
        result = movie_page17_step(slot);
        break;
    case 18:
        result = movie_page18_step(slot);
        break;
    case 25:
        result = note_page_step(slot);
        break;
    case 19:
    case 20:
    case 21:
        result = plsearch_page19_step(slot);
        break;
    case 22:
    case 23:
    case 24:
        result = plsearch_page22_step(slot);
        break;
    }
    return result;
}

/* 0x802A07A0 (0xD20): the menu work's per-frame driver for the single-screen menu: the pad copy, the open/close
 * buttons, the top menu and its sub-list, the open page's frame, the keyboard and the close sequence, then the
 * menu's draw task. */
extern "C" void menu_work_update(void)
{
    MenuSlot* slot = &menu_work.slot[0];
    u16 keys;
    u32 sound;
    s32 result;

    slot->field_0x00F = GameMode_ck();
    if (slot->field_0x00F == 2) {
        if (game_ready_ck() == 0) {
            if (userdata_progress_flag_ck(99) == 0) {
                return;
            }
        } else if (lobby_w.community_flag_0x009 == 1) {
            return;
        }
    }
    if ((u8)event_demo_running_ck() == 1) {
        return;
    }
    switch (GameMode_ck()) {
    case 1: {
        SysQuestMove* quest = (SysQuestMove*)get_move_work_adrs(0);

        if (quest != 0 && quest->menu_lock_0x112 != 1 && get_fade_stat(0) != 2) {
            break;
        }
        /* falls through */
    }
    default:
        return;
    case 2:
        if (lobby_w.area_change_0x026 == 1) {
            return;
        }
        break;
    }
    if (pad_hold_ck() != 1 && cockpit_msg_slot_update(0) == 0 &&
        (GameMode_ck() != 2 ||
         ((lobby_w.state_0x000 != 36 || slot->field_0x000 == 2) &&
          (slot->worker == 0 || slot->worker->act_no != 43)))) {
        menu_pad_read(slot);
        if (chk_pointer() != 0) {
            menu_pad_clear(slot);
        }
        if (slot->field_0x32D == 0) {
            if (slot->active != 0) {
                system_w.menu_closed_0x8c0 = 0;
            } else {
                system_w.menu_closed_0x8c0 = 1;
            }
            if (system_menu_callback_call() == 1) {
                if (slot->active == 0) {
                    menu_main_open(0);
                    if (slot->active == 0) {
                        sysSE_req(2);
                    } else {
                        slot->field_0x000 = 2;
                        slot->field_0x001 = 0;
                        slot->field_0x32D = 2;
                    }
                } else if (slot->field_0x021 == 0 && slot->field_0x000 < 2) {
                    slot->field_0x000 = 2;
                    slot->field_0x001 = 0;
                    slot->field_0x32D = 2;
                }
            }
        }
        if ((slot->buttons_trig & 0x400) != 0 && slot->field_0x32D == 0) {
            if (slot->active == 0) {
                menu_main_open(1);
                if (slot->active == 0) {
                    sysSE_req(2);
                }
            } else if (slot->open_wait == 0 && slot->field_0x021 == 0 && slot->field_0x000 < 2) {
                slot->field_0x000 = 2;
                slot->field_0x001 = 0;
                sysSE_req(1);
            }
        }
        if (slot->active == 0) {
            return;
        }
        if (slot->open_wait != 0) {
            slot->open_wait--;
        }
        keys = slot->buttons_trig | (slot->field_0x008 & 0xF);
        if (slot->field_0x021 == 0 && slot->field_0x000 < 2) {
            ainpc_page_hold_set();
        } else {
            ainpc_page_hold_clear();
        }
        switch (slot->field_0x000) {
        case 0:
            switch (slot->field_0x015) {
            case 0:
                menu_top_rows_build();
                menu_slot_entry_b_select(slot, slot->field_0x01B);
                if (keys & 0x20) {
                    if (slot->open_wait == 0) {
                        slot->field_0x000 = 3;
                        slot->field_0x001 = 0;
                        sysSE_req(1);
                    }
                } else if (keys & 0x10) {
                    if (slot->entries_a[slot->field_0x019].field_0x02 != 0 &&
                        slot->entries_b[slot->field_0x01B].field_0x02 != 0) {
                        switch (slot->field_0x019) {
                        case 0:
                            switch (slot->field_0x01B) {
                            case 0:
                                menu_item_page_open(slot);
                                slot->field_0x000 = 1;
                                break;
                            case 1:
                                slot->ticket = 0xFFFF;
                                if (cockpit_ticket_acquire(0, &slot->ticket) >= 0) {
                                    equip_panel_slot_reset(slot);
                                    slot->field_0x000 = 1;
                                    slot->field_0x002 = 0;
                                    slot->field_0x001 = 0;
                                }
                                break;
                            case 2:
                                menu_page02_open(slot);
                                slot->field_0x000 = 1;
                                slot->field_0x002 = 0;
                                slot->field_0x001 = 0;
                                break;
                            case 3:
                                menu_effect_slot_open(slot);
                                slot->field_0x000 = 1;
                                sysSE_req(25);
                                break;
                            case 4:
                                plsearch_page11_open(slot);
                                slot->field_0x000 = 1;
                                break;
                            case 5:
                                if (slot->field_0x00F == 1) {
                                    menu_quit_open(slot);
                                    slot->field_0x000 = 1;
                                } else {
                                    slot->field_0x015 = 1;
                                    slot->sub_list_open = 1;
                                    slot->field_0x01A = 2;
                                    slot->field_0x23A = 0;
                                    menu_sub_rows_build(slot);
                                }
                                break;
                            }
                            if (slot->field_0x01B != 3) {
                                sysSE_req(0);
                            }
                            break;
                        case 1:
                            switch (slot->field_0x01B) {
                            case 0:
                                menu_item_sub_page10_open(slot);
                                slot->field_0x000 = 1;
                                break;
                            case 1:
                                menu_status_page_open(slot);
                                slot->field_0x000 = 1;
                                break;
                            case 2:
                                equip_detail_screen_reset(slot);
                                slot->field_0x000 = 1;
                                break;
                            case 3:
                                placeinfo_page12_open(slot);
                                slot->field_0x000 = 1;
                                break;
                            case 4:
                                placeinfo_page13_open(slot);
                                slot->field_0x000 = 1;
                                break;
                            case 5:
                                movie_page04_open(slot);
                                slot->field_0x000 = 1;
                                break;
                            }
                            sysSE_req(0);
                            break;
                        case 2:
                            switch (slot->field_0x01B) {
                            case 0:
                                menu_item_sub_page09_open(slot);
                                slot->field_0x000 = 1;
                                slot->field_0x001 = 0;
                                break;
                            case 1:
                                note_page_open(slot, 25);
                                slot->field_0x000 = 1;
                                slot->field_0x001 = 0;
                                break;
                            case 2:
                                slot->field_0x015 = 1;
                                slot->sub_list_open = 1;
                                slot->field_0x01A = 0;
                                slot->field_0x23A = 0;
                                menu_sub_rows_build(slot);
                                break;
                            case 4:
                                slot->field_0x015 = 1;
                                slot->sub_list_open = 1;
                                slot->field_0x01A = 0;
                                slot->field_0x23A = 0;
                                menu_sub_rows_build(slot);
                                break;
                            case 3:
                                movie_page18_open(slot);
                                slot->field_0x000 = 1;
                                break;
                            case 5:
                                slot->field_0x015 = 1;
                                slot->sub_list_open = 1;
                                slot->field_0x01A = 0;
                                slot->field_0x23A = 0;
                                menu_sub_rows_build(slot);
                                break;
                            }
                            sysSE_req(0);
                            break;
                        }
                    } else {
                        sysSE_req(2);
                    }
                } else {
                    if (slot->field_0x23A != 0) {
                        slot->field_0x23A--;
                    }
                    switch (menu_top_shortcut_ck(slot, keys)) {
                    case 0:
                        if (keys & 0xC) {
                            slot->field_0x019 =
                                menu_cursor_step_open_last(slot->field_0x019, (s8&)slot->entry_count_a, keys, 4, 8, 6);
                            menu_tab_select(slot, slot->field_0x019);
                            menu_slot_entry_b_select(slot, slot->field_0x01B);
                        } else if (keys & 3) {
                            slot->field_0x01B = menu_cursor_step(slot->field_0x01B, slot->entry_count_b, keys, 1, 2);
                            menu_slot_entry_b_select(slot, slot->field_0x01B);
                        }
                        break;
                    case 4:
                        slot->field_0x000 = 3;
                        slot->field_0x001 = 0;
                        slot->field_0x015 = 0;
                        break;
                    }
                }
                break;
            case 1:
                if (slot->field_0x23A < sprite_ary_last_frame((u16*)get_menu_lsp_tbl(30))) {
                    slot->field_0x23A++;
                } else {
                    slot->field_0x01A = 0;
                    slot->field_0x015 = 2;
                }
                menu_sub_row_select(slot->field_0x01A);
                break;
            case 2:
                if (keys & 0x10) {
                    MenuEntry* entry = &slot->entries_c[slot->field_0x01A];

                    if (entry->field_0x02 != 0) {
                        slot->field_0x000 = 1;
                        slot->field_0x014 = entry->page_kind;
                        sound = 1;
                        switch ((s8)entry->page_kind) {
                        case 6:
                            menu_lobby_exit_open(slot);
                            sound = 0;
                            sysSE_req(5);
                            break;
                        case 14:
                            menu_item_sub_page14_open(slot);
                            break;
                        case 15:
                            menu_item_sub_page15_open(slot);
                            break;
                        case 16:
                            menu_item_sub_page16_open(slot);
                            break;
                        case 17:
                            movie_page17_open(slot);
                            break;
                        case 19:
                        case 20:
                        case 21:
                            plsearch_page19_open(slot);
                            break;
                        case 22:
                        case 24:
                            sound = 0;
                            sysSE_req(27);
                            plsearch_page22_open(slot);
                            break;
                        case 23:
                            plsearch_page22_open(slot);
                            break;
                        }
                        if (sound == 1) {
                            sysSE_req(0);
                        }
                    } else {
                        sysSE_req(2);
                    }
                } else if (keys & 0x20) {
                    sysSE_req(1);
                    menu_top_rows_build();
                    slot->field_0x015 = 0;
                    slot->field_0x23A = 0;
                } else {
                    switch (menu_top_shortcut_ck(slot, keys)) {
                    case 0:
                        if (keys & 3) {
                            slot->field_0x01A = menu_cursor_step(slot->field_0x01A, (s8)slot->entry_count_c, keys, 1, 2);
                            menu_sub_row_select(slot->field_0x01A);
                        }
                        break;
                    case 4:
                        slot->field_0x000 = 3;
                        slot->field_0x001 = 0;
                        break;
                    }
                }
                break;
            }
            break;
        case 1:
            result = menu_page_step(slot);
            switch (result) {
            case 1:
            case 2: {
                slot->field_0x000 = 0;
                slot->field_0x23A = slot->field_0x015 == 0 ? 0 : sprite_ary_last_frame((u16*)get_menu_lsp_tbl(30));
                slot->field_0x021 = 0;
                menu_top_reset(slot);
                break;
            }
            case 3:
            case 4:
                slot->field_0x000 = 3;
                slot->field_0x001 = 0;
                break;
            }
            break;
        case 2:
            switch (slot->field_0x001) {
            case 0:
                if (game_ready_ck() == 1) {
                    if (GameMode_ck() == 2) {
                        lobby_w.state_0x000 = 36;
                    }
                    menu_slot0_lock_set(1);
                    note_pane_reset();
                    if (slot->field_0x32D == 0) {
                        slot->field_0x32D = 1;
                    }
                    slot->field_0x001++;
                } else {
                    slot->field_0x000 = 3;
                    slot->field_0x001 = 0;
                }
                break;
            case 1: {
                u8 mode = 1;

                if (slot->field_0x32D == 2) {
                    mode = 0x81;
                }
                if (kbd_open(mode) == 1) {
                    menu_work.slot[0].field_0x32D = 1;
                    note_pen_seed();
                    slot->field_0x001++;
                }
                break;
            }
            case 2:
                switch (kbd_move()) {
                case 1:
                case -1:
                    menu_work.slot[0].field_0x32D = 0;
                    slot->open_wait = 8;
                    slot->field_0x001 = 3;
                    break;
                }
                break;
            case 3:
                if (slot->open_wait == 0) {
                    if (GameMode_ck() == 2) {
                        lobby_w.state_0x000 = 0;
                    }
                    menu_slot0_lock_set(0);
                    slot->field_0x000 = 3;
                    slot->field_0x001 = 0;
                }
                break;
            }
            break;
        case 3:
            switch (slot->field_0x001) {
            case 0:
                if (menu_quest_release() == 0) {
                    if (game_ready_ck() == 1 && isServerSelectState() == 0) {
                        menu_slot0_lock_set(1);
                        ainpc_page_hold_clear();
                        slot->field_0x001++;
                    } else {
                        slot->active = 0;
                        slot->field_0x32D = 0;
                        slot->field_0x000 = 0;
                        slot->field_0x001 = 0;
                    }
                } else {
                    menu_slot0_lock_set(1);
                    ainpc_page_hold_clear();
                    slot->field_0x001++;
                }
                break;
            case 1:
                if (menu_work.slot[0].field_0x31E == 1 || menu_work.slot[0].field_0x31F != get_option_cfg(27)) {
                    requestFriendSync(&lobby_state_block.result_0x1695);
                    slot->field_0x001++;
                } else {
                    menu_slot0_lock_set(0);
                    slot->active = 0;
                    slot->field_0x32D = 0;
                    slot->field_0x000 = 0;
                    slot->field_0x001 = 0;
                }
                break;
            case 2:
                if (lobby_state_block.result_0x1695 != 0) {
                    menu_slot0_lock_set(0);
                    slot->active = 0;
                    slot->field_0x32D = 0;
                    slot->field_0x000 = 0;
                    slot->field_0x001 = 0;
                }
                break;
            }
            break;
        }
    }
    if ((u32)lb_quest_work_active_ck() != 1) {
        if (cockpit_msg_slot_busy_ck(0) != 0) {
            subTransSetPrio(6, (u32)cockpit_msg_slot_draw, 0, 0);
        } else if (slot->active != 0) {
            subTransSetPrio(6, (u32)menu_main_draw, 0, 0);
        }
    }
}

/* 0x802A14C0 (0x138): the top menu's shortcut buttons: 0x1000 opens the effect slot page, 0x2000 sends the
 * quest-ready flag, 0x4000 toggles the quest mark.  Returns 1 while busy, 4 when a shortcut closed the menu. */
extern "C" s32 menu_top_shortcut_ck(MenuSlot* slot, u16 keys)
{
    if (slot->field_0x021 == 1 || slot->open_wait != 0) {
        return 1;
    }
    if ((PlayMode_ck() != 2 || system_w.field_0x8af != 0) && (keys & 0x1000) != 0) {
        menu_effect_slot_open(slot);
        slot->field_0x000 = 1;
        slot->field_0x019 = 0;
        slot->field_0x01B = 3;
        slot->field_0x015 = 0;
        sysSE_req(25);
        return 1;
    }
    if (slot->field_0x00F == 1) {
        if ((keys & 0x2000) != 0) {
            if (quest_slot_arm_all_a((s8)my_player_no()) != 0) {
                lb_entry_selected_send(slot->worker);
                return 4;
            }
            sysSE_req(2);
        } else if ((keys & 0x4000) != 0) {
            quest_mark_toggle(&cockpit_work[slot->field_0x011]);
            return 4;
        }
    }
    return 0;
}

/* 0x802A15F8 (0x5C): the split-screen menu's quest-mark shortcut (0x4000); returns 4 when it fired. */
extern "C" s32 menu_split_shortcut_ck(MenuSlot* slot, u16 keys)
{
    if (slot->field_0x00F == 1 && (keys & 0x4000) != 0) {
        quest_mark_toggle(&cockpit_work[slot->field_0x011]);
        return 4;
    }
    return 0;
}

/* 0x802A1654 (0xA4): copies the slot player's pad words into the slot, or clears them while the network is busy. */
extern "C" void menu_pad_read(MenuSlot* slot)
{
    PadButtons* pad = &Psw[slot->field_0x011].button_0x2C0;

    if (lobby_net_busy_ck() == 1) {
        menu_pad_clear(slot);
        return;
    }
    slot->buttons_trig = pad->pressed_0x04;
    slot->buttons_0x006 = pad->hold_0x00;
    slot->field_0x008 = pad->held_0x14 | pad->pressed_0x04;
    slot->buttons_0x00C = pad->edge_on_0x1F | (pad->edge_0x1D & ~pad->edge_off_0x1E);
    slot->buttons_0x00A = pad->trigger_0x0C;
}

/* 0x802A16F8: clears the slot's five pad words. */
extern "C" void menu_pad_clear(MenuSlot* slot)
{
    slot->buttons_trig = 0;
    slot->buttons_0x006 = 0;
    slot->field_0x008 = 0;
    slot->buttons_0x00C = 0;
    slot->buttons_0x00A = 0;
}

/* 0x802A1714 (0xCE0): the single-screen menu's draw task: the top menu (its tabs, six rows, the open sub-list, the
 * help line and the button guides), or the open page's own draw. */
extern "C" void menu_main_draw(u32 idx)
{
    MenuSlot* slot = &menu_work.slot[0];
    _mh_ivec2_ pos;
    _mh_ivec2_ cell;
    _SPR_DATA_ spr;
    MenuEntry* entry;
    s32 i;
    s16 offset;
    u16 frame;
    u32 selected;
    u32 top;
    u32 flags;
    u16* table;
    u16 help;
    s8* text;

    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    switch (menu_work.slot[0].field_0x000) {
    case 0:
        offset = 0;
        if ((u32)(slot->field_0x015 - 1) > 1) {
            if (slot->field_0x015 == 0) {
                offset = slot->field_0x23A * 100;
                pos.x = offset;
                pos.y = 0;
                ainpc_page_pos_set(pos);
                frame = 0;
            }
        } else {
            frame = slot->field_0x23A;
        }
        get_lsp_data(488, &pos);
        pos.x += offset;
        draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(20), frame, &pos);
        if ((s8)slot->entry_count_a == 2) {
            switch (slot->field_0x019) {
            case 0:
                table = (u16*)get_menu_lsp_tbl(21);
                break;
            case 1:
                table = (u16*)get_menu_lsp_tbl(22);
                break;
            }
        } else {
            switch (slot->field_0x019) {
            case 0:
                table = (u16*)get_menu_lsp_tbl(23);
                break;
            case 1:
                table = (u16*)get_menu_lsp_tbl(24);
                break;
            case 2:
                table = (u16*)get_menu_lsp_tbl(25);
                break;
            }
        }
        draw_sprite_anim_ary(table, frame, &pos);
        get_lsp_data(540, &pos);
        pos.x += offset;
        spr_data_copy(&spr, get_lsp_data(545, 0));
        for (entry = slot->entries_b, i = 0; i < 6; i++, entry++) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(26), &pos);
            flags = 0;
            selected = entry->selected != 0;
            top = slot->field_0x015 == 0;
            if (entry->field_0x02 == 0) {
                spr.color = 0x646464FF;
            } else if (top == 1) {
                if (selected == 1) {
                    spr.color = 0xF0F0F0FF;
                } else {
                    spr.color = 0xC3C3C3FF;
                }
            } else if (selected == 1) {
                spr.color = 0xE8A40FFF;
            } else {
                spr.color = 0x878787FF;
            }
            if (entry->field_0x02 != 0 && entry->selected != 0) {
                flags = 4;
            }
            if (i == 0 && slot->field_0x019 == 0) {
                _SPR_DATA_ first = *get_lsp_data(546, 0);

                first.color = spr.color;
                draw_font(first, (s8*)entry->field_0x08, flags, &pos);
                if (slot->field_0x010 == 0) {
                    draw_font_idx(547, (s8*)get_menu_msg(15, 0), flags, &pos);
                } else {
                    draw_font_idx(548, (s8*)get_menu_msg(15, 1), flags, &pos);
                }
            } else {
                draw_font(spr, (s8*)entry->field_0x08, flags, &pos);
            }
            if (entry->selected != 0 && slot->field_0x015 != 1) {
                put_menu_cursor((u16*)get_menu_lsp_tbl(27), frame, &pos);
            }
            pos.y += 26;
        }
        if (slot->field_0x015 != 0) {
            if (slot->sub_list_open == 0) {
                get_lsp_data(524, &pos);
                pos.x += offset;
                pos.y += (s16)(slot->field_0x01B * 26);
                menu_sub_list_frame_draw(slot, frame, &pos);
                sprite_frame_apply(&spr, 537, frame, 0);
                if ((u8)spr.color != 0) {
                    for (entry = slot->entries_c, i = 0; i < slot->entry_count_c; i++, entry++) {
                        draw_sprite_anim_idx(538, frame, &pos);
                        flags = 0;
                        spr.color = 0x646464FF;
                        if (entry->field_0x02 != 0) {
                            spr.color = 0xC3C3C3FF;
                            if (entry->selected != 0) {
                                flags = 4;
                                spr.color = 0xF0F0F0FF;
                            }
                        }
                        draw_font(spr, (s8*)entry->field_0x08, flags, &pos);
                        pos.y += 26;
                    }
                }
                get_lsp_data(560, &pos);
                pos.x += offset;
                pos.y += (s16)((slot->field_0x01B + slot->field_0x01A) * 26);
                if (slot->field_0x015 == 2) {
                    put_menu_cursor((u16*)get_menu_lsp_tbl(37), frame, &pos);
                }
            } else {
                get_lsp_data(584, &pos);
                pos.x += offset;
                pos.y += (s16)(slot->field_0x01B * 26);
                menu_sub_list_frame_draw(slot, frame, &pos);
                sprite_frame_apply(&spr, 618, frame, 0);
                if ((u8)spr.color != 0) {
                    if (slot->entry_count_c == 3) {
                        table = (u16*)get_menu_lsp_tbl(34);
                    } else {
                        table = (u16*)get_menu_lsp_tbl(36);
                    }
                    for (entry = slot->entries_c, i = 0; i < slot->entry_count_c; i++, entry++, table++) {
                        _SPR_DATA_* rec = get_lsp_data(*table, 0);

                        cell.x = pos.x + rec->pos.x;
                        cell.y = pos.y + rec->pos.y;
                        draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(39), frame, &cell);
                        flags = 0;
                        spr.color = 0x646464FF;
                        if (entry->field_0x02 != 0) {
                            spr.color = 0xC3C3C3FF;
                            if (entry->selected != 0) {
                                flags = 4;
                                spr.color = 0xF0F0F0FF;
                            }
                        }
                        draw_font(spr, (s8*)entry->field_0x08, flags, &cell);
                    }
                }
                get_lsp_data(623, &pos);
                pos.x += offset;
                pos.y += (s16)((slot->field_0x01B + slot->field_0x01A - slot->entry_count_c + 4) * 26);
                if (slot->field_0x015 == 2) {
                    put_menu_cursor((u16*)get_menu_lsp_tbl(38), frame, &pos);
                }
            }
        }
        get_lsp_data(581, &pos);
        pos.x += offset;
        draw_window_frame_preset(pos.x, pos.y, 0);
        switch (slot->field_0x019) {
        case 0:
            text = (s8*)get_menu_msg(18, slot->field_0x01B);
            switch (slot->field_0x01B) {
            case 0:
                if (slot->field_0x010 == 1) {
                    text = (s8*)get_menu_msg(18, 6);
                }
                break;
            case 5:
                if (slot->field_0x00F == 1) {
                    if (move_work_state_ck() == 1) {
                        help = 0;
                    } else if (game_ready_ck() == 0) {
                        if (PlayMode_ck() == 2 && system_w.field_0x8af == 0) {
                            help = 3;
                        } else {
                            help = 1;
                        }
                    } else {
                        help = 2;
                    }
                    if (quest_select_ready_ck() != 0) {
                        help = 0;
                    }
                    text = (s8*)get_menu_msg(23, help);
                } else if (slot->field_0x015 == 2) {
                    text = (s8*)get_menu_msg(24, slot->field_0x01A);
                }
                break;
            }
            break;
        case 1:
            if (game_ready_ck() != 0) {
                text = (s8*)get_menu_msg(20, slot->field_0x01B);
            } else {
                text = (s8*)get_menu_msg(19, slot->field_0x01B);
            }
            if (slot->field_0x01B == 3 && slot->field_0x015 != 0) {
                text = (s8*)get_menu_msg(25, slot->field_0x01A);
            }
            break;
        case 2:
            text = (s8*)get_menu_msg(21, slot->field_0x01B);
            if (slot->field_0x015 == 2) {
                switch (slot->field_0x01B) {
                case 1:
                    text = (s8*)get_menu_msg(26, slot->field_0x01A);
                    break;
                case 4:
                    text = (s8*)get_menu_msg(28, slot->field_0x01A);
                    break;
                case 2:
                    text = (s8*)get_menu_msg(27, slot->field_0x01A);
                    break;
                case 5:
                    text = (s8*)get_menu_msg(29, slot->field_0x01A);
                    break;
                }
            }
            break;
        }
        font_set_size(16, 16);
        font_print_ex(pos.x + 24, pos.y + 16, 0, text);
        if (chk_pointer() != 0) {
            break;
        }
        if (slot->field_0x00F == 1) {
            get_lsp_data(582, &pos);
            pos.x += offset;
            if (get_cfg(slot->worker->chunk_ofs, 7) == 0) {
                if (quest_mark_flag_get(&cockpit_work[slot->field_0x011]) == 0) {
                    put_button_guide_b(6, &pos);
                } else {
                    put_button_guide_b(7, &pos);
                }
            } else {
                if (quest_mark_flag_get(&cockpit_work[slot->field_0x011]) == 0) {
                    put_button_guide_b(8, &pos);
                } else {
                    put_button_guide_b(9, &pos);
                }
            }
            get_lsp_data(583, &pos);
            if (PlayMode_ck() == 2 && system_w.field_0x8af == 0) {
                break;
            }
            pos.x += offset;
            if (get_cfg(slot->worker->chunk_ofs, 7) == 0) {
                put_button_guide_c(7, &pos);
            } else {
                put_button_guide_c(8, &pos);
            }
        } else {
            get_lsp_data(582, &cell);
            get_lsp_data(583, &pos);
            pos.x += offset;
            pos.y = cell.y;
            if (get_cfg(slot->worker->chunk_ofs, 7) == 0) {
                put_button_guide_c(7, &pos);
            } else {
                put_button_guide_c(8, &pos);
            }
        }
        break;
    case 1:
        switch (slot->field_0x014) {
        case 0:
            menu_item_page_draw(slot);
            break;
        case 1:
            menu_equip_panel_draw(slot);
            break;
        case 2:
            menu_page02_draw(slot);
            break;
        case 3:
            item_page_draw(slot);
            break;
        case 5:
            menu_quit_draw(slot);
            break;
        case 6:
            menu_lobby_exit_draw(slot);
            break;
        case 10:
            menu_item_sub_page10_draw(slot);
            break;
        case 7:
            equip_info_update(slot);
            break;
        case 8:
            equip_detail_screen_draw(slot);
            break;
        case 9:
            menu_item_sub_page09_draw(slot);
            break;
        case 4:
            movie_page04_draw(slot);
            break;
        case 11:
            plsearch_page11_draw(slot);
            break;
        case 12:
            placeinfo_page12_draw(slot);
            break;
        case 13:
            placeinfo_page13_draw(slot);
            break;
        case 14:
            menu_item_sub_page14_draw(slot);
            break;
        case 15:
            menu_item_sub_page15_draw(slot);
            break;
        case 16:
            menu_item_sub_page16_draw(slot);
            break;
        case 17:
            movie_page17_draw(slot);
            break;
        case 18:
            movie_page18_draw(slot);
            break;
        case 19:
            plsearch_page19_draw(slot);
            break;
        case 20:
            plsearch_page19_draw(slot);
            break;
        case 21:
            plsearch_page19_draw(slot);
            break;
        case 22:
            plsearch_page22_draw(slot);
            break;
        case 23:
            plsearch_page22_draw(slot);
            break;
        case 24:
            plsearch_page22_draw(slot);
            break;
        case 25:
            note_page_draw(slot);
            break;
        }
        break;
    }
}

/* 0x802A23F4 (0x15C): draws the open sub-list's frame at `frame`: the two-row animated frame, or the three- and
 * four-row frames built sprite by sprite from the frame table. */
extern "C" void menu_sub_list_frame_draw(MenuSlot* slot, u16 frame, const _mh_ivec2_* pos)
{
    u16* frames;
    u16* ids;
    _SPR_DATA_ anim;
    _SPR_DATA_ spr;

    if (slot->sub_list_open == 0) {
        switch ((s8)slot->entry_count_c) {
        case 2:
            draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(30), frame, pos);
            return;
        case 3:
            ids = (u16*)get_menu_lsp_tbl(31);
            break;
        case 4:
            ids = (u16*)get_menu_lsp_tbl(32);
            break;
        default:
            return;
        }
    } else {
        switch ((s8)slot->entry_count_c) {
        case 3:
            ids = (u16*)get_menu_lsp_tbl(33);
            break;
        case 4:
            ids = (u16*)get_menu_lsp_tbl(35);
            break;
        default:
            return;
        }
    }
    for (frames = (u16*)get_menu_lsp_tbl(30); *frames != 0xFFFF; frames++, ids++) {
        sprite_frame_apply(&anim, *frames, frame, 0);
        spr_data_copy(&spr, get_lsp_data(*ids, 0));
        spr.color = anim.color;
        menu_word_pair_copy(&spr.u_scale, &anim.u_scale);
        draw_sprite(spr, pos);
    }
}

/* 0x802A2550: copies a pair of 16-bit words. */
extern "C" void menu_word_pair_copy(u16* dst, const u16* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

/* 0x802A2564 (0xBC): draws the menu cursor at animation frame `frame`, its base sprite blinking with the cockpit. */
void put_menu_cursor(u16* rows, u16 frame, const _mh_ivec2_* pos)
{
    _SPR_DATA_ spr;

    if (chk_pointer() == 0) {
        sprite_frame_apply(&spr, rows[0], frame, 0);
        u16 scale = (s32)(120.0f * cockpit_blink_get()) + 0x88;

        u16 v = ((u8)spr.color * scale) >> 8;
        spr.color &= 0xFFFFFF00;
        spr.color += v;
        draw_sprite(spr, pos);
        draw_sprite_anim_ary(rows + 1, frame, pos);
    }
}

/* 0x802A2620 (0xD4): steps the use panel's animation while the slot's player can still use the item. */
extern "C" void menu_slot_anim_update(u8 idx)
{
    MenuSlot* slot = &menu_work.slot[idx];

    if (slot->panel_on == 0) {
        return;
    }
    if (GameMode_ck() == 1) {
        if (fn_8027D738(slot->worker) == 0) {
            slot->panel_on = 0;
            return;
        }
    } else if (game_ready_ck() == 1) {
        if (fn_8027D738(slot->worker) == 0) {
            slot->panel_on = 0;
            return;
        }
    } else {
        slot->panel_on = 0;
        return;
    }
    slot->panel_frame = ++slot->panel_frame % sprite_anim_last_frame(790);
}

/* 0x802A26F4 (0x258): draws the use panel: the hunter name, the item name, its count and the button guide. */
extern "C" void menu_slot_panel_draw(u8 idx)
{
    MenuSlot* slot;
    _mh_ivec2_ pos;
    _SPR_DATA_* rec;
    u32 plain;
    u16 id;
    s16 x;
    char text[0x10];

    if (menu_slot_busy_ck(idx) != 0) {
        return;
    }
    slot = &menu_work.slot[idx];
    if (slot->panel_on == 0) {
        return;
    }
    plain = 1;
    if (game_ready_ck() == 1 && get_option_cfg(12) == 1) {
        plain = 0;
    }
    get_lsp_data(778, &pos);
    draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(51), slot->panel_frame, &pos);
    rec = get_lsp_data(798, 0);
    font_set_size(rec->width, rec->height);
    if (plain == 1) {
        font_print_plain(pos.x + rec->pos.x, pos.y + rec->pos.y, 0, slot->name_0x1D2);
    } else {
        font_print_ex(pos.x + rec->pos.x, pos.y + rec->pos.y, 0, (s8*)"%s", slot->label_0x1C8);
    }
    rec = get_lsp_data(799, 0);
    font_set_size(rec->width, rec->height);
    font_print_ex(pos.x + rec->pos.x, pos.y + rec->pos.y, 0, (s8*)"%s", ItemName(slot->item_id_0x1B2));
    sprintf(text, "%d", slot->count_0x1B6);
    id = 801;
    if (slot->count_0x1B6 < 10) {
        id = 800;
    }
    rec = get_lsp_data(id, 0);
    x = menu_text_center_x(text, (s16)(pos.x + rec->pos.x), rec->width);
    font_set_size(rec->width, rec->height);
    font_print_ex(x, pos.y + rec->pos.y, 0, (s8*)"%s", text);
    if (get_cfg(slot->worker->chunk_ofs, 7) == 0) {
        draw_lsp_sprite_variant(838, 6);
    } else {
        draw_lsp_sprite_variant(838, 9);
    }
}

/* 0x802A294C (0x1DC): the split-screen form of the use panel. */
extern "C" void menu_split_panel_draw(u8 idx)
{
    s16 base_x;
    s16 base_y;
    MenuSlot* slot;
    _mh_ivec2_ pos;
    _mh_ivec2_ guide;
    char text[0x8];
    _SPR_DATA_* rec;
    _SPR_DATA_* box;
    u16 id;

    if (menu_slot_busy_ck(idx) != 0) {
        return;
    }
    slot = &menu_work.slot[idx];
    if (slot->panel_on == 0) {
        return;
    }
    get_lsp_data(((u16*)get_menu_lsp_tbl(367))[slot->pad_no], &pos);
    draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(368), slot->panel_frame, &pos);
    rec = get_lsp_data(4050, 0);
    base_x = pos.x - rec->pos.x;
    base_y = pos.y - rec->pos.y;
    rec = get_lsp_data(4070, 0);
    font_set_size(rec->width, rec->height);
    font_print_plain(pos.x + rec->pos.x, pos.y + rec->pos.y, 0, slot->name_0x1D2);
    draw_font_idx(4071, (s8*)ItemName(slot->item_id_0x1B2), 0, &pos);
    sprintf(text, "%d", slot->count_0x1B6);
    id = 4073;
    if (slot->count_0x1B6 < 10) {
        id = 4072;
    }
    draw_font_idx(id, (s8*)text, 1, &pos);
    box = get_lsp_data(4050, 0);
    rec = get_lsp_data(4032, 0);
    guide.x = base_x + rec->pos.x;
    guide.y = box->pos.y + base_y + box->height - 3;
    if (get_arena_cfg(idx, 7) == 0) {
        put_button_guide(6, &guide);
    } else {
        put_button_guide(9, &guide);
    }
}

/* 0x802A2B28 (0x48): re-tests whether the split-screen slot's item row may be picked. */
extern "C" void menu_split_item_row_update(u8 idx)
{
    MenuSlot* slot = &menu_work.slot[idx];

    slot->entries_b[4].field_0x02 = menu_item_input_ok(slot);
}

/* 0x802A2B70 (0x128): opens a split-screen slot for player `idx`. */
extern "C" void menu_split_open(u8 idx)
{
    MenuSlot* slot = &menu_work.slot[idx];
    _PLW* plw;

    slot->field_0x00F = GameMode_ck();
    if ((s32)slot->field_0x00F == 1 && screen_split_mode_ck() != 0) {
        slot->field_0x011 = idx;
        slot->pad_no = player_pad_channel_get(idx);
        plw = (_PLW*)get_move_work_adrs(2) + idx;
        slot->worker = plw;
        slot->page_a = (IdValue*)plw->slot_id;
        if (pl_item_page2_ck(plw) == 1) {
            slot->field_0x010 = 1;
            slot->page_b = slot->page_a + 26;
        } else {
            slot->field_0x010 = 0;
            slot->page_b = 0;
        }
        if ((u32)pl_act_menu_lock_ck(plw) != 1) {
            slot->active = 1;
            slot->field_0x000 = 0;
            slot->field_0x001 = 0;
            slot->field_0x002 = 0;
            slot->page_index = 0;
            slot->field_0x23A = 4;
            slot->field_0x01B = 0;
            menu_split_top_rows_build(slot);
            menu_split_item_row_update(idx);
            slot->open_wait = 10;
            sysSE_req(5);
        }
    }
}

/* 0x802A2C98: clears a slot's in-use flag. */
extern "C" void menu_split_close(u8 idx)
{
    menu_work.slot[idx].active = 0;
}

/* 0x802A2CB8 (0xB0): builds a split-screen slot's five top-menu rows. */
extern "C" void menu_split_top_rows_build(MenuSlot* slot)
{
    u32* table;
    MenuEntry* entry;
    s32 i;

    memset(slot->entries_b, 0, 0x78);
    slot->entry_count_b = 5;
    slot->entries_b[0].field_0x02 = 1;
    slot->entries_b[1].field_0x02 = 1;
    slot->entries_b[2].field_0x02 = 1;
    slot->entries_b[3].field_0x02 = 1;
    slot->entries_b[4].field_0x02 = 1;
    if (slot->field_0x01B >= (s8)slot->entry_count_b) {
        slot->field_0x01B = (s8)(slot->entry_count_b - 1);
    }
    table = (u32*)get_menu_msg_tbl(17);
    for (i = 0, entry = slot->entries_b; i < (s8)slot->entry_count_b; entry++, i++) {
        if (table != 0) {
            entry->field_0x08 = *table;
            table++;
        }
    }
}

/* 0x802A2D68 (0x50): marks row `index` of a split-screen slot's top menu as selected. */
extern "C" void menu_split_top_row_select(u8 idx, s32 index)
{
    MenuSlot* slot = &menu_work.slot[idx];
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_b; i++) {
        slot->entries_b[i].selected = i == index;
    }
}

/* 0x802A2DB8 (0x3AC): one split-screen slot's frame: the pad copy, the open/close button, the top menu and the
 * open page's frame handler, then the slot's draw task. */
extern "C" void menu_split_update(u8 idx)
{
    MenuSlot* slot = &menu_work.slot[idx];
    _PLW* plw;
    u16 keys;
    s32 result;

    if (pad_hold_ck() != 1 && cockpit_msg_slot_update(idx) == 0) {
        plw = (_PLW*)get_move_work_adrs(2) + idx;
        menu_pad_read(slot);
        if (slot->buttons_trig & 0x400) {
            if (slot->active == 0) {
                menu_split_open(idx);
            } else if (slot->open_wait == 0 && slot->field_0x021 == 0) {
                menu_split_close(idx);
                sysSE_req(1);
            }
        }
        if (slot->active == 0) {
            return;
        }
        if (slot->open_wait != 0) {
            slot->open_wait--;
        }
        keys = slot->buttons_trig | (slot->field_0x008 & 0xF);
        switch (slot->field_0x000) {
        case 0:
            menu_split_item_row_update(idx);
            if (keys & 0x20) {
                if (slot->open_wait == 0) {
                    menu_split_close(idx);
                    sysSE_req(1);
                }
            } else if (keys & 0x10) {
                if (slot->entries_b[slot->field_0x01B].field_0x02 != 0) {
                    sysSE_req(0);
                    slot->field_0x000 = 1;
                    switch (slot->field_0x01B) {
                    case 0:
                        menu_split_item_page_open(slot);
                        break;
                    case 1:
                        quest_info_page_open(slot);
                        break;
                    case 2:
                        slot->scroll.equip_page = 0;
                        slot->scroll.equip_page_count = equip_list_page_count((EquipListWork*)plw) + 2;
                        slot->field_0x014 = 7;
                        break;
                    case 3:
                        movie_page04_split_open(slot);
                        break;
                    case 4:
                        slot->field_0x18C = 1;
                        slot->field_0x014 = 5;
                        break;
                    }
                } else {
                    sysSE_req(2);
                }
            } else {
                switch (menu_split_shortcut_ck(slot, keys)) {
                case 0:
                    if (keys & 3) {
                        slot->field_0x01B = menu_cursor_step(slot->field_0x01B, slot->entry_count_b, keys, 1, 2);
                        menu_split_top_row_select(idx, slot->field_0x01B);
                    }
                    break;
                case 4:
                    menu_split_close(idx);
                    break;
                }
            }
            break;
        case 1:
            result = 0;
            switch (slot->field_0x014) {
            case 0:
                result = menu_split_item_page_step(slot);
                break;
            case 10:
                result = quest_info_page_step(slot);
                break;
            case 7:
                result = menu_split_status_page_step(slot);
                break;
            case 4:
                result = movie_page04_split_step(slot);
                break;
            case 5:
                result = menu_split_quit_step(slot);
                break;
            }
            switch (result) {
            case 2:
                slot->field_0x000 = 0;
                menu_split_top_rows_build(slot);
                menu_split_item_row_update(idx);
                break;
            case 4:
                menu_split_close(idx);
                break;
            }
            break;
        }
    }
    if ((u32)lb_quest_work_active_ck() != 1) {
        if (cockpit_msg_slot_busy_ck(idx) != 0) {
            subTransSetPrio(6, (u32)cockpit_msg_slot_split_draw, idx, 0);
        } else if (slot->active != 0) {
            subTransSetPrio(6, (u32)menu_split_draw, idx, 0);
        }
    }
}

/* 0x802A3164 (0x2C): runs both split-screen slots' frames. */
extern "C" void menu_split_update_all(void)
{
    menu_split_update(0);
    menu_split_update(1);
}

/* 0x802A3190: the second entry array's selector, over the slot the caller passes. */
extern "C" void menu_split_row_select(MenuSlot* slot, s32 index)
{
    s32 i;

    for (i = 0; i < (s8)slot->entry_count_b; i++) {
        if (i == index) {
            slot->entries_b[i].selected = 1;
        } else {
            slot->entries_b[i].selected = 0;
        }
    }
}

/* 0x802A31D4 (0xA4): opens the item page of a split-screen slot. */
extern "C" void menu_split_item_page_open(MenuSlot* slot)
{
    u8 count = 24;

    if (slot->field_0x010 != 0) {
        count = 32;
    }
    slot->field_0x014 = 0;
    slot->field_0x1B0 = 0;
    slot->field_0x19E = 0;
    slot->entry_count_b = 4;
    slot->field_0x1A3 = 4;
    slot->field_0x1A0 = 0;
    slot->field_0x1A1 = menu_page_count(count, 4);
    slot->field_0x002 = 0;
    slot->field_0x001 = 0;
    slot->field_0x01A = 0;
    slot->field_0x1A2 = 0;
    slot->field_0x23A = 0;
    menu_split_row_select(slot, 0);
    menu_item_grid_rows_build(slot);
}

/* 0x802A3278 (0x424): the split-screen item page's frame: the grid cursor, the action list and the action's own
 * frame handler. */
extern "C" s32 menu_split_item_page_step(MenuSlot* self)
{
    s32 result = 0;
    u16 keys = (u16)(self->buttons_trig | (self->field_0x008 & 0xF));
    IdValue* entry;
    s8 index;

    self->field_0x19C = keys;
    self->field_0x19E = 0;

    switch (self->field_0x001) {
    case 0:
        if (self->field_0x23A != 0) {
            self->field_0x23A--;
        }
        if (self->field_0x1B0 != 0) {
            switch (toggle_word_step_dpad(&self->field_0x18C, keys, 4, 8)) {
            case 1:
                menu_item_pages_resort(self->worker, self->page_a, self->page_b);
                self->field_0x1B0 = 0;
                sysSE_req(0);
                break;
            case 2:
                self->field_0x1B0 = 0;
                break;
            }
            if (self->field_0x1B0 != 0) {
                break;
            }
            keys = 0;
        } else if (keys & 0x40) {
            self->field_0x1B0 = 1;
            self->field_0x18C = 0;
            sysSE_req(13);
            break;
        }

        if (keys & 0x20) {
            sysSE_req(1);
            result = 2;
        } else if (keys & 0x10) {
            if (self->entries_b[(s8)self->field_0x1A2].field_0x02 != 0) {
                self->entry_count_a = 3;
                menu_item_action_list_build(self, self->field_0x01A);
                self->field_0x001 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else {
            menu_cursor_column_step(&self->field_0x19C);
        }
        menu_split_row_select(self, (s8)self->field_0x1A2);
        break;

    case 1:
        entry = menu_slot_get(self, (s16)((s8)self->field_0x1A2 + (s8)self->field_0x1A3 * (s8)self->field_0x1A0));
        if (entry->id == 0) {
            self->field_0x001 = 0;
            break;
        }
        if (self->field_0x23A < 3) {
            self->field_0x23A++;
        }
        if (keys & 0x10) {
            if (menu_item_worker_ck(self->worker) == 0) {
                sysSE_req(2);
            } else {
                switch (self->field_0x01A) {
                case 1:
                    self->field_0x001 = 2;
                    index = (s8)(self->field_0x1A2 + self->field_0x1A3 * self->field_0x1A0);
                    self->field_0x1E6 = index;
                    menu_item_move_target_set(self, self->field_0x1E6);
                    sysSE_req(14);
                    break;
                case 0:
                    self->field_0x18C = 1;
                    self->field_0x001 = 2;
                    sysSE_req(20);
                    break;
                case 2:
                    if (menu_item_use_ck(self) == 1) {
                        self->count_0x1B6 = 1;
                        self->count_max_0x1B8 = entry->value;
                        self->field_0x001 = 2;
                        self->field_0x002 = self->count_max_0x1B8 == 1;
                        sysSE_req(0);
                    } else {
                        sysSE_req(2);
                    }
                    break;
                }
            }
        } else if (keys & 0x20) {
            self->field_0x001 = 0;
            sysSE_req(1);
        } else if (keys & 3) {
            self->field_0x01A = (s8)menu_cursor_step(self->field_0x01A, (s8&)self->entry_count_a, keys, 1, 2);
        }
        menu_item_action_list_build(self, self->field_0x01A);
        break;

    case 2:
        result = 2;
        switch (self->field_0x01A) {
        case 1:
            result = menu_item_move_step(self);
            break;
        case 0:
            result = menu_item_drop_step(self);
            break;
        case 2:
            result = menu_item_split_use_step(self);
            break;
        }
        switch (result) {
        case 1:
            self->field_0x001 = 0;
            break;
        case 2:
            sysSE_req(1);
            /* falls through */
        case 3:
            self->field_0x001 = 1;
            menu_item_action_list_build(self, self->field_0x01A);
            break;
        case 4:
            result = 4;
            break;
        }
        break;
    }
    menu_item_grid_rows_build(self);
    return result;
}

/* 0x802A369C (0xA4): the equipment status page's frame: B closes it, the d-pad pages it. */
extern "C" s32 menu_split_status_page_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 keys = slot->buttons_trig | (slot->field_0x008 & 0xF);

    if ((keys & 0x20) != 0) {
        sysSE_req(1);
        result = 2;
    } else if ((keys & 0x10) == 0 && (keys & 0xC) != 0) {
        slot->scroll.equip_page = menu_cursor_step((s8)slot->scroll.equip_page, (s8)slot->scroll.equip_page_count, keys, 4, 8);
    }
    return result;
}

/* 0x802A3740 (0xB4): the quit-quest confirmation's frame: a yes leaves the quest when no motion input is held. */
extern "C" s32 menu_split_quit_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 keys = slot->buttons_trig | (slot->field_0x008 & 0xF);

    if (menu_item_input_ok(slot) == 0) {
        return 2;
    }
    switch (toggle_word_step_dpad(&slot->field_0x18C, keys, 4, 8)) {
    case 1:
        if (Pl_motion_input_ck(0) == 0) {
            ef_move_state_dispatch(1);
        }
        result = 4;
        break;
    case 2:
        result = 2;
        break;
    }
    return result;
}

/* 0x802A37F4 (0x3DC): the split-screen menu's draw task for slot `idx`: the top menu's five rows and its button
 * guide, or the open page's own draw. */
extern "C" void menu_split_draw(u32 idx)
{
    _mh_ivec2_ pos;
    _mh_ivec2_ row;
    _SPR_DATA_ spr;
    MenuEntry* entry;
    MenuSlot* slot;
    u32 flags;
    s32 i;

    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    slot = &menu_work.slot[idx];
    switch (slot->field_0x000) {
    case 0:
        get_lsp_data(((u16*)get_menu_lsp_tbl(357))[slot->pad_no], &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(358), &pos);
        uv_pair_copy(&row, &pos);
        spr_data_copy(&spr, get_lsp_data(3845, 0));
        for (i = 0, entry = slot->entries_b; i < 5; entry++, i++) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(359), &row);
            flags = 0;
            spr.color = 0x646464FF;
            if (entry->field_0x02 != 0) {
                spr.color = 0xC3C3C3FF;
                if (entry->selected != 0) {
                    flags = 4;
                    spr.color = 0xF0F0F0FF;
                }
            }
            if (i == 0) {
                _SPR_DATA_ first = *get_lsp_data(3846, 0);

                first.color = spr.color;
                draw_font(first, (s8*)entry->field_0x08, flags, &row);
                if (slot->field_0x010 == 0) {
                    draw_font_idx(3847, (s8*)get_menu_msg(15, 0), flags, &row);
                } else {
                    draw_font_idx(3848, (s8*)get_menu_msg(15, 1), flags, &row);
                }
            } else {
                draw_font(spr, (s8*)entry->field_0x08, flags, &row);
            }
            row.y += 26;
        }
        row.y = pos.y + slot->field_0x01B * 26;
        put_menu_cursor((u16*)get_menu_lsp_tbl(360), 0, &row);
        spr_data_copy(&spr, get_lsp_data(3829, 0));
        row.x = pos.x - spr.pos.x;
        row.y = pos.y - spr.pos.y;
        spr_data_copy(&spr, get_lsp_data(3857, 0));
        row.x += spr.pos.x;
        row.y += spr.pos.y;
        if (get_arena_cfg(idx, 7) == 0) {
            if (quest_mark_flag_get(&cockpit_work[slot->field_0x011]) == 0) {
                put_button_guide(7, &row);
            } else {
                put_button_guide(8, &row);
            }
        } else {
            if (quest_mark_flag_get(&cockpit_work[slot->field_0x011]) == 0) {
                put_button_guide(10, &row);
            } else {
                put_button_guide(11, &row);
            }
        }
        break;
    case 1:
        switch (slot->field_0x014) {
        case 0:
            menu_split_item_page_draw(slot);
            break;
        case 10:
            quest_info_page_draw(slot);
            break;
        case 7:
            menu_split_status_page_draw(slot);
            break;
        case 4:
            movie_page04_split_draw(slot);
            break;
        case 5:
            menu_split_quit_draw(slot);
            break;
        }
        break;
    }
}

/* 0x802A3BD0 (0x4E0): draws the split-screen item page: the grid, the action list, the drop confirmation and the
 * count arrow the current state shows. */
extern "C" void menu_split_item_page_draw(MenuSlot* slot)
{
    s16 base_x;
    s16 base_y;
    s32 draw_grid = 1;
    u8 two_page;
    u32 flags = 0;
    u32 draw_panel = 0;
    s32 draw_arrow = 0;
    s32 confirm = -1;
    s32 draw_guide = 0;
    s8 player = slot->field_0x011;
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _mh_ivec2_ row;
    _SPR_DATA_* rec;

    if (slot->field_0x19E & 4) {
        flags |= 8;
    }
    if (slot->field_0x19E & 8) {
        flags |= 16;
    }
    two_page = menu_slot_index_is_page_b(slot, (s16)(slot->field_0x1A0 * slot->field_0x1A3));
    switch (slot->field_0x001) {
    case 0:
        if (two_page != 0) {
            flags |= 1;
        }
        if (slot->field_0x1B0 != 0) {
            confirm = 1;
            flags |= 0x200;
        } else {
            draw_guide = 1;
        }
        break;
    case 1:
        if (two_page != 0) {
            flags |= 1;
        }
        flags |= 0x200;
        draw_panel = 1;
        break;
    case 2:
        switch (slot->field_0x01A) {
        case 1:
            if (two_page != 0) {
                flags |= slot->field_0x1E7 ? 1 : 2;
            }
            flags |= 4;
            draw_panel = 2;
            break;
        case 0:
            if (two_page != 0) {
                flags |= 1;
            }
            flags |= 0x200;
            confirm = 0;
            draw_guide = 0;
            break;
        case 2:
            if (two_page != 0) {
                flags |= 1;
            }
            flags |= 0x200;
            switch (slot->field_0x002) {
            case 0:
                draw_arrow = 1;
                break;
            case 1:
            case 2:
                draw_grid = 0;
                break;
            }
            break;
        }
        break;
    }
    get_lsp_data(((u16*)get_menu_lsp_tbl(361))[slot->pad_no], &pos);
    rec = get_lsp_data(3936, 0);
    base_x = pos.x - rec->pos.x;
    base_y = pos.y - rec->pos.y;
    if (draw_panel == 2) {
        rec = get_lsp_data(4039, 0);
        at.x = base_x + rec->pos.x;
        at.y = base_y + rec->pos.y;
        menu_choice_window_draw(slot->entries_a, slot->entry_count_a, &at);
        font_flush();
    }
    if (draw_grid != 0) {
        if (slot->field_0x1B0 == 0) {
            menu_frame_page_draw(two_page == 0 ? slot->page_a : slot->page_b, slot->entries_b, slot->field_0x1A0,
                                 slot->field_0x1A1, slot->field_0x23A, flags, player);
        } else {
            menu_frame_entries_build_row(slot->worker, slot->page_a, slot->page_b, slot->field_0x1A1, flags, player);
        }
        if (draw_guide != 0) {
            rec = get_lsp_data(4032, 0);
            at.x = base_x + rec->pos.x;
            at.y = base_y + rec->pos.y;
            if (get_arena_cfg(slot->field_0x011, 7) == 0) {
                put_button_guide(4, &at);
            } else {
                put_button_guide(12, &at);
            }
        }
    }
    if (confirm >= 0) {
        rec = get_lsp_data(player == 0 ? 4106 : 4125, &at);
        draw_window_box(at.x, at.y, rec->width, rec->height, 0, 0);
        rec = get_lsp_data(4111, 0);
        row.x = at.x + rec->pos.x;
        row.y = at.y + rec->pos.y;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(371), &row);
        if (slot->field_0x18C == 0) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(372), &row);
        }
        rec = get_lsp_data(4112, 0);
        row.x = at.x + rec->pos.x;
        row.y = at.y + rec->pos.y;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(371), &row);
        if (slot->field_0x18C == 1) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(372), &row);
        }
        draw_font_idx(4108, (s8*)get_menu_msg(16, confirm), 1, &at);
        draw_font_idx(4109, (s8*)get_menu_msg(13, 0), 1, &at);
        draw_font_idx(4110, (s8*)get_menu_msg(13, 1), 1, &at);
    }
    if (draw_panel == 1) {
        font_flush();
        rec = get_lsp_data(4039, 0);
        at.x = base_x + rec->pos.x;
        at.y = base_y + rec->pos.y;
        menu_choice_window_draw(slot->entries_a, slot->entry_count_a, &at);
    }
    if (draw_arrow != 0) {
        get_lsp_data(player == 0 ? 4076 : 4122, &at);
        menu_item_count_arrow_draw(slot, &at);
    }
}

/* 0x802A40B0 (0x1D8): draws the split-screen equipment status page: the page tabs, its arrow and the page's
 * status, element or skill block. */
extern "C" void menu_split_status_page_draw(MenuSlot* slot)
{
    _PLW* plw = slot->worker;
    _mh_ivec2_ pos;
    _mh_ivec2_ block;
    _EQUIP equip[9];
    _SPR_DATA_* rec;

    get_lsp_data(((u16*)get_menu_lsp_tbl(383))[slot->pad_no], &pos);
    draw_sprite_ary(((u16**)get_menu_lsp_tbl(387))[slot->scroll.equip_page], &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(388), &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(389), slot->scroll.equip_page, slot->scroll.equip_page_count,
                 slot->field_0x008, &pos, 0);
    rec = get_lsp_data(4127, 0);
    block.x = pos.x - rec->pos.x;
    block.y = pos.y - rec->pos.y;
    Set_equip_column_arrangement(plw, 0, equip);
    if ((u32)(slot->scroll.equip_page - 2) > 1) {
        switch (slot->scroll.equip_page) {
        case 0:
            rec = get_lsp_data(4151, 0);
            block.x += rec->pos.x;
            block.y += rec->pos.y;
            draw_sprite_ary((u16*)get_menu_lsp_tbl(390), &block);
            Put_status_equip_status_block(plw, equip, 0, &block);
            break;
        case 1:
            rec = get_lsp_data(4157, 0);
            block.x += rec->pos.x;
            block.y += rec->pos.y;
            draw_sprite_ary((u16*)get_menu_lsp_tbl(391), &block);
            Put_status_equip_element_block(plw, equip, 0, &block);
            break;
        }
    } else {
        Put_status_invork_skill_sep(plw, block, slot->scroll.equip_page - 2);
    }
}

/* 0x802A4288 (0x194): draws the split-screen quit-quest confirmation: its window, title and the yes/no rows. */
extern "C" void menu_split_quit_draw(MenuSlot* slot)
{
    _mh_ivec2_ pos;
    _mh_ivec2_ row;
    _SPR_DATA_* rec;
    u16* ids;
    s32 i;

    rec = get_lsp_data(((u16*)get_menu_lsp_tbl(401))[slot->pad_no], &pos);
    draw_layout_window(pos.x, pos.y, rec->width, rec->height);
    rec = get_lsp_data(4212, 0);
    pos.x -= rec->pos.x;
    pos.y -= rec->pos.y;
    rec = get_lsp_data(4213, 0);
    pos.x += rec->pos.x;
    pos.y += rec->pos.y;
    draw_sprite_ary((u16*)get_menu_lsp_tbl(402), &pos);
    draw_font_idx(4214, (s8*)get_menu_msg(41, 5), 5, &pos);
    ids = (u16*)get_menu_lsp_tbl(403);
    for (i = 0; i < 2; i++) {
        rec = get_lsp_data(ids[i], 0);
        row.x = pos.x + rec->pos.x;
        row.y = pos.y + rec->pos.y;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(404), &row);
        draw_font_idx(2745, (s8*)get_menu_msg(13, i), 1, &row);
        if (i == slot->field_0x18C) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(405), &row);
        }
    }
}

/* 0x802A441C: puts the caller's record into its mode-5 state. */
extern "C" void menu_quit_open(MenuSlot* self)
{
    self->field_0x014 = 5;
    self->field_0x18C = 1;
}

/* 0x802A4430 (0x110): the return-to-village confirmation's frame. */
extern "C" s32 menu_quit_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 keys = slot->buttons_trig | (slot->field_0x008 & 0xF);
    u32 sound;

    if (menu_item_input_ok(slot) == 0) {
        return 2;
    }
    sound = 1;
    if (game_ready_ck() == 0) {
        if (PlayMode_ck() == 2 && system_w.field_0x8af == 0) {
            sound = 1;
        } else {
            sound = 0;
        }
    }
    switch (toggle_word_step(&slot->field_0x18C, keys, 4, 8, (u16)0xFFFE)) {
    case 1:
        ef_move_state_dispatch(1);
        if (sound == 1) {
            sysSE_req(0);
        }
        result = 4;
        break;
    case 2:
        sysSE_req(1);
        result = 2;
        break;
    }
    return result;
}

/* 0x802A4540 (0x2B4): draws the return-to-village confirmation: its window, title and the yes/no rows. */
extern "C" void menu_quit_draw(MenuSlot* slot)
{
    _mh_ivec2_ pos;
    _mh_ivec2_ row;
    _SPR_DATA_* rec;
    u16* ids;
    u32* msgs;
    s32 title;
    s32 i;
    bool selected;
    s32 text;

    if (menu_quest_state_ck() != 0) {
        rec = get_lsp_data(2736, &pos);
        draw_layout_window(pos.x, pos.y, rec->width, rec->height);
        get_lsp_data(2737, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(228), &pos);
        draw_font_idx(2738, (s8*)get_menu_msg(41, 0), 5, &pos);
        ids = (u16*)get_menu_lsp_tbl(232);
    } else {
        rec = get_lsp_data(2753, &pos);
        draw_layout_window(pos.x, pos.y, rec->width, rec->height);
        get_lsp_data(2754, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(229), &pos);
        msgs = (u32*)get_menu_msg_tbl(41);
        if (game_ready_ck() != 0) {
            title = 1;
            text = 3;
        } else if (PlayMode_ck() == 2 && system_w.field_0x8af == 0) {
            title = 12;
            text = 13;
        } else {
            title = 2;
            text = 3;
        }
        draw_font_idx(2755, (s8*)msgs[title], 5, &pos);
        draw_font_idx(2756, (s8*)msgs[text], 0, &pos);
        draw_font_idx(2758, (s8*)msgs[4], 1, &pos);
        ids = (u16*)get_menu_lsp_tbl(233);
    }
    for (i = 0; i < 2; i++) {
        rec = get_lsp_data(ids[i], 0);
        row.x = pos.x + rec->pos.x;
        row.y = pos.y + rec->pos.y;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(230), &row);
        selected = i == slot->field_0x18C;
        _SPR_DATA_ spr = *get_lsp_data(2745, 0);
        spr.color = GetMenuFontColor(1, selected, 1, 0);
        draw_font(spr, (s8*)get_menu_msg(13, i), 1, &row);
        if (selected == 1) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(231), 0, &row);
        }
    }
}

/* 0x802A47F4: the mode-6 state of the same record. */
extern "C" void menu_lobby_exit_open(MenuSlot* self)
{
    self->field_0x014 = 6;
    self->field_0x18C = 1;
    self->field_0x001 = 0;
}

/* 0x802A4810 (0x294): the lobby-exit save sequence's frame: the confirmation, the data-file create, the save
 * wait and the system-file create, then the exit. */
extern "C" s32 menu_lobby_exit_step(MenuSlot* slot)
{
    s32 result = 0;
    u16 keys = slot->buttons_trig | (slot->field_0x008 & 0xF);

    ainpc_page_hold_clear();
    if ((u32)(slot->field_0x001 - 30) > 2) {
        switch (slot->field_0x001) {
        case 0:
            switch (toggle_word_step_dpad(&slot->field_0x18C, keys, 4, 8)) {
            case 1:
                slot->field_0x18C = 0;
                slot->field_0x001 = 10;
                slot->field_0x021 = 1;
                lobby_w.active_0x008 = 1;
                NetCtrlWk::raiseRequestFlag();
                break;
            case 2:
                result = 2;
                break;
            }
            break;
        case 10:
            switch (toggle_word_step_dpad(&slot->field_0x18C, keys, 4, 8)) {
            case 1:
                if (game_data_file_create_start() == 0) {
                    slot->field_0x001 = 30;
                } else {
                    system_w.field_0x865 = 1;
                    slot->field_0x001++;
                }
                break;
            case 2:
                slot->field_0x001 = 20;
                break;
            }
            break;
        case 11:
            switch (game_save_wait()) {
            case 1:
                if (game_system_file_create_start() == 0) {
                    system_w.field_0x865 = 0;
                    slot->field_0x001 = 30;
                } else {
                    slot->field_0x001 = 12;
                }
                break;
            case -1:
                system_w.field_0x865 = 0;
                slot->field_0x001 = 31;
                break;
            case -2:
                system_w.field_0x865 = 0;
                slot->field_0x001 = 32;
                break;
            }
            break;
        case 12:
            switch (game_system_file_create_wait()) {
            case 1:
                system_w.field_0x865 = 0;
                slot->field_0x001 = 20;
                break;
            case -1:
                system_w.field_0x865 = 0;
                slot->field_0x001 = 31;
                break;
            case -2:
                system_w.field_0x865 = 0;
                slot->field_0x001 = 32;
                break;
            }
            break;
        case 20:
            system_w.field_0x7ce = slot->field_0x01A;
            ef_move_state_dispatch(1);
            result = 4;
            break;
        }
    } else if ((keys & 0x30) != 0) {
        sysSE_req(1);
        result = 2;
    }
    return result;
}

/* 0x802A4AA4 (0x254): draws the lobby-exit save sequence: the confirmation window, or the NAND error message. */
extern "C" void menu_lobby_exit_draw(MenuSlot* slot)
{
    _mh_ivec2_ pos;
    _mh_ivec2_ row;
    _SPR_DATA_* rec;
    u16* ids;
    u16 text;
    u16 title;
    s32 i;
    bool selected;
    char message[0x100];

    switch (slot->field_0x001) {
    case 0:
        title = slot->field_0x01A + 9;
        text = 4;
        break;
    case 10:
        title = 6;
        text = 7;
        break;
    case 30:
    case 31:
    case 32:
        wii_sysmsg_gen(chg_nand_err2msgcode(), message, 0);
        put_message_sys((s8*)message, 0, 0, 0);
        put_message_sys_ok_button();
        return;
    default:
        return;
    }
    rec = get_lsp_data(2764, &pos);
    draw_layout_window(pos.x, pos.y, rec->width, rec->height);
    get_lsp_data(2765, &pos);
    if (title != 0xFFFF) {
        draw_font_idx(2766, (s8*)get_menu_msg(41, title), 8, &pos);
    }
    if (text != 0xFFFF) {
        draw_font_idx(2767, (s8*)get_menu_msg(41, text), 1, &pos);
    }
    ids = (u16*)get_menu_lsp_tbl(234);
    for (i = 0; i < 2; i++) {
        rec = get_lsp_data(ids[i], 0);
        row.x = pos.x + rec->pos.x;
        row.y = pos.y + rec->pos.y;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(230), &row);
        selected = i == slot->field_0x18C;
        _SPR_DATA_ spr = *get_lsp_data(2745, 0);
        spr.color = GetMenuFontColor(1, selected, 1, 0);
        draw_font(spr, (s8*)get_menu_msg(13, i), 1, &row);
        if (selected == 1) {
            put_menu_cursor((u16*)get_menu_lsp_tbl(231), 0, &row);
        }
    }
}

/* 0x802A4CF8 (0xA0): opens the item page of the main menu slot. */
extern "C" void menu_item_page_open(MenuSlot* slot)
{
    u8 count = 24;

    if (slot->field_0x010 != 0) {
        count = 32;
    }
    slot->field_0x014 = 0;
    slot->field_0x1B0 = 0;
    slot->field_0x19E = 0;
    slot->entry_count_b = 8;
    slot->field_0x1A3 = 8;
    slot->field_0x1A0 = 0;
    slot->field_0x1A1 = menu_page_count(count, 8);
    slot->field_0x002 = 0;
    slot->field_0x001 = 0;
    slot->field_0x01A = 0;
    slot->field_0x1A2 = 0;
    menu_slot_entry_b_select(slot, 0);
    menu_item_grid_rows_build(slot);
}

/* 0x802A4D98 (0x160): fills the item grid's column rows from the page the column falls on: which cells may be
 * picked, and the colours of the cell a move started from. */
extern "C" void menu_item_grid_rows_build(MenuSlot* slot)
{
    IdValue* entry;
    MenuEntry* cell = slot->entries_b;
    IdValue* page = slot->page_a;
    s16 index = (s8)slot->field_0x1A0 * (s8)slot->field_0x1A3;
    s32 i;

    if (menu_slot_index_is_page_b(slot, index) != 0) {
        index -= 24;
        page = slot->page_b;
    }
    for (i = 0, entry = &page[index]; i < (s8)slot->field_0x1A3; i++, cell++, entry++, index++) {
        cell->field_0x0C = 0;
        if (entry->id != 0) {
            cell->field_0x02 = 1;
            if ((s32)slot->field_0x001 == 2 && slot->field_0x01A == 1) {
                if (slot->field_0x1E7 != 0) {
                    if (slot->field_0x1E6 == i + (s8)slot->field_0x1A0 * (s8)slot->field_0x1A3) {
                        cell->field_0x0C = 0xF3D73EFF;
                        cell->colour_0x10 = 0xF53737FF;
                        cell->colour_0x14 = 0xFFFFFFFF;
                    } else if (slot->field_0x1E6 >= 24 && GetItemData(entry->id)->kind_0x00 != 1) {
                        cell->field_0x02 = 0;
                    }
                } else {
                    cell->field_0x02 = 0;
                }
            }
        } else {
            cell->field_0x02 = 0;
        }
        cell->field_0x06 = index;
    }
}

/* 0x802A4EF8 (0xF4): builds the three-row action list (use, move, drop) and marks the row `index`. */
extern "C" void menu_item_action_list_build(MenuSlot* slot, s16 index)
{
    MenuEntry* entry = slot->entries_a;
    s32 i;

    for (i = 0; i < 3; i++, entry++) {
        entry->selected = 0;
        entry->field_0x02 = 0;
        entry->field_0x08 = get_menu_msg(11, i);
        entry->field_0x0C = 0;
    }
    if (menu_item_worker_ck(slot->worker) != 0) {
        slot->entries_a[1].field_0x02 = 1;
        slot->entries_a[0].field_0x02 = 1;
        if ((s8)slot->entry_count_a >= 3 && menu_item_use_ck(slot) != 0) {
            slot->entries_a[2].field_0x02 = 1;
        }
    }
    if (index <= (s8)slot->entry_count_a) {
        if (slot->field_0x001 != 2) {
            slot->entries_a[index].selected = 1;
        } else {
            slot->entries_a[index].field_0x0C = 0xE8A40FFF;
        }
    }
}

/* 0x802A4FEC (0x334): the move action's frame: picks the destination cell and swaps the two item pairs, keeping
 * the player's shortcut and selected slots on the moved items. */
extern "C" s32 menu_item_move_step(MenuSlot* slot)
{
    s32 result;
    u16 keys;
    s32 ok;
    s8 dest;
    IdValue tmp;

    if (menu_item_worker_ck(slot->worker) == 0) {
        slot->field_0x1A0 = slot->field_0x1E6 / (s8)slot->field_0x1A3;
        slot->field_0x1A2 = slot->field_0x1E6 % (s8)slot->field_0x1A3;
        menu_slot_entry_b_select(slot, (s8)slot->field_0x1A2);
        menu_item_action_list_build(slot, slot->field_0x01A);
        return 3;
    }
    result = 0;
    keys = slot->buttons_trig;
    if (slot->field_0x23A != 0) {
        slot->field_0x23A--;
    }
    if (keys & 0x10) {
        ok = 0;
        if (slot->field_0x1E7 != 0) {
            dest = slot->field_0x1A2 + slot->field_0x1A3 * slot->field_0x1A0;
            if (dest != slot->field_0x1E6) {
                ok = 1;
                if (slot->field_0x1E6 >= 24) {
                    u16 id = menu_slot_get(slot, dest)->id;

                    if (id != 0 && GetItemData(id)->kind_0x00 != 1) {
                        ok = 0;
                    }
                }
            }
        }
        if (ok != 0) {
            IdValue* a = menu_slot_get(slot, dest);
            IdValue* b = menu_slot_get(slot, slot->field_0x1E6);

            item_pair_copy(&tmp, a);
            item_pair_copy(a, b);
            item_pair_copy(b, &tmp);
            if (GameMode_ck() == 1) {
                s8 from;
                s8 to;
                _PLW* plw = slot->worker;
                s32 sel;

                if (dest >= 24) {
                    from = dest + 2;
                } else {
                    from = dest;
                }
                if (slot->field_0x1E6 >= 24) {
                    to = slot->field_0x1E6 + 2;
                } else {
                    to = slot->field_0x1E6;
                }
                if (plw->field_0x304 == from) {
                    plw->field_0x304 = to;
                } else if (plw->field_0x304 == to) {
                    plw->field_0x304 = from;
                }
                if (slot->field_0x010 != 0) {
                    sel = plw->item_slot_sel;
                    if (sel & 0x80) {
                        sel = (u16)((sel & 0x7F) + 24);
                    }
                    if ((u16)sel == slot->field_0x1E6) {
                        if (dest >= 24) {
                            plw->item_slot_sel = (dest - 24) | 0x80;
                        } else {
                            plw->item_slot_sel = dest;
                        }
                    } else if ((u16)sel == dest) {
                        if (slot->field_0x1E6 >= 24) {
                            plw->item_slot_sel = (slot->field_0x1E6 - 24) | 0x80;
                        } else {
                            plw->item_slot_sel = slot->field_0x1E6;
                        }
                    }
                }
            }
            result = 1;
            sysSE_req(15);
        } else {
            sysSE_req(2);
        }
    } else if (keys & 0x20) {
        slot->field_0x1A0 = slot->field_0x1E6 / (s8)slot->field_0x1A3;
        slot->field_0x1A2 = slot->field_0x1E6 % (s8)slot->field_0x1A3;
        result = 2;
    } else {
        menu_cursor_column_step(&slot->field_0x19C);
        menu_item_move_target_set(slot, (s16)(slot->field_0x1A2 + (s8)slot->field_0x1A3 * (s8)slot->field_0x1A0));
    }
    menu_slot_entry_b_select(slot, slot->field_0x1A2);
    menu_item_action_list_build(slot, slot->field_0x01A);
    return result;
}

/* 0x802A5320 (0x124): the drop action's frame: a yes throws the whole stack away. */
extern "C" s32 menu_item_drop_step(MenuSlot* slot)
{
    s32 result;
    u16 keys;
    IdValue* entry;

    if (menu_item_worker_ck(slot->worker) == 0) {
        return 3;
    }
    result = 0;
    keys = slot->buttons_trig;
    switch (screen_split_mode_ck() == 0 ? toggle_word_step_dpad(&slot->field_0x18C, keys, 1, 2)
                                        : toggle_word_step_dpad(&slot->field_0x18C, keys, 4, 8)) {
    case 1:
        entry = menu_slot_get(slot, (u16)(slot->field_0x1A2 + (s8)slot->field_0x1A3 * (s8)slot->field_0x1A0));
        if (slot->field_0x00F == 2) {
            userdata_item_give(lobby_world_block, entry->id, -100, 1);
        } else {
            pl_item_add(slot->worker, entry->id, -100);
        }
        sysSE_req(0);
        result = 1;
        break;
    case 2:
        result = 2;
        break;
    }
    return result;
}

/* The option list's frame handler: runs the menu's own state machine on the key word, then the item
 * list's two waits. */
extern "C" s32 menu_item_use_step(MenuSlot* self) {
    s32 result = 0;
    u16 keys = (u16)(self->buttons_trig | (self->field_0x008 & 0xF));
    u8 mode;
    u32 ok;
    _PLW* plw;

    if (menu_item_worker_ck(self->worker) == 0 && self->field_0x002 != 3) {
        return 3;
    }

    mode = menu_list_mode_get(self);
    switch (mode) {
    case 0:
    case 3:
        nw4r::db::Panic("menu_item.cpp", 396, "NW4R:Failed assertion 0");
        return 4;
    default:
        break;
    }

    switch (self->field_0x002) {
    case 0:
        switch (menu_cursor_page_move(&self->count_0x1B6, self->count_max_0x1B8, self->buttons_trig, self->field_0x008,
                                      &self->move_result_0x238)) {
        case 1:
            if (mode == 1) {
                sysSE_req(0);
                self->field_0x002 = 1;
            } else if (mode == 2) {
                sysSE_req(0);
                self->field_0x002 = 2;
            }
            break;
        case 2:
            result = 2;
            break;
        default:
            break;
        }
        break;

    case 1:
        if (keys & 0x10) {
            ok = 0;
            if (menu_list_fill(self, -1) != 0) {
                ok = 1;
            }
            if (ok == 1) {
                self->field_0x002 = 2;
            } else {
                sysSE_req(2);
            }
        } else if (keys & 0x20) {
            if (self->count_max_0x1B8 != 1) {
                self->field_0x002 = 0;
                sysSE_req(1);
            } else {
                result = 2;
            }
        } else {
            if (menu_list_fill(self, -1) != 0) {
                if (keys & 3) {
                    self->item_cursor = (s8)menu_cursor_step(self->item_cursor, self->item_count, keys, 1, 2);
                }
            } else {
                self->item_cursor = 0;
                result = 2;
            }
        }
        break;

    case 2:
        plw = self->worker;
        if (mode == 1) {
            plw->field_0x655 = self->items_0x1BE[self->item_cursor];
        } else if (mode == 2) {
            plw->field_0x655 = 0x80;
        }
        if (menu_list_names_set(self, plw->field_0x655) == 0) {
            result = 2;
            break;
        }
        plw->field_0x650 = menu_slot_get(self, (s16)((s8)self->field_0x1A2 + (s8)self->field_0x1A3 * (s8)self->field_0x1A0))->id;
        plw->field_0x652 = self->count_0x1B6;
        plw->pad_0x654[0] = 1;
        self->panel_on = 0;
        if (mode == 1) {
            if (get_move_work_adrs(2) != 0) {
                self->item_id_0x1B2 = plw->field_0x650;
                self->field_0x1B4 = plw->field_0x652;
            }
        } else {
            self->item_id_0x1B2 = 0;
            self->field_0x1B4 = 0;
        }
        self->field_0x002 = 3;
        break;

    case 3:
        if (fn_8027D738(self->worker) != 0) {
            if (mode == 1) {
                self->panel_on = 1;
            }
            result = 4;
            sysSE_req(0);
        } else {
            result = 1;
            sysSE_req(2);
        }
        break;

    default:
        break;
    }

    return result;
}

/* The pick list's frame handler: advances the grid cursor, then opens the entry the cursor sits on. */
extern "C" s32 menu_item_split_use_step(MenuSlot* self) {
    s32 result = 0;
    u8 mode;
    _PLW* plw;
    MENU_MOVE_WORK* move_work;

    if (menu_item_worker_ck(self->worker) == 0 && self->field_0x002 < 2) {
        return 3;
    }

    mode = menu_list_mode_get(self);
    if (mode != 3) {
        return 4;
    }

    switch (self->field_0x002) {
    case 0:
        switch (menu_cursor_page_move(&self->count_0x1B6, self->count_max_0x1B8, self->buttons_trig, self->field_0x008,
                                      &self->move_result_0x238)) {
        case 1:
            self->field_0x002 = 1;
            sysSE_req(0);
            break;
        case 2:
            result = 2;
            break;
        default:
            break;
        }
        break;

    case 1: {
        s16 cell = (s16)((s8)self->field_0x1A2 + (s8)self->field_0x1A3 * (s8)self->field_0x1A0);
        plw = self->worker;
        plw->field_0x655 = self->items_0x1BE[0];
        plw->field_0x650 = menu_slot_get(self, cell)->id;
        plw->field_0x652 = self->count_0x1B6;
        plw->pad_0x654[0] = 1;
        self->panel_on = 0;
        move_work = (MENU_MOVE_WORK*)get_move_work_adrs(2);
        if (move_work != 0) {
            strcpy(self->name_0x1D2, (char*)move_work[plw->field_0x655].name_0x5CA);
            self->item_id_0x1B2 = plw->field_0x650;
            self->field_0x1B4 = plw->field_0x652;
        } else {
            nw4r::db::Panic("menu_item.cpp", 579, "NW4R:Failed assertion 0");
            result = 1;
        }
        self->field_0x002 = 2;
        break;
    }

    case 2:
        if (fn_8027D738(self->worker) != 0) {
            if (mode == 3) {
                self->panel_on = 1;
            }
            result = 4;
            sysSE_req(0);
        } else {
            result = 1;
            sysSE_req(1);
        }
        break;

    default:
        break;
    }

    return result;
}

/* The item list's own frame handler: the per-state cursor/key machine, then the item walk that
 * commits the selection. */
extern "C" s32 menu_item_page_step(MenuSlot* self) {
    MenuSlot* unused_self = self;
    s32 result = 0;
    u16 keys = (u16)(self->buttons_trig | (self->field_0x008 & 0xF));
    IdValue* entry;
    u8 mode;
    s32 pick;
    s8 index;

    self->field_0x19C = keys;
    self->field_0x19E = 0;

    switch (self->field_0x001) {
    case 0:
        if (self->field_0x23A != 0) {
            self->field_0x23A--;
        }
        if (self->field_0x1B0 != 0) {
            switch (toggle_word_step_dpad(&self->field_0x18C, keys, 1, 2)) {
            case 1:
                if (self->field_0x00F == 2) {
                    item_pages_sort(self->page_a, self->page_b);
                } else {
                    menu_item_pages_resort(self->worker, self->page_a, self->page_b);
                }
                self->field_0x1B0 = 0;
                sysSE_req(0);
                break;
            case 2:
                self->field_0x1B0 = 0;
                break;
            default:
                break;
            }
            if (self->field_0x1B0 != 0) {
                break;
            }
            keys = 0;
        } else if (keys & 0x40) {
            self->field_0x1B0 = 1;
            self->field_0x18C = 0;
            sysSE_req(13);
            break;
        }

        if (keys & 0x10) {
            if (self->entries_b[(s8)self->field_0x1A2].field_0x02 != 0) {
                menu_list_count_update(self);
                menu_item_action_list_build(self, self->field_0x01A);
                self->field_0x001 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (keys & 0x20) {
            result = 2;
            sysSE_req(1);
        } else {
            menu_cursor_column_step(&self->field_0x19C);
            if (keys & 0x8000) {
                if (self->flag_0x1B1 == 0) {
                    sysSE_req(1);
                } else {
                    sysSE_req(5);
                }
                self->flag_0x1B1 = self->flag_0x1B1 ^ 1;
            }
        }
        menu_slot_entry_b_select(self, (s8)self->field_0x1A2);
        break;

    case 1:
        entry = menu_slot_get(self, (s16)((s8)self->field_0x1A2 + (s8)self->field_0x1A3 * (s8)self->field_0x1A0));
        if (entry->id == 0) {
            self->field_0x001 = 0;
            break;
        }
        if (self->field_0x23A < 3) {
            self->field_0x23A++;
        }
        if (keys & 0x10) {
            if (menu_item_worker_ck(self->worker) == 0) {
                sysSE_req(2);
            } else {
                switch (self->field_0x01A) {
                case 1:
                    self->field_0x001 = 2;
                    index = (s8)(self->field_0x1A2 + self->field_0x1A3 * self->field_0x1A0);
                    self->field_0x1E6 = index;
                    menu_item_move_target_set(self, self->field_0x1E6);
                    sysSE_req(14);
                    break;
                case 0:
                    self->field_0x18C = 1;
                    self->field_0x001 = 2;
                    sysSE_req(20);
                    break;
                case 2:
                    if (menu_item_use_ck(self) == 1) {
                        mode = menu_list_mode_get(self);
                        self->item_cursor = 0;
                        self->count_0x1B6 = 1;
                        self->count_max_0x1B8 = entry->value;
                        if (mode == 2) {
                            s16 value = ai_npc_item_stock_get(entry->id);
                            if (value < self->count_max_0x1B8) {
                                self->count_max_0x1B8 = value;
                            }
                        }
                        self->field_0x001 = 2;
                        if (self->count_max_0x1B8 != 1) {
                            self->field_0x002 = 0;
                        } else if (mode == 1) {
                            self->field_0x002 = 1;
                        } else {
                            self->field_0x002 = 0;
                        }
                        sysSE_req(0);
                    } else {
                        sysSE_req(2);
                    }
                    break;
                default:
                    break;
                }
            }
        } else if (keys & 0x20) {
            self->field_0x001 = 0;
            sysSE_req(1);
        } else if (keys & 3) {
            self->field_0x01A = (s8)menu_cursor_step(self->field_0x01A, (s8&)self->entry_count_a, keys, 1, 2);
        }
        menu_item_action_list_build(self, self->field_0x01A);
        break;

    case 2:
        pick = 2;
        switch (self->field_0x01A) {
        case 1:
            pick = menu_item_move_step(self);
            break;
        case 0:
            pick = menu_item_drop_step(self);
            break;
        case 2:
            pick = menu_item_use_step(self);
            break;
        default:
            break;
        }
        switch (pick) {
        case 1:
            self->field_0x001 = 0;
            break;
        case 2:
            sysSE_req(1);
            /* falls through */
        case 3:
            self->field_0x001 = 1;
            menu_item_action_list_build(self, self->field_0x01A);
            break;
        case 4:
            result = 4;
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    menu_item_grid_rows_build(unused_self);
    return result;
}

/* Draws the whole item list: the two pages' grid, the panel lines the current phase selects, and the
 * per-entry label/icon row with the cursor. */
extern "C" void menu_item_page_draw(MenuSlot* self) {
    u32 flags = 0;
    s32 draw_panel = 0;
    s32 draw_list = 0;
    s32 draw_arrow = 0;
    s32 draw_ok = 0;
    s32 line_kind = 0;
    u32 span = 0xFF;
    u32 two_page;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    IdValue* entry;
    MENU_MOVE_WORK* move_work;
    u8 item;
    u32 length;
    s32 i;

    if (self->field_0x19E & 4) {
        flags |= 8;
    }
    if (self->field_0x19E & 8) {
        flags |= 16;
    }
    two_page = (u8)menu_slot_index_is_page_b(self, (s16)((s8)self->field_0x1A0 * (s8)self->field_0x1A3));

    switch (self->field_0x001) {
    case 0:
        if (two_page != 0) {
            flags |= 1;
        }
        flags |= 0x80;
        if (self->flag_0x1B1 == 0) {
            flags |= 0x40;
        }
        if (self->field_0x1B0 != 0) {
            draw_ok = 1;
        }
        break;

    case 1:
        if (two_page != 0) {
            flags |= 1;
        }
        flags |= 0x200;
        draw_panel = 3;
        line_kind = 2;
        span = (u8)self->field_0x01A;
        break;

    case 2:
        switch (self->field_0x01A) {
        case 1:
            if (two_page != 0) {
                flags |= self->field_0x1E7 ? 1 : 2;
            }
            flags |= 4;
            draw_panel = 3;
            line_kind = 2;
            span = 3;
            if (self->field_0x1E6 >= 24) {
                break;
            }
            if (two_page == 0) {
                break;
            }
            entry = menu_slot_get(self, self->field_0x1E6);
            if (entry->id == 0) {
                break;
            }
            if (GetItemData(entry->id)->kind_0x00 == 1) {
                break;
            }
            line_kind = 3;
            break;

        case 0:
            if (two_page != 0) {
                flags |= 1;
            }
            flags |= 0x200;
            draw_ok = 1;
            line_kind = 2;
            span = 4;
            break;

        case 2:
            if (two_page != 0) {
                flags |= 1;
            }
            flags |= 0x200;
            switch (self->field_0x002) {
            case 0:
                draw_arrow = 1;
                line_kind = 2;
                span = 5;
                break;
            case 1:
                if (self->item_count != 0) {
                    draw_list = 1;
                }
                line_kind = 2;
                span = 6;
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    if (self->field_0x1B0 == 0) {
        menu_frame_draw_page(two_page == 0 ? self->page_a : self->page_b, self->entries_b,
                    (s8)(self->field_0x1A0 + 1), self->field_0x1A1, (u16)self->field_0x23A, flags);
    } else {
        menu_frame_entries_build(self->page_a, self->page_b, self->field_0x1A1, 628, 0);
    }

    if (line_kind != 0 && (u8)span != 0xFF) {
        switch (line_kind) {
        case 2:
            get_lsp_data(732, &pos);
            draw_window_frame_preset(pos.x, pos.y, 0);
            font_set_size(16, 16);
            font_print_ex((s16)(pos.x + 24), (s16)(pos.y + 16), 0, (s8*)get_menu_msg(37, (u8)span));
            break;
        case 3:
            get_lsp_data(732, &pos);
            draw_window_frame_preset(pos.x, pos.y, 0);
            font_set_size(16, 16);
            font_print_ex((s16)(pos.x + 24), (s16)(pos.y + 16), 0, (s8*)get_menu_msg(37, (u8)span));
            font_print_ex((s16)(pos.x + 24), (s16)(pos.y + 32), 2, (s8*)get_menu_msg(37, 8));
            break;
        }
    }
    font_flush();

    if (draw_arrow != 0) {
        get_lsp_data(803, &pos);
        menu_item_count_arrow_draw(self, &pos);
    }

    if (draw_list != 0) {
        get_lsp_data(748, &pos);
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(48), (u16)((self->item_count << 1) - 2), &pos);
        pos.x += 12;
        pos.y += 17;
        move_work = (MENU_MOVE_WORK*)get_move_work_adrs(2);
        for (i = 0; i < self->item_count; i++) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(49), &pos);
            spr_data_copy(&spr, get_lsp_data(772, 0));
            spr.color = 0xC3C3C3FF;
            length = 0;
            if (i == self->item_cursor) {
                put_menu_cursor((u16*)get_menu_lsp_tbl(50), 0, &pos);
                spr.color = 0xF0F0F0FF;
                length = 4;
            }
            item = self->items_0x1BE[i];
            if (self->field_0x00F == 2) {
                MENU_OPTION_REC* option = &((MENU_OPTION_REC*)lobby_hunter_cards)[(s8)item];
                if (get_option_cfg(12) == 0) {
                    draw_font_align(&spr, option->value_0x0D, length, &pos);
                } else {
                    draw_font(spr, option->label_0x03, length, &pos);
                }
            } else {
                MENU_MOVE_WORK* work = &move_work[(s8)item];
                if (get_option_cfg(12) == 0) {
                    draw_font_align(&spr, work->name_0x5CA, length, &pos);
                } else {
                    draw_font(spr, work->value_0x5DB, length, &pos);
                }
            }
            pos.y += 26;
        }
    }

    if (draw_ok != 0) {
        get_lsp_data(741, &pos);
        menu_select_window_draw((u8)self->field_0x18C, 2, (s8*)get_menu_msg_tbl(13), &pos, 0);
    }

    if (draw_panel != 0) {
        get_lsp_data(742, &pos);
        menu_choice_window_draw(&self->entries_a, (u8)self->entry_count_a, &pos);
    }
}

/* The count arrows' sprite rows `put_page_count` draws, 0xFFFF-terminated. `.data` 0x805CDFF0 (0x1C B). */
u16 item_count_arrow_rows[14] = {
    0x032D, 0x032E, 0x032F, 0x0330, 0x032C, 0x0324, 0x0325, 0x0326, 0x0327, 0x0328, 0x0329, 0x032A, 0x032B, 0xFFFF,
};

/* The cursor arrow: hands the player work record's cursor state to the arrow-table drawer. */
extern "C" void menu_item_count_arrow_draw(MenuSlot* self, const _mh_ivec2_* pos) {
    put_page_count(item_count_arrow_rows, self->count_0x1B6, self->count_max_0x1B8, self->move_result_0x238, pos);
}

/* The player work record's item-list test, through the Pl unit's own entry point. */
extern "C" s32 menu_item_worker_ck(_PLW* plw) {
    return pl_weapon_drawn_ck(plw);
}

/* Re-tests the entry the cursor sits on and records whether it may be picked. */
extern "C" u32 menu_item_move_target_set(MenuSlot* self, s16 index) {
    if (menu_item_slot_accepts(self, menu_slot_get(self, self->field_0x1E6)->id, index) != 0) {
        self->field_0x1E7 = 1;
        return 1;
    }
    self->field_0x1E7 = 0;
    return 0;
}

/* Whether the entry the cursor sits on may be picked in the current menu kind. */
extern "C" u32 menu_item_use_ck(MenuSlot* self) {
    u16 item_id;

    if (menu_page_open_ck(0, self->field_0x011) == 0) {
        nw4r::db::Panic("menu_item.cpp", 1175, "NW4R:Failed assertion 0");
        return 0;
    }

    item_id = menu_slot_get(self, (s16)((s8)self->field_0x1A2 + (s8)self->field_0x1A3 * (s8)self->field_0x1A0))->id;

    switch ((u8)menu_list_mode_get(self)) {
    default:
        return 0;

    case 1:
        if (GetItemData(item_id)->level_0x01 >= 3) {
            return 0;
        }
        if (menu_list_fill(self, -1) == 0) {
            return 0;
        }
        break;

    case 3:
        if (GetItemData(item_id)->level_0x01 >= 3) {
            return 0;
        }
        if (menu_list_fill(self, (s8)self->field_0x011) == 0) {
            return 0;
        }
        break;

    case 2:
        if (quest_move_state_valid_ck() == 0) {
            return 0;
        }
        if ((s16)ai_npc_item_stock_get(item_id) == 0) {
            return 0;
        }
        break;
    }

    return 1;
}
