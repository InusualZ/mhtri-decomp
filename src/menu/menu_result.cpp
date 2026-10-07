/*
 * menu/menu_result.cpp - the quest-result screen: the state machine over `get_qResult_work()`'s record, the message
 *   table `q_result_msg_adrs` (by id and language), the reward, item, rank, size-record and unlock pages, and the
 *   item grids the player sorts the quest's spoils on, then the multiplayer result's box band (`multi_box_*`, the box
 *   cursor, grid credit and phase latch).  C++ (the bodies call mangled callees such as
 *   `get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3`; the two box helpers carry the original manglings).
 * RANGE. .text 0x80396948-0x8039E5CC (93 functions); extab 0x80018444-0x80018684, extabindex 0x800384CC-0x8003882C,
 *   .data 0x805F1500-0x805F1704 (the sprite-id and unlock tables and three switch tables), .bss 0x806C5528-0x806C5558
 *   (`q_result_msg_adrs`), .sdata 0x80793488-0x80793520, .sbss 0x80794C08-0x80794C10 (the box band's one byte pair),
 *   .sdata2 0x8079C300-0x8079C330.  The `.data` referrer runs break below 0x80396BBC.  The box band 0x8039D278-0x8039E5CC
 *   (11 functions) joins this TU: it takes this screen's `QResultScreen`, calls `q_result_phase_enter`/`_is_2`/`_is_3`/
 *   `_apply` and `q_result_draw_task`, walks the same +0x33DC array of 0x18-byte records as `q_result_phase_enter`, owns
 *   no `.data`, `.rodata`, `.sdata` or `.sdata2` word, and its extab/extabindex records continue this run without a gap;
 *   the right edge is where `enemy/em029_prog.cpp` opens its own pool and tables.
 * FLAGS. `cflags_menu` (configure.py); `#pragma pool_data off` (retail reaches every table by its own `lis`/`addi`:
 *   `q_result_grid_draw` 90.3 -> 91.5, `q_result_reward_page_draw` 96.2 -> 98.3).
 * NAMES. Module `menu`: its tables sit between `menu_note.cpp` (0x805E91F8) and `menu_placeinfo.cpp` (0x80604780) in
 *   `.data`, and every callee is the menu library's.  No `__FILE__` string reaches the range and the dump answers `zz_`,
 *   so the file name and every function and table name (the `q_result_*` scheme, the `QResult*` records) are GUESSes
 *   from the bodies; `q_result_msg_adrs` is the runtime dump's own name.
 *   GUESS: `q_result_work_clear`,
 *   GUESS: `q_result_hunt_recs_build`, `q_result_hunt_events_raise`, `q_result_file_ready`,
 *   GUESS: `q_result_ready_ck`, `q_result_sub_screen_ready`, `q_result_swap_counter_get`,
 *   GUESS: `q_result_unlock_next`, `q_result_phase_is_2`, `q_result_phase_is_3`, `q_result_phase_next`,
 *   GUESS: `q_result_item_page_init`, `q_result_init_flag_set`, `q_result_swap_start`, `q_result_phase_init`,
 *   GUESS: `q_result_task`, `q_result_draw_task`, `q_result_phase_apply`, `q_result_cursor_anim_step`,
 *   GUESS: `q_result_items_left_ck`, `q_result_delivery_left_ck`, `q_result_sell_total_get`,
 *   GUESS: `q_result_items_sell`, `q_result_grid_cursor_step`, `q_result_grid_cursor_index`,
 *   GUESS: `q_result_grid_entry`, `q_result_equip_cells_fill`, `q_result_delivered_at`,
 *   GUESS: `q_result_delivered_add`, `q_result_equip_cell_get`, `q_result_list_cursor_index`,
 *   GUESS: `q_result_list_entry`, `q_result_equip_sel_count`, `q_result_equip_sel_toggle`,
 *   GUESS: `q_result_equip_take`, `q_result_row_init`, `q_result_phase_input`, `q_result_timer_draw`,
 *   GUESS: `q_result_header_draw`, `q_result_draw_icon_value`, `q_result_msg_frame_rect`,
 *   GUESS: `q_result_font_print_row`, `q_result_font_print_a`, `q_result_font_print_b`,
 *   GUESS: `q_result_font_print_line`, `q_result_item_info_draw`, `q_result_item_counts_draw`,
 *   GUESS: `q_result_equip_info_draw`, `q_result_equip_detail_draw`, `q_result_box_list_draw`,
 *   GUESS: `q_result_info_panel_draw`, `q_result_tab_draw`, `q_result_equip_tab_draw`,
 *   GUESS: `q_result_cell_frame_draw`, `q_result_cell_marks_draw`, `q_result_item_cell_draw`,
 *   GUESS: `q_result_equip_cell_draw`, `q_result_grid_draw`, `q_result_page_set`, `q_result_time_page_draw`,
 *   GUESS: `q_result_page_step_a`, `q_result_page_step_b`, `q_result_reward_page_draw`,
 *   GUESS: `q_result_points_page_draw`, `q_result_size_row_draw`, `q_result_size_page_draw`,
 *   GUESS: `q_result_unlock_page_draw`, `q_result_draw`, `q_result_noop`, `q_result_box_count_get`,
 *   GUESS: `q_result_box_fit_ck`, `q_result_box_take`, `q_result_owned_count_get`, `q_result_box_slot_set`,
 *   GUESS: `q_result_box_record_init`, `q_result_phase_enter`
 *   GUESS (the box band, from each body and its callers): `multi_box_phase_apply`, `multi_box_cursor_index`,
 *   GUESS: `multi_box_grid_clear`, `multi_box_cursor_clamp`, `multi_box_grid_step`, `multi_box_phase_ck`,
 *   GUESS: `multi_box_phase_step`, `multi_box_result_step`, `multi_box_phase_input`; `multi_box_rem_exist_ck` and `multi_box_cursor_item_get` carry the dump's manglings
 * RESIDUALS. Partial rows, all register-allocation or
 *   scheduling residue: `q_result_unlock_next` (the target keeps one more value live and saves r31),
 *   `q_result_equip_detail_draw` (retail tests the kind with range compares, our `switch` with a compare tree),
 *   `q_result_hunt_recs_build`, `q_result_box_list_draw`, `q_result_equip_sel_count`, `q_result_phase_enter`,
 *   `q_result_owned_count_get` (`userdata_gunner_ck`/`item_slots_count_sum` are unsigned in retail's view),
 *   `q_result_sub_screen_ready` (MWCC if-converts the second `return (B == 1)`).
 *   Every row written.
 *   Partial rows of the box band:
 *  - `multi_box_rem_exist_ck`: 2 instructions short, the target keeps a dead loop counter;
 *  - `multi_box_phase_ck`: the ready mask in r31 against retail's r30 (playbook 22);
 *  - `multi_box_grid_clear`: `items`/`i` mirrored and a `mullw` operand order (both spellings measured).
 *  - `multi_box_phase_input`: phase 1's "any record still choosing" loop keeps its flag in r3 and the pointer in r4
 *    where ours swaps them, and the last save step computes `Psw[box_player]` with the `mulli` ahead of the base
 *    (index, inline-helper and separate-pointer spellings measured lower or equal).
 *   flipcheck: `.text`, extab and extabindex differ (the partial rows above); `.sbss` emits `multi_box_save_keep`'s 2 bytes
 *   of the claimed 0x8 (the body reads only bytes 0 and 1).
 * SHAPES. A two-way test on a `u8` mode is a `switch` in retail (`cmpwi`, the default body first); a mode tested
 *   for `<= 2` then `== 3` is `case 0: case 1: case 2:` / `case 3:`; the `GetMenuFontColor` flags are `bool` locals;
 *   the per-frame `frame` arguments are `u8` locals (the `u16` parameter then takes a `clrlwi`);
 *   `multi_box_phase_ck` declares the mask, index and count at function scope and leaves its loop with a `break` (a
 *   second constant return makes MWCC if-convert the mask test into a branchless `srwi`).
 */

#pragma pool_data off

#include "types.h"
#include "menu/menu_result.h"

#include "Runtime.PPCEABI.H/memset.h"
#include "hud/layout.h"
#include "font/flfnt.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "menu/get_pop_dat_ptr.h"
#include "fn_8004CAD8.h"
#include "fn_80047398.h"
#include "fn_80047398/lobby_world_block.h"
#include "fn_80056F24.h"
#include "sound/fn_800D7F54.h"
#include "ef/get_move_work_adrs.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/em_pop.h"
#include "quest/quest_entry.h"
#include "quest/quest_types.h"
#include "mh3_pad/system_w.h"
#include "pad_connect.h"
#include "mh3_pad/Screen_w.h"
#include "mh3_pad/Psw.h"
#include "mh3_pad/option_w.h"
#include "menu/menu_plsearch.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "hud/cockpit.h"
#include "menu/menu_row.h"
#include "userdata_item.h"
#include "lobby/lb_quest_board.h"
#include "ef/eft_res.h"
#include "Pl/pl_skill.h"
#include "lobby/userdata_award_bit_set.h"
#include "quest/arenatask.h"
#include "lobby/equip_sell_price_get.h"
#include "Pl/pl_act.h"
#include "Pl/fn_8027D684.h"
#include "menu/menu_sysmsg.h"
#include "hud/net_char_sync.h"
#include "ef/monster_size_value_get.h"
#include "menu/menu_infomation.h"
#include "MSL_C/alloc.h"


/* One 4-byte record of the menu row table `menu_row_table_get` returns, as the size rows read it (`menu/menu_item_page.h`
 * names the same record `MenuRowRec`, but that header redefines `_mh_ivec2_` and cannot sit beside `hud/layout.h`). */
typedef struct QResultMenuRow {
    /* +0x0 */ u8 kind;
    /* +0x1 */ u8 mode;               /* 1 marks a monster whose sizes are recorded */
    /* +0x2 */ u8 pad_0x2[0x2];
} QResultMenuRow; /* size: 0x4 */

/* One unlock-flag entry: the bit (1..63) of `unlock_bits` and the notice text it shows. */
typedef struct QResultUnlockBit {
    /* +0x0 */ u8 bit;
    /* +0x1 */ u8 text;
} QResultUnlockBit; /* size: 0x2 */

/* One event-flag entry: a non-zero key, the event id (9000 + the bit of `unlock_events`) and the notice text. */
typedef struct QResultUnlockEvent {
    /* +0x0 */ u16 key;
    /* +0x2 */ u16 event;
    /* +0x4 */ u8 text;
    /* +0x5 */ u8 pad_0x5;
} QResultUnlockEvent; /* size: 0x6 */

/* One hunted-monster event: the monster index (0 ends the table) and the event the hunt raises. */
typedef struct QResultHuntEvent {
    /* +0x0 */ u8 monster;
    /* +0x1 */ u8 pad_0x1[0x2];
    /* +0x3 */ u8 event;
} QResultHuntEvent; /* size: 0x4 */

/* One line of the arena time page: the anchor, the box size, the colour and the font flags. */
typedef struct QResultTextBox {
    /* +0x0 */ _mh_ivec2_ pos;
    /* +0x4 */ s16 width;
    /* +0x6 */ s16 height;
    /* +0x8 */ u32 color;
    /* +0xC */ u32 flags;
} QResultTextBox; /* size: 0x10 */

QResultUnlockBit q_result_unlock_bit_tbl[32] = {
    {0x01, 0x01}, {0x02, 0x02}, {0x03, 0x03}, {0x04, 0x04}, {0x07, 0x05}, {0x08, 0x06}, {0x09, 0x07}, {0x0A, 0x08},
    {0x0B, 0x09}, {0x0C, 0x0A}, {0x0D, 0x0B}, {0x0E, 0x0C}, {0x0F, 0x0D}, {0x10, 0x0E}, {0x11, 0x0F}, {0x12, 0x10},
    {0x13, 0x11}, {0x14, 0x12}, {0x15, 0x13}, {0x16, 0x14}, {0x17, 0x15}, {0x18, 0x16}, {0x19, 0x17}, {0x1A, 0x18},
    {0x1B, 0x19}, {0x1C, 0x1A}, {0x22, 0x24}, {0x23, 0x1B}, {0x24, 0x1C}, {0x25, 0x1D}, {0x26, 0x1E}, {0x00, 0x00},
};
QResultHuntEvent q_result_hunt_event_tbl[5] = {
    {0x03, {0x01, 0x1B}, 0x07}, {0x08, {0x01, 0x1C}, 0x08}, {0x12, {0x01, 0x1D}, 0x09}, {0x09, {0x01, 0x1E}, 0x0A},
    {0x00, {0x00, 0x00}, 0x00},
};
QResultUnlockEvent q_result_unlock_event_tbl[5] = {
    {0x0404, 0x232B, 0x1F, 0x00}, {0x040E, 0x232C, 0x20, 0x00}, {0x0412, 0x232D, 0x21, 0x00},
    {0x041A, 0x232E, 0x22, 0x00}, {0x0000, 0x0000, 0x00, 0x00},
};
u16 q_result_cmd_frame_ids[6] = {0x11E4, 0x11E5, 0x11E6, 0x11E7, 0xFFFF, 0x0000};
u16 q_result_cmd_cursor_ids[6] = {0x11ED, 0x11EA, 0x11EB, 0x11EC, 0xFFFF, 0x0000};
u16 q_result_reward_row_lsp[6] = {0x1246, 0x1247, 0x1248, 0x124D, 0x124E, 0x0000};
u16 q_result_zenny_row_lsp[6] = {0x1246, 0x1247, 0x1248, 0x1249, 0x124A, 0x0000};
u16 q_result_item_row_lsp[10] = {0x1246, 0x1247, 0x1248, 0x1249, 0x124A, 0x124B, 0x124C, 0x124D, 0x124E, 0x0000};
u16 q_result_header_ids[6] = {0x1284, 0x1283, 0x127D, 0x1280, 0xFFFF, 0x0000};
u16 q_result_grid_row_lsp[6] = {0x1214, 0x1215, 0x1216, 0x1217, 0x121A, 0x0000};
u16 q_result_kept_row_lsp[6] = {0x1214, 0x1215, 0x1216, 0x1217, 0x1218, 0x1219};
u16 q_result_equip_row_lsp[6] = {0x1227, 0x1228, 0x1229, 0x122A, 0x122B, 0x0000};
u16 q_result_equip_tab_ids[6] = {0x1221, 0x1223, 0x1224, 0x1225, 0x1226, 0xFFFF};
u16 q_result_item_info_ids[10] = {0x11FA, 0x11FB, 0x11FC, 0x11FD, 0x1207, 0x11FE, 0xFFFF, 0x0000, 0x0000, 0x0000};
u16 q_result_equip_info_ids[8] = {0x11FA, 0x11FB, 0x11FC, 0x11FD, 0x1209, 0x11FE, 0x120A, 0xFFFF};
u16 q_result_equip_cursor_ids[6] = {0x007E, 0x007B, 0x007C, 0x007D, 0xFFFF, 0x0000};
u16 q_result_page_arrow_ids[10] = {0x123C, 0x123D, 0x123E, 0x123F, 0x1243, 0x1241, 0x1240, 0x1244, 0x1243, 0xFFFF};
u16 q_result_size_row_ids[8] = {0x126E, 0x126F, 0x126A, 0x126B, 0x126C, 0xFFFF, 0x0000, 0x0000};
QResultTextBox q_result_time_rows[4] = {
    {{0x008C, 0x0010}, 0x0014, 0x0014, 0xBC6056FF, 0x00000005},
    {{0x0026, 0x002E}, 0x0012, 0x0012, 0xB4965AFF, 0x00000000},
    {{0x0026, 0x004C}, 0x0012, 0x0012, 0xB4965AFF, 0x00000000},
    {{0x0026, 0x006A}, 0x0012, 0x0012, 0xB4965AFF, 0x00000000},
};
QResultTextBox q_result_time_total_row = {{0x001E, 0x0018}, 0x0012, 0x0012, 0xB4965AFF, 0x00000000};

u16 q_result_cmd_frame_anim[4] = {0x11D6, 0x11D7, 0x11D8, 0xFFFF};
u16 q_result_cmd_rows_4[4] = {0x11DD, 0x11DE, 0x11DF, 0x0000};
u16 q_result_cmd_rows_2[2] = {0x11E0, 0x11E1};
u16 q_result_cmd_rows_1[1] = {0x11E2};
u16 q_result_hunt_row_lsp[4] = {0x1246, 0x1247, 0x124E, 0x0000};
u16 q_result_panel_anim_a[2] = {0x11F4, 0xFFFF};
u16 q_result_panel_anim_b[3] = {0x11F5, 0x11F6, 0xFFFF};
u16 q_result_tab_ids_a[3] = {0x120E, 0x120F, 0xFFFF};
u16 q_result_tab_ids_b[4] = {0x120E, 0x1210, 0x1211, 0xFFFF};
u16 q_result_tab_ids_c[4] = {0x1212, 0x1213, 0xFFFF, 0x0000};
u16 q_result_tab_anim[4] = {0x120C, 0x120D, 0xFFFF, 0x0000};
u16 q_result_equip_tab_frame[4] = {0x121F, 0x1220, 0xFFFF, 0x0000};
u16 q_result_equip_tab_label[4] = {0x0078, 0x0079, 0x007A, 0xFFFF};
u16 q_result_reward_page_ids[4] = {0x1238, 0x1239, 0x123A, 0x123B};
u16 q_result_points_frame_ids[4] = {0x1253, 0xFFFF, 0x0000, 0x0000};
u16 q_result_rank_ids[4] = {0x1257, 0x1258, 0x1259, 0xFFFF};
u16 q_result_rank_up_ids[4] = {0x125A, 0x125B, 0x125C, 0xFFFF};
u16 q_result_size_frame_ids[2] = {0x1264, 0xFFFF};

char*** q_result_msg_adrs[12];

extern "C" {

/* This unit's own functions the header does not publish, declared up front so the bodies can stay in address
 * order. */
void q_result_hunt_recs_build(QResultScreen* self, Q_ResultWork* result);
void q_result_hunt_events_raise(Q_ResultWork* result);
u8 q_result_unlock_next(QResultScreen* self);
BOOL q_result_phase_next(QResultScreen* self, u8 phase);
void q_result_item_page_init(QResultScreen* self);
void q_result_phase_init(QResultScreen* self, s32 phase);
BOOL q_result_task(Q_MoveWork* move);
s32 multi_box_phase_input(QResultScreen* self);
BOOL multi_box_result_step(Q_MoveWork* move);
void q_result_draw_task(void);
void q_result_cursor_anim_step(QResultScreen* self);
u32 q_result_items_left_ck(QResultScreen* self, u8 which);
u32 q_result_delivery_left_ck(QResultScreen* self, u8 which);
s32 q_result_sell_total_get(QResultScreen* self, u8 which);
void q_result_items_sell(QResultScreen* self, u8 which);
void q_result_grid_cursor_step(QResultScreen* self, PadButtons* pad, u8 which);
void q_result_equip_cells_fill(QResultScreen* self);
IdValue* q_result_delivered_at(QResultScreen* self, u16 index);
void q_result_delivered_add(QResultScreen* self, IdValue* item);
_EQUIP* q_result_equip_cell_get(QResultScreen* self, u8 which);
s16 q_result_equip_sel_count(QResultScreen* self);
u32 q_result_equip_sel_toggle(QResultScreen* self, u16 index);
u32 q_result_equip_take(QResultScreen* self);
s32 q_result_phase_input(QResultScreen* self);
void q_result_timer_draw(QResultScreen* self);
void q_result_header_draw(QResultScreen* self);
void q_result_msg_frame_rect(u8 frame_kind, s16* x, s16* y, u16* width, u16* height);
void q_result_font_print_line(s16 row, s16 color, s16 line);
void q_result_item_info_draw(u16 item, s32 count, s32 max, _mh_ivec2_* unused, _mh_ivec2_* pos);
void q_result_item_counts_draw(IdValue* item);
void q_result_equip_info_draw(QResultScreen* self, _EQUIP* equip, _mh_ivec2_* pos);
void q_result_equip_detail_draw(QResultScreen* self, _EQUIP* equip);
void q_result_box_list_draw(QResultScreen* self, u8 which);
void q_result_info_panel_draw(QResultScreen* self, u8 which, u32 active);
void q_result_tab_draw(QResultScreen* self, u8 which);
void q_result_equip_tab_draw(QResultScreen* self, bool active);
void q_result_cell_frame_draw(QResultScreen* self, _mh_ivec2_* pos, u32 cursor, u32 marked, u32 moved);
void q_result_cell_marks_draw(QResultScreen* self, _mh_ivec2_* pos, u32 cursor, u32 marked, u32 moved, u32 waiting);
void q_result_item_cell_draw(QResultScreen* self, _mh_ivec2_* pos, u16 item, u32 cursor, u32 marked, u32 moved,
                             u32 waiting);
void q_result_equip_cell_draw(QResultScreen* self, _mh_ivec2_* pos, _EQUIP* equip, u32 cursor, u32 marked, u32 moved,
                              u32 waiting);
void q_result_grid_draw(QResultScreen* self, u8 which, u32 active);
void q_result_time_page_draw(QResultScreen* self);
void q_result_reward_page_draw(QResultScreen* self);
void q_result_points_page_draw(QResultScreen* self);
void q_result_size_row_draw(QResultScreen* self, u8 which, u8 monster, u16 size_min, u16 size_max, _mh_ivec2_* pos);
void q_result_size_page_draw(QResultScreen* self);
void q_result_unlock_page_draw(QResultScreen* self);
void q_result_draw(QResultScreen* self);
s16 q_result_box_count_get(u16 item);
u8 q_result_box_fit_ck(u16 item, s16 count, s16* fit);
void q_result_box_take(u16 item, s16 count);
s16 q_result_owned_count_get(u16 item);
void q_result_box_slot_set(u16 item, s16 count, u16 index);
void q_result_box_record_init(_multi_result_work* rec, u8 player_no);

/* 0x80396948 (0x2C): The language-selected message table for one message id: `q_result_msg_adrs[id][language]`. */
char** q_result_msg_table(u8 id)
{
    return q_result_msg_adrs[id][system_w.field_0x09];
}

/* 0x80396974 (0x38): One string of the language-selected message table for message `id`. */
char* q_result_msg_entry(u8 id, u8 index)
{
    return q_result_msg_adrs[id][system_w.field_0x09][index];
}

/* 0x803969AC (0x2C): The same table as `q_result_msg_table`, instruction for instruction - the page draw calls both
 * spellings, so the original source carried two names for it. */
char** q_result_msg_table_alt(u8 id)
{
    return q_result_msg_adrs[id][system_w.field_0x09];
}

/* 0x803969D8 (0x3C): Message set 10's string array, indexed by id. */
char* q_result_msg_string(u8 id)
{
    return q_result_msg_table(10)[id];
}

/* 0x80396A14 (0x3C): Message set 11's string array, indexed by id (the sibling `q_result_msg_string` reads set 10). */
char* q_result_msg_string_alt(u8 id)
{
    return q_result_msg_table(11)[id];
}

/* 0x80396A50 (0x40): Zeroes the 0x350C-byte result screen work, or reports that the move work has none yet. */
BOOL q_result_work_clear(Q_MoveWork* move)
{
    if (move->result_buffer_0x150 == NULL) {
        return FALSE;
    }
    memset(move->result_buffer_0x150, 0, 0x350C);
    return TRUE;
}

/* 0x80396A90 (0x114): Fills the size rows: every monster the quest measured whose sizes beat the save's records. */
void q_result_hunt_recs_build(QResultScreen* self, Q_ResultWork* result)
{
    Q_UserData* user = get_userdata();
    QResultMenuRow* rows;
    u8 i;
    s8 row;
    QResultHuntRec* rec;
    Q_SizeRecord* best;

    self->hunt_rec_count = 0;
    rows = (QResultMenuRow*)menu_row_table_get();
    for (i = 0; i < 0x29; i++) {
        if (result->sizes_0x0A4[i].size_min != 0) {
            row = menu_row_monster_index_get(i);
            if (row >= 0 && rows[row].mode == 1) {
                rec = &self->hunt_recs[self->hunt_rec_count];
                best = &user->sizes_0x3BC0[i];
                if (best->size_min == 0 || result->sizes_0x0A4[i].size_min < best->size_min) {
                    rec->new_flags = 1;
                } else {
                    rec->new_flags = 0;
                }
                rec->monster = i;
                rec->size_min = result->sizes_0x0A4[i].size_min;
                if (result->sizes_0x0A4[i].size_max > best->size_max) {
                    rec->new_flags |= 2;
                }
                rec->monster = i;
                rec->size_max = result->sizes_0x0A4[i].size_max;
                self->hunt_rec_count++;
            }
        }
    }
}

/* 0x80396BA4 (0x74): Raises the event of every monster in the hunt-event table the quest hunted. */
void q_result_hunt_events_raise(Q_ResultWork* result)
{
    QResultHuntEvent* entry;

    for (entry = q_result_hunt_event_tbl; entry->monster != 0; entry++) {
        if (result->count_a[entry->monster] != 0 || result->count_b[entry->monster] != 0) {
            unlock_bit_raise(entry->event);
        }
    }
}

/* 0x80396C18 (0x30): Whether the result is the arena's (0x803B5030's flag for the current record). */
u32 q_result_file_ready(QResultScreen* self)
{
    return quest_flag_10_ck(NULL) == 1;
}

/* 0x80396C48 (0x14): The `ready_flag` gate `q_result_phase_ck` switches on. */
BOOL q_result_ready_ck(QResultScreen* self)
{
    return self->ready_flag != 0;
}

/* 0x80396C5C (0x48): Whether either of the two sub-screen resource sets has finished loading. */
u32 q_result_sub_screen_ready(QResultScreen* self)
{
    if (quest_flag_2000000_ck(NULL) != 1) {
        if (quest_flag_80000000_ck(NULL) != 1) {
            return FALSE;
        }
    }
    return TRUE;
}

/* 0x80396CA4 (0x8): The screen's unlock notice count at +0x306E. */
u8 q_result_swap_counter_get(QResultScreen* self)
{
    return self->swap_counter;
}

/* 0x80396CAC (0x228): Takes the next unannounced unlock flag off the screen's masks and returns its notice text,
 * 0xFF when none is left. */
u8 q_result_unlock_next(QResultScreen* self)
{
    s32 word;
    s32 bit;
    QResultUnlockBit* entry;
    QResultUnlockEvent* event;
    u16 event_mask;
    u8 id;

    for (word = 0; word < 2; word++) {
        for (bit = 0; bit < 32; bit++) {
            if ((1 << bit) & self->unlock_bits[word]) {
                id = bit + word * 32;
                for (entry = q_result_unlock_bit_tbl; entry->bit != 0; entry++) {
                    if (entry->bit == id) {
                        self->unlock_bits[word] &= ~(1 << bit);
                        return entry->text;
                    }
                }
            }
        }
    }
    for (event = q_result_unlock_event_tbl; event->key != 0; event++) {
        event_mask = 1 << (event->event - 9000);
        if (self->unlock_events & event_mask) {
            self->unlock_events &= (u16)~event_mask;
            return event->text;
        }
    }
    return 0xFF;
}

/* 0x80396ED4 (0x30): Whether the result record reports load phase 2. */
u32 q_result_phase_is_2(QResultScreen* self)
{
    return get_qResult_work()->phase_0x1E2 == 2;
}

/* 0x80396F04 (0x30): Whether the result record reports load phase 3 (loaded). */
u32 q_result_phase_is_3(QResultScreen* self)
{
    return get_qResult_work()->phase_0x1E2 == 3;
}

/* 0x80396F34 (0x1C8): The per-phase gate the phase advance drives: mode picks which load condition the phase waits
 * for, and the return says the phase may be entered. */
BOOL q_result_phase_ck(QResultScreen* self, u8 mode)
{
    Q_ResultWork* q = get_qResult_work();

    switch (mode) {
    case 0:
        return FALSE;
    case 1:
        if (q_result_phase_is_2(self) == 1) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        break;
    case 2:
        if (q_result_phase_is_2(self) == 1) {
            return FALSE;
        }
        if (q_result_ready_ck(self) == 0) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        break;
    case 3:
        if (q->present_0x3A4 == 0) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        break;
    case 4:
        break;
    case 5:
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        break;
    case 6:
        if (q->progress_0x1E0 < 10000) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        if (q_result_file_ready(self) == 1) {
            return FALSE;
        }
        break;
    case 7:
        if (self->hunt_rec_count <= 0) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        if (q_result_file_ready(self) == 1) {
            return FALSE;
        }
        break;
    case 8:
        if (q_result_swap_counter_get(self) == 0) {
            return FALSE;
        }
        break;
    }

    return TRUE;
}

/* 0x803970FC (0x13C): Steps from `phase` to the next phase the screen may enter and latches it; 0 when `phase` is
 * the last one. */
BOOL q_result_phase_next(QResultScreen* self, u8 phase)
{
    Q_ResultWork* q = get_qResult_work();

    for (;;) {
        switch (phase) {
        case 0:
            if (q_result_phase_is_2(self) == 1) {
                phase = 3;
            } else if (q_result_phase_is_3(self) == 1) {
                phase = 8;
            } else {
                phase = 1;
            }
            break;
        case 1:
            phase = 2;
            break;
        case 2:
            phase = 3;
            break;
        case 3:
            phase = 5;
            break;
        case 5:
            phase = 6;
            break;
        case 6:
            phase = 7;
            break;
        case 7:
            if (q_result_phase_is_3(self) == 0) {
                userdata_size_records_update(&q->sizes_0x0A4[0].size_min, 0);
                userdata_size_records_update(&q->sizes_0x0A4[0].size_max, 1);
            }
            phase = 8;
            break;
        default:
            return FALSE;
        }
        if (q_result_phase_ck(self, phase) != 0) {
            break;
        }
    }
    self->phase = phase;
    return TRUE;
}

/* 0x80397238 (0xE8): Resets the item pages' cursors and points the box list at the result's item box. */
void q_result_item_page_init(QResultScreen* self)
{
    Q_UserData* user = get_userdata();
    Q_ResultWork* q = get_qResult_work();
    s16 slots;

    self->open_frame = 0;
    self->cursor_count = 0;
    self->cursor = 0;
    self->grid_cursor_col = 0;
    self->grid_cursor_row = 0;
    self->cursor_blink = 0;
    self->equip_wait = 0;
    self->grid_rows = 0;
    self->sell_confirm = 0;
    self->moved_col = 0;
    self->moved_row = 0;
    self->moved_anim = 0xFFFF;
    self->moved_item.id = 0;
    self->moved_item.value = 0;
    self->page_moved = 0;
    self->list_cursor_row = 0;
    self->list_cursor_col = 0;
    self->box_main = &q->box_0x154[0];
    slots = 0x18;
    if (userdata_gunner_ck(user) == 1) {
        self->box_extra = &q->box_0x154[0x18];
        slots = 0x20;
    } else {
        self->box_extra = NULL;
    }
    self->list_rows = menu_page_count(slots, 8);
    self->equip_sel_bits[0] = 0;
    self->equip_sel_bits[1] = 0;
    self->equip_sel_bits[2] = 0;
}

/* 0x80397320 (0x18): Latches the one-shot `init_flag` at +0x3027. */
void q_result_init_flag_set(QResultScreen* self)
{
    if (self->init_flag == 0) {
        self->init_flag = 1;
    }
}

/* 0x80397338 (0xC): Arms the 90-frame swap timer and requests the swap sound effect. */
void q_result_swap_start(QResultScreen* self)
{
    self->swap_timer = 0x5A;
    sysSE_bank24_req();
}

/* 0x80397344 (0x228): Enters `phase`: resets the phase timer and sets up the page the phase shows. */
void q_result_phase_init(QResultScreen* self, s32 phase)
{
    Q_UserData* user = get_userdata();

    get_qResult_work();
    self->phase = phase;
    self->select_mode = 0;
    self->phase_step = 0;
    self->phase_timer = 120.0f * Screen_w.frame_scale;
    self->phase_frame = 0;
    switch (phase) {
    case 1:
        q_result_item_page_init(self);
        self->cursor_count = 3;
        if (q_result_file_ready(self) == 1) {
            self->grid_rows = 2;
        } else if (q_result_sub_screen_ready(self) == 1) {
            self->grid_rows = 6;
        } else {
            self->grid_rows = 5;
        }
        break;
    case 2:
        q_result_item_page_init(self);
        self->cursor_count = 3;
        self->grid_rows = 6;
        break;
    case 3:
        q_result_item_page_init(self);
        self->cursor_count = 1;
        self->grid_rows = 6;
        self->equip_ready = 0;
        break;
    case 5:
        q_result_init_flag_set(self);
        score_add_clamped(self->zenny_total + self->hunt_zenny_total + self->item_value_total,
                          &((Q_UserData*)lobby_world_block)->zenny_0x18);
        self->cursor = 0;
        self->cursor_count = 4;
        break;
    case 6:
        q_result_init_flag_set(self);
        self->rank_before = user->hunter_rank_0x3DE4;
        self->rank_state = userdata_hunter_points_add(self->points_total);
        self->rank_anim = 0;
        break;
    case 7:
        q_result_init_flag_set(self);
        self->hunt_anim[0] = 0;
        self->hunt_anim[1] = 0;
        self->hunt_anim[2] = 0;
        self->hunt_anim[3] = 0;
        self->hunt_anim[4] = 0;
        self->hunt_anim[5] = 0;
        self->hunt_anim[6] = 0;
        self->hunt_anim[7] = 0;
        self->hunt_anim[8] = 0;
        self->hunt_anim[9] = 0;
        self->cursor = 0;
        self->cursor_count = self->hunt_rec_count;
        break;
    case 8:
        q_result_init_flag_set(self);
        q_result_swap_start(self);
        self->cursor_count = q_result_swap_counter_get(self);
        self->cursor = 0;
        self->unlock_text = q_result_unlock_next(self);
        self->unlock_wait = 10;
        break;
    }
}

/* 0x8039756C (0xB00): The result screen's frame task: sets the screen up from the quest's records on its first
 * frame (rewards, kept items, size rows, unlocks), then runs the phase it is in; 1 once the screen has closed. */
BOOL q_result_task(Q_MoveWork* move)
{
    u32* seen = system_w.unlock_seen_0x888;
    Q_UserData* user = get_userdata();
    QResultScreen* self = (QResultScreen*)move->result_buffer_0x150;
    Q_ResultWork* q = get_qResult_work();
    Q_ItemWork* work = move_work_item_work_get();
    struct _PLW* plw;
    Q_ResultRow* row;
    s32 i;
    f32 scale;
    u8 arena;
    s32 word;
    s32 bit;
    u16 events;
    QResultUnlockEvent* event;

    q_result_noop(self);
    switch (self->state) {
    case 0:
        if (file_loading_ck(NULL, NULL) == 1) {
            break;
        }
        self->state++;
        self->sub_state = 0;
        self->player_no = my_player_no();
        if (q->progress_0x1E0 >= 10000) {
            if (move->player_count_0x22E2 != 0) {
                self->field_0x0007 = move->player_count_0x22E2;
            } else {
                self->field_0x0007 = 1;
            }
        } else {
            self->field_0x0007 = 1;
        }
        plw = &((PlMoveWork*)get_move_work_adrs(2))[self->player_no].pl;
        if (q->progress_0x1E0 >= 10000 && user->hunter_rank_0x3DE4 >= 31) {
            if (quest_slot_progress_get(NULL) == 0) {
                self->reward_cut = 1;
            } else {
                self->reward_cut = 0;
            }
        } else {
            self->reward_cut = 0;
        }
        self->swap_timer = 0;
        self->zenny_rows[0] = 0;
        self->zenny_rows[1] = 0;
        self->zenny_rows[2] = 0;
        self->zenny_rows[3] = 0;
        q->field_0x1E4 = 0;
        self->point_rows[0] = 0;
        self->point_rows[1] = 0;
        self->point_rows[2] = 0;
        q->field_0x1E8 = 0;
        self->hunt_count[0] = 0;
        self->hunt_count[1] = 0;
        if (q_result_phase_is_3(self) == 0) {
            self->skill_points = Pl_cat_skill_ck(plw, 0x1B);
            self->skill_zenny = Pl_cat_skill_ck(plw, 0x29);
            row = work->record_0x3C;
            if (row->objective_0x34C[0] > 0) {
                self->reward_shown[0] = 1;
                if (quest_element_pick_ck((QuestWork*)work, (quest_flag_4000000_ck(NULL) == 1) ? 2 : 0, 1) == 1 ||
                    (move->sub_0xFA == 4 &&
                     ((work->record_0x3C->flags_0x310 & 0x800000) || quest_element_state_ck() == 1))) {
                    self->reward_cleared[0] = 1;
                }
            }
            if (quest_flag_100_ck(NULL) == 0) {
                if (row->objective_0x34C[1] > 0) {
                    self->reward_shown[1] = 1;
                    if (quest_element_pick_ck((QuestWork*)work, 1, 1) == 1) {
                        self->reward_cleared[1] = 1;
                    }
                }
                if (row->objective_0x34C[2] > 0) {
                    self->reward_shown[2] = 1;
                    if (quest_element_pick_ck((QuestWork*)work, (quest_flag_4000000_ck(NULL) == 1) ? 0 : 2, 1) == 1) {
                        self->reward_cleared[2] = 1;
                    }
                }
            }
            self->hunt_zenny_total = 0;
            self->hunt_points_total = 0;
            if (q_result_phase_is_2(self) == 0) {
                for (i = 0; i < 2; i++) {
                    self->hunt_count[i] = work->tier_count_0x6980[i];
                    self->hunt_monster[i] = move->spawn_0x2274[i + 3].monster_0x6;
                    if (quest_flag_100_ck(NULL) == 1) {
                        self->hunt_zenny[i] = work->sub_reward_0x2EC[1];
                        self->hunt_points[i] = work->sub_points_0x300[1];
                    } else {
                        self->hunt_zenny[i] = work->tier_zenny_0x698C[i];
                        self->hunt_points[i] = work->tier_points_0x6998[i];
                    }
                }
                if (self->reward_cleared[0] == 1) {
                    self->zenny_rows[1] = work->reward_0x2E8 / self->field_0x0007;
                    if (self->player_no == 0) {
                        self->zenny_rows[0] = work->fee_0x2E4 * 2;
                    }
                }
                if (self->reward_cleared[1] == 1) {
                    self->zenny_rows[2] = work->sub_reward_0x2EC[0] / self->field_0x0007;
                }
                if (self->reward_cleared[2] == 1) {
                    self->zenny_rows[3] = work->sub_reward_0x2EC[1] / self->field_0x0007;
                }
                scale = 1.0f;
                if (self->reward_cut != 0) {
                    scale *= 0.75f;
                }
                if (self->skill_zenny == 1) {
                    scale *= 2.0f;
                }
                self->zenny_rows[1] = (f32)self->zenny_rows[1] * scale;
                self->zenny_rows[2] = (f32)self->zenny_rows[2] * scale;
                self->zenny_rows[3] = (f32)self->zenny_rows[3] * scale;
                if (self->hunt_count[0] != 0) {
                    self->hunt_zenny[0] = self->hunt_zenny[0] / self->field_0x0007;
                    self->hunt_zenny_total += self->hunt_zenny[0] * self->hunt_count[0];
                }
                if (self->hunt_count[1] != 0) {
                    self->hunt_zenny[1] = self->hunt_zenny[1] / self->field_0x0007;
                    self->hunt_zenny_total += self->hunt_zenny[1] * self->hunt_count[1];
                }
                if (q_result_file_ready(self) == 1) {
                    arena = q->progress_0x1E0 - 60000;
                    if (arena < 12) {
                        if (user->arena_best_0x5334[arena] == 0 || user->arena_best_0x5334[arena] > q->elapsed_0x148) {
                            user->arena_best_0x5334[arena] = q->elapsed_0x148;
                            if (self->field_0x0007 == 2) {
                                PlMoveWork* partner = (PlMoveWork*)get_move_work_adrs(2);

                                if (self->player_no == 0) {
                                    partner = &partner[1];
                                }
                                strcpy(user->arena_0x42D0[arena].name_a, (char*)&partner->pl.user_profile_0x5CA[0]);
                                strcpy(user->arena_0x42D0[arena].name_b, (char*)&partner->pl.user_profile_0x5CA[0x11]);
                            } else {
                                user->arena_0x42D0[arena].name_a[0] = 0;
                                user->arena_0x42D0[arena].name_b[0] = 0;
                            }
                        }
                        if (q->elapsed_0x148 <= arena_time_table[arena][1] * 30) {
                            userdata_award_bit_set((struct LbEquipWork*)user, 15);
                        }
                    }
                }
                if (q->progress_0x1E0 == 14002 && userdata_flag_ck(14002) == 1) {
                    unlock_bit_raise(6);
                }
            } else if (quest_flag_2000000_ck(NULL) == 1 || quest_flag_80000000_ck(NULL) == 1) {
                self->reward_cleared[0] = 0;
                self->reward_cleared[1] = 0;
            }
            q->field_0x1E4 = self->zenny_rows[0] + self->zenny_rows[1] + self->zenny_rows[2] + self->zenny_rows[3];
            if (self->reward_cleared[0] == 1) {
                self->point_rows[0] = work->points_0x2F8;
            }
            if (self->reward_cleared[1] == 1) {
                self->point_rows[1] = work->sub_points_0x300[0];
            }
            if (self->reward_cleared[2] == 1) {
                self->point_rows[2] = work->sub_points_0x300[1];
            }
            scale = 1.0f;
            if (self->reward_cut != 0) {
                scale *= 0.75f;
            }
            if (self->skill_points == 1) {
                scale *= 1.5f;
            }
            self->point_rows[0] = (f32)self->point_rows[0] * scale;
            self->point_rows[1] = (f32)self->point_rows[1] * scale;
            self->point_rows[2] = (f32)self->point_rows[2] * scale;
            if (self->hunt_count[0] != 0) {
                self->hunt_points_total += self->hunt_points[0] * self->hunt_count[0];
            }
            if (self->hunt_count[1] != 0) {
                self->hunt_points_total += self->hunt_points[1] * self->hunt_count[1];
            }
            self->faint_count = work->my_faint_count_0x92;
            q->field_0x1E8 = self->faint_count * -20 + (self->point_rows[0] + self->point_rows[1]) +
                             (self->hunt_points_total + self->point_rows[2]);
            quest_item_pair_tbl_copy((Q_ItemPair*)self->grid_items, (Q_ItemPair*)self->grid_kept);
            if (q_result_items_left_ck(self, 1) == 1) {
                self->ready_flag = 1;
            } else {
                self->ready_flag = 0;
            }
            q_result_hunt_recs_build(self, q);
            self->kept_item_count = 0;
            for (i = 0; i < 0x23; i++) {
                if (q->kept_0x1EC[i].id != 0) {
                    item_pair_copy(&self->kept_items[self->kept_item_count], &q->kept_0x1EC[i]);
                    self->item_value_total += GetItemData(self->kept_items[self->kept_item_count].id)->field_0x010 *
                                              self->kept_items[self->kept_item_count].value;
                    self->kept_item_count++;
                }
            }
            self->zenny_total = q->field_0x1E4;
            self->points_total = q->field_0x1E8;
            q_result_hunt_events_raise(q);
        }
        self->swap_counter = 0;
        for (word = 0; word < 2; word++) {
            self->unlock_bits[word] = move->unlock_bits_0x148[word] & ~seen[word];
            for (bit = 0; bit < 32; bit++) {
                if ((1 << bit) & self->unlock_bits[word]) {
                    self->swap_counter++;
                }
            }
        }
        unlock_seen_mark(move->unlock_bits_0x148);
        events = user->event_bits_0x52E2;
        for (event = q_result_unlock_event_tbl; event->key != 0; event++) {
            if (userdata_flag_ck(event->key) == 1 && userdata_event_bit_ck(event->event) == 0) {
                userdata_event_bit_set(event->event);
                self->swap_counter++;
            }
        }
        self->unlock_events = user->event_bits_0x52E2 & ~events;
        self->init_flag = 0;
        if (q_result_phase_next(self, 0) == 0) {
            return TRUE;
        }
        break;
    case 1:
        if (self->swap_timer > 0) {
            self->swap_timer--;
        }
        switch (self->sub_state) {
        case 0:
            system_copy_filter_arm();
            q_result_phase_apply(self);
            break;
        case 1:
            if (q_result_phase_input(self) == 1) {
                self->sub_state++;
            }
            break;
        case 2:
            self->phase_timer++;
            if (self->phase_timer > 20) {
                self->phase_timer = 0;
                if (q_result_phase_next(self, self->phase) == 0) {
                    self->sub_state++;
                    system_copy_filter_clear();
                    self->phase_timer = 16;
                    if (self->phase_timer < self->swap_timer) {
                        self->phase_timer = self->swap_timer;
                    }
                } else {
                    self->sub_state = 0;
                    q_result_phase_apply(self);
                }
            }
            break;
        case 3:
            if (self->phase_timer > 0) {
                self->phase_timer--;
                break;
            }
            return TRUE;
        }
        subTransSet((u32)q_result_draw_task, 0, NULL);
        break;
    }
    return FALSE;
}

/* 0x8039806C (0x84): The sub-transparency draw callback: draws the result screen (or the arena's own page in play
 * mode 2) while the root move work's screen is running. */
void q_result_draw_task(void)
{
    u8* root = (u8*)get_move_work_adrs(0);
    QResultScreen* self;

    if (root != NULL) {
        self = (QResultScreen*)((Q_MoveWork*)root)->result_buffer_0x150;
        if (PlayMode_ck() == 2) {
            if (self->state == 1) {
                ((void (*)(QResultScreen*))arena_draw_func[2])(self);
            }
        } else if (self->state == 1) {
            q_result_draw(self);
        }
    }
}

/* 0x803980F0 (0x110): Enters the phase the screen names and steps the sub-state on. */
void q_result_phase_apply(QResultScreen* self)
{
    switch (self->phase) {
    case 0:
        q_result_phase_init(self, 1);
        self->sub_state++;
        break;
    case 1:
        q_result_phase_init(self, 1);
        self->sub_state++;
        break;
    case 2:
        q_result_phase_init(self, 2);
        self->sub_state++;
        break;
    case 3:
        q_result_phase_init(self, 3);
        self->sub_state++;
        break;
    case 5:
        system_copy_filter_clear();
        q_result_phase_init(self, 5);
        self->sub_state++;
        break;
    case 6:
        q_result_phase_init(self, 6);
        self->sub_state++;
        break;
    case 7:
        q_result_phase_init(self, 7);
        self->sub_state++;
        break;
    case 8:
        q_result_phase_init(self, 8);
        self->sub_state++;
        break;
    }
}

/* 0x80398200 (0x54): Steps the cursor blink (wraps at 40) and the moved item's flash (idle again after 12). */
void q_result_cursor_anim_step(QResultScreen* self)
{
    self->page_moved = 0;
    if (++self->cursor_blink > 40) {
        self->cursor_blink = 0;
    }
    if (self->moved_anim != 0xFFFF) {
        if (++self->moved_anim > 12) {
            self->moved_anim = 0xFFFF;
        }
    }
}

/* 0x80398254 (0x260): Whether the grid `which` names still holds anything: an item with a count (grids 0..2) or an
 * equipment record (3). */
u32 q_result_items_left_ck(QResultScreen* self, u8 which)
{
    IdValue* items = NULL;
    _EQUIP* equip = NULL;
    s32 i;

    switch (which) {
    case 0:
    case 2:
        items = self->grid_items;
        break;
    case 1:
        items = self->grid_kept;
        break;
    case 3:
        equip = self->equip_cells;
        break;
    default:
        return 0;
    }
    if (items != NULL) {
        for (i = 0; i < 0x30; i++) {
            if (items[i].id != 0 && items[i].value > 0) {
                return 1;
            }
        }
    }
    if (equip != NULL) {
        for (i = 0; i < 0x28; i++) {
            if (equip[i].kind != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x803984B4 (0xB8): Whether the grid `which` names still holds a delivery item (category bit 0x10). */
u32 q_result_delivery_left_ck(QResultScreen* self, u8 which)
{
    IdValue* items;
    s32 i;

    switch (which) {
    case 0:
    case 2:
        items = self->grid_items;
        break;
    case 1:
        items = self->grid_kept;
        break;
    default:
        return 0;
    }
    if (items != NULL) {
        for (i = 0; i < 0x30; i++) {
            if (items[i].id != 0 && items[i].value > 0 && (GetItemData(items[i].id)->field_0x002 & 0x10)) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x8039856C (0x120): What selling everything left in the grid `which` names would pay. */
s32 q_result_sell_total_get(QResultScreen* self, u8 which)
{
    IdValue* items = NULL;
    _EQUIP* equip = NULL;
    s32 total = 0;
    s32 i;
    ItemDataRecord* data;

    switch (which) {
    case 0:
    case 2:
        items = self->grid_items;
        break;
    case 1:
        items = self->grid_kept;
        break;
    case 3:
        equip = self->equip_cells;
        break;
    default:
        return 0;
    }
    if (items != NULL) {
        for (i = 0; i < 0x30; i++) {
            if (items[i].id != 0 && items[i].value > 0) {
                data = GetItemData(items[i].id);
                if (!(data->field_0x002 & 0x10)) {
                    total += data->field_0x010 * items[i].value;
                }
            }
        }
    }
    if (equip != NULL) {
        for (i = 0; i < 0x28; i++) {
            if (equip[i].kind != 0) {
                total += equip_sell_price_get(&equip[i]);
            }
        }
    }
    return total;
}

/* 0x8039868C (0x144): Sells everything left in the grid `which` names: delivery items go to the result's delivered
 * list, the rest is credited to the save's money, and the cells are emptied. */
void q_result_items_sell(QResultScreen* self, u8 which)
{
    IdValue* items = NULL;
    _EQUIP* equip = NULL;
    s32 i;
    ItemDataRecord* data;

    switch (which) {
    case 0:
    case 2:
        items = self->grid_items;
        break;
    case 1:
        items = self->grid_kept;
        break;
    case 3:
        equip = self->equip_cells;
        break;
    default:
        return;
    }
    if (items != NULL) {
        for (i = 0; i < 0x30; i++) {
            if (items[i].id != 0 && items[i].value > 0) {
                data = GetItemData(items[i].id);
                if (data->field_0x002 & 0x10) {
                    q_result_delivered_add(self, &items[i]);
                } else {
                    score_add_clamped(data->field_0x010 * items[i].value, &((Q_UserData*)lobby_world_block)->zenny_0x18);
                }
                items[i].id = 0;
                items[i].value = 0;
            }
        }
    }
    if (equip != NULL) {
        for (i = 0; i < 0x28; i++) {
            if (equip[i].kind != 0) {
                score_add_clamped(equip_sell_price_get(&equip[i]), &((Q_UserData*)lobby_world_block)->zenny_0x18);
                memset(&equip[i], 0, sizeof(_EQUIP));
            }
        }
    }
}

/* 0x803987D0 (0xE4): Moves the grid cursor with the pad: rows wrap through `grid_rows`, columns through 8 (the
 * equipment grid's last row is the command row, where the column stays put). */
void q_result_grid_cursor_step(QResultScreen* self, PadButtons* pad, u8 which)
{
    u8 row;

    row = menu_cursor_step(self->grid_cursor_row, self->grid_rows, pad->pressed_0x04 | pad->held_0x14, 1, 2);
    self->grid_cursor_row = row;
    if (which == 3) {
        if (row < self->grid_rows - 1) {
            self->grid_cursor_col = menu_cursor_step(self->grid_cursor_col, 8, pad->pressed_0x04 | pad->held_0x14, 4, 8);
        }
    } else {
        self->grid_cursor_col = menu_cursor_step(self->grid_cursor_col, 8, pad->pressed_0x04 | pad->held_0x14, 4, 8);
    }
}

/* 0x803988B4 (0x18): The item grid's cursor as a flat index: column plus row times eight. */
u16 q_result_grid_cursor_index(QResultScreen* self)
{
    return self->grid_cursor_col + (self->grid_cursor_row << 3);
}

/* 0x803988CC (0x6C): The cell of the grid `which` names under the grid cursor. */
IdValue* q_result_grid_entry(QResultScreen* self, u8 which)
{
    IdValue* items;

    switch (which) {
    case 0:
    case 2:
        items = self->grid_items;
        break;
    case 1:
        items = self->grid_kept;
        break;
    default:
        return NULL;
    }
    return &items[q_result_grid_cursor_index(self)];
}

/* 0x80398938 (0xAC): Fills the equipment grid from the result's delivered list: one record per delivered piece. */
void q_result_equip_cells_fill(QResultScreen* self)
{
    _EQUIP* cell = self->equip_cells;
    IdValue* item = get_qResult_work()->delivered_ids_0x304;
    s32 i;
    s32 n;

    for (i = 0; i < 0x28; i++, item++) {
        if (item->id == 0 || item->value <= 0) {
            memset(cell, 0, sizeof(_EQUIP));
        } else {
            for (n = 0; n < item->value; n++) {
                equip_from_item_id(item->id, cell);
                cell++;
            }
        }
    }
    self->equip_ready = 1;
}

/* 0x803989E4 (0x110): The delivered pair the flat equipment index `index` falls in, or null past the last one. */
IdValue* q_result_delivered_at(QResultScreen* self, u16 index)
{
    IdValue* item = get_qResult_work()->delivered_ids_0x304;
    s32 i;

    for (i = 0; i < 0x28; i++, item++) {
        if (item->value >= 0) {
            if (item->value > index) {
                return item;
            }
            index -= item->value;
        }
    }
    return NULL;
}

/* 0x80398AF4 (0xA0): Adds `item` to the result's delivered list: onto the pair with its id, else into the first
 * free pair; marks the result as carrying records. */
void q_result_delivered_add(QResultScreen* self, IdValue* item)
{
    Q_ResultWork* q = get_qResult_work();
    IdValue* slot = q->delivered_ids_0x304;
    s32 i;

    for (i = 0; i < 0x28; i++, slot++) {
        if (slot->id != 0 && slot->value >= 0) {
            if (slot->id == item->id) {
                slot->value += item->value;
                q->present_0x3A4 = 1;
                return;
            }
            continue;
        }
        item_pair_copy(slot, item);
        q->present_0x3A4 = 1;
        return;
    }
}

/* 0x80398B94 (0x80): The equipment cell under the grid cursor (grid 3 only, not on the command row), or null. */
_EQUIP* q_result_equip_cell_get(QResultScreen* self, u8 which)
{
    _EQUIP* cells;

    switch (which) {
    case 3:
        if (self->select_mode >= 2) {
            if (self->grid_cursor_row == self->grid_rows - 1) {
                return NULL;
            }
            cells = self->equip_cells;
            break;
        }
        return NULL;
    default:
        return NULL;
    }
    return &cells[q_result_grid_cursor_index(self)];
}

/* 0x80398C14 (0x18): The box list's cursor as a flat index: column plus row times eight. */
u16 q_result_list_cursor_index(QResultScreen* self)
{
    return self->list_cursor_col + (self->list_cursor_row << 3);
}

/* 0x80398C2C (0x5C): The box slot under the list cursor: the first 0x18 come from the main box and the rest
 * continue in the extra one. */
IdValue* q_result_list_entry(QResultScreen* self)
{
    u16 index = q_result_list_cursor_index(self);

    if (index < 0x18) {
        return &self->box_main[index];
    }
    return &self->box_extra[(u16)(index - 0x18)];
}

/* 0x80398C88 (0x118): How many equipment cells are marked. */
s16 q_result_equip_sel_count(QResultScreen* self)
{
    s16 count = 0;
    s32 word;
    s32 bit;

    for (word = 0; word < 3; word++) {
        for (bit = 0; bit < 16; bit++) {
            if ((1 << bit) & self->equip_sel_bits[word]) {
                count++;
            }
        }
    }
    return count;
}

/* 0x80398DA0 (0xA8): Toggles the mark of equipment cell `index`; a new mark needs a free equipment box slot. */
u32 q_result_equip_sel_toggle(QResultScreen* self, u16 index)
{
    u16 bit = 1 << (index & 0xF);
    s32 word = index >> 4;

    if (!(bit & self->equip_sel_bits[word])) {
        if (userdata_equip_box_free_count() - q_result_equip_sel_count(self) <= 0) {
            return 0;
        }
    }
    self->equip_sel_bits[word] ^= bit;
    return 1;
}

/* 0x80398E48 (0xDC): Files every marked equipment cell into the save's equipment box and empties it; 1 when
 * anything was filed. */
u32 q_result_equip_take(QResultScreen* self)
{
    Q_UserData* user = get_userdata();
    u32 taken = 0;
    u16 i;
    _EQUIP* cell;

    for (i = 0; i < 0x28; i++) {
        if ((1 << (i & 0xF)) & self->equip_sel_bits[i >> 4]) {
            cell = &self->equip_cells[i];
            if (cell->kind == 6) {
                userdata_equip_box_add(user, cell);
                taken = 1;
            } else if (cell->kind != 0) {
                userdata_equip_box_add_new(user, cell->kind, cell->item_id);
                taken = 1;
            }
            memset(cell, 0, sizeof(_EQUIP));
            q_result_equip_sel_toggle(self, i);
        }
    }
    return taken;
}

/* 0x80398F24 (0x58): Records the move of the item `item` out of grid cell (`col`, `row`) and starts its flash. */
void q_result_row_init(QResultScreen* self, const IdValue* item, u8 col, u8 row)
{
    self->moved_anim = 0;
    item_pair_copy(&self->moved_item, item);
    self->moved_col = col;
    self->moved_row = row;
}

/* 0x80398F7C (0xC80): The running phase's per-frame input: counts the phase timer down (then sells what is left and
 * returns 1), moves the cursors, takes and sells items and steps the notice pages; 1 once the phase is done. */
s32 q_result_phase_input(QResultScreen* self)
{
    PadButtons* pad = &Psw[0].button_0x2C0;
    Q_UserData* user = get_userdata();
    u8 which;
    IdValue* cell;
    IdValue held;
    s16 fit;
    s16 room;
    u16 index;
    ItemDataRecord* data;
    _EQUIP* equip;
    u16 keys;
    u8 anim;
    u8 box_out[4];

    if (--self->phase_timer <= 0) {
        switch (self->phase) {
        case 1:
            if (q_result_file_ready(self) == 1) {
                q_result_items_sell(self, 2);
            } else {
                q_result_items_sell(self, 0);
            }
            break;
        case 2:
            q_result_items_sell(self, 1);
            sysSE_req(9);
            /* fall through */
        case 3:
            if (self->select_mode <= 1) {
                q_result_equip_cells_fill(self);
            }
            q_result_equip_take(self);
            q_result_items_sell(self, 3);
            break;
        }
        return 1;
    }

    switch (self->phase) {
    case 1:
    case 2:
        ai_npc_reaction_forward();
        if (self->phase == 2) {
            which = 1;
        } else {
            which = (q_result_file_ready(self) == 1) ? 2 : 0;
        }
        q_result_cursor_anim_step(self);
        if (self->open_frame < 7) {
            if (self->open_frame == 0) {
                sysSE_req(5);
            }
            self->open_frame++;
        }
        switch (self->select_mode) {
        case 0:
            if (pad->pressed_0x04 & 0x10) {
                switch (self->cursor) {
                case 0:
                    if (q_result_items_left_ck(self, which) == 1) {
                        self->select_mode = 1;
                        self->sell_mode = 0;
                        sysSE_req(0);
                    } else {
                        sysSE_req(2);
                    }
                    break;
                case 1:
                    if (q_result_items_left_ck(self, which) == 1) {
                        self->select_mode = 1;
                        self->sell_mode = 1;
                        sysSE_req(0);
                    } else {
                        sysSE_req(2);
                    }
                    break;
                case 2:
                    sysSE_req(0);
                    if (q_result_items_left_ck(self, which) == 1) {
                        self->select_mode = 2;
                        self->sell_confirm = 0;
                        self->sell_total = q_result_sell_total_get(self, which);
                        break;
                    }
                    return 1;
                }
            } else if (pad->pressed_0x04 & 0x20) {
                self->cursor = self->cursor_count - 1;
                sysSE_req(1);
            } else if (q_result_items_left_ck(self, which) == 1) {
                self->cursor = menu_cursor_step(self->cursor, self->cursor_count, pad->pressed_0x04 | pad->held_0x14, 1, 2);
            }
            break;
        case 1:
            if (pad->pressed_0x04 & 0x10) {
                cell = q_result_grid_entry(self, which);
                if (cell->id != 0) {
                    if (GetItemData(cell->id)->field_0x002 & 0x10) {
                        q_result_row_init(self, cell, self->grid_cursor_col, self->grid_cursor_row);
                        q_result_delivered_add(self, cell);
                        cell->id = 0;
                        cell->value = 0;
                        sysSE_req(8);
                    } else if (self->sell_mode == 0) {
                        switch (q_result_box_fit_ck(cell->id, cell->value, &fit)) {
                        case 0:
                            q_result_row_init(self, cell, self->grid_cursor_col, self->grid_cursor_row);
                            q_result_box_take(cell->id, cell->value);
                            cell->id = 0;
                            cell->value = 0;
                            sysSE_req(8);
                            break;
                        case 2:
                            q_result_row_init(self, cell, self->grid_cursor_col, self->grid_cursor_row);
                            q_result_box_take(cell->id, fit);
                            cell->value -= fit;
                            if (cell->value <= 0) {
                                cell->id = 0;
                            }
                            sysSE_req(8);
                            break;
                        case 3:
                            self->select_mode = 3;
                            self->list_cursor_row = 0;
                            self->list_cursor_col = 0;
                            sysSE_req(0);
                            break;
                        default:
                            sysSE_req(2);
                            break;
                        }
                    } else {
                        room = item_slots_room_get(cell->id, ((Q_UserData*)lobby_world_block)->box_0x180,
                                                   userdata_box_capacity(lobby_world_block));
                        if (room > 0) {
                            if (room > cell->value) {
                                room = cell->value;
                            }
                            q_result_row_init(self, cell, self->grid_cursor_col, self->grid_cursor_row);
                            item_box_store(cell->id, room, box_out);
                            cell->value -= room;
                            if (cell->value == 0) {
                                cell->id = 0;
                            }
                            sysSE_req(8);
                        } else {
                            sysSE_req(2);
                        }
                    }
                } else {
                    sysSE_req(2);
                }
                if (q_result_items_left_ck(self, which) == 0) {
                    self->select_mode = 0;
                    self->cursor = self->cursor_count - 1;
                }
            } else if (pad->pressed_0x04 & 0x20) {
                self->select_mode = 0;
                sysSE_req(1);
            } else {
                q_result_grid_cursor_step(self, pad, which);
            }
            break;
        case 2:
            switch (toggle_word_step(&self->sell_confirm, pad->pressed_0x04 | pad->held_0x14, 4, 8, 0xFFFF)) {
            case 1:
                q_result_items_sell(self, which);
                sysSE_req(9);
                return 1;
            case 2:
                self->select_mode = 0;
                break;
            }
            break;
        case 3:
            keys = pad->pressed_0x04;
            if (keys & 0x10) {
                cell = q_result_grid_entry(self, which);
                data = GetItemData(cell->id);
                index = q_result_list_cursor_index(self);
                if ((s16)index < 0x18 || data->kind_0x00 == 1) {
                    item_pair_copy(&held, cell);
                    item_pair_copy(cell, q_result_list_entry(self));
                    q_result_box_slot_set(held.id, held.value, index);
                    self->select_mode = 1;
                    sysSE_req(0);
                }
            } else if (keys & 0x20) {
                self->select_mode = 1;
                sysSE_req(1);
            } else {
                keys |= pad->held_0x14;
                if (keys & 3) {
                    self->list_cursor_col = menu_cursor_step(self->list_cursor_col, 8, keys, 1, 2);
                } else if (keys & 0xC) {
                    self->list_cursor_row = menu_cursor_step_fixed_tail(self->list_cursor_row, self->list_rows, keys, 4, 8,
                                                                        &self->page_moved);
                }
            }
            break;
        }
        break;
    case 3:
        ai_npc_reaction_forward();
        q_result_cursor_anim_step(self);
        if (self->open_frame < 7) {
            if (self->open_frame == 0) {
                sysSE_req(5);
            }
            self->open_frame++;
        }
        switch (self->select_mode) {
        case 0:
            if (pad->pressed_0x04 & 0x10) {
                self->select_mode++;
                self->cursor_count = 0;
                sysSE_bank20_req(0);
            }
            break;
        case 1:
            if (++self->equip_wait >= 60) {
                self->select_mode++;
                self->cursor_count = 2;
            }
            if (self->equip_ready == 0 && self->equip_wait > 3) {
                q_result_equip_cells_fill(self);
            }
            break;
        case 2:
            if (pad->pressed_0x04 & 0x10) {
                sysSE_req(0);
                switch (self->cursor) {
                case 0:
                    if (q_result_items_left_ck(self, 3) == 1) {
                        self->select_mode = 3;
                    } else {
                        sysSE_req(2);
                    }
                    break;
                case 1:
                    if (q_result_items_left_ck(self, 3) == 1) {
                        self->select_mode = 4;
                        self->sell_confirm = 0;
                        self->sell_total = q_result_sell_total_get(self, 3);
                        break;
                    }
                    return 1;
                }
            } else if (pad->pressed_0x04 & 0x20) {
                self->cursor = self->cursor_count - 1;
                sysSE_req(1);
            } else if (q_result_items_left_ck(self, 3) == 1) {
                self->cursor = menu_cursor_step(self->cursor, self->cursor_count, pad->pressed_0x04 | pad->held_0x14, 1, 2);
            }
            break;
        case 3:
            if (pad->pressed_0x04 & 0x10) {
                if (self->grid_cursor_row == self->grid_rows - 1) {
                    if (q_result_equip_take(self) == 1) {
                        sysSE_req(8);
                    } else {
                        sysSE_req(2);
                    }
                    if (q_result_items_left_ck(self, 3) == 0) {
                        self->select_mode = 2;
                        self->cursor = self->cursor_count - 1;
                    }
                } else {
                    equip = q_result_equip_cell_get(self, 3);
                    if (equip != NULL && equip->kind != 0) {
                        if (q_result_equip_sel_toggle(self, q_result_grid_cursor_index(self)) == 1) {
                            sysSE_req(0);
                        } else {
                            sysSE_req(2);
                        }
                    } else {
                        sysSE_req(2);
                    }
                }
            } else if (pad->pressed_0x04 & 0x20) {
                self->select_mode = 2;
                sysSE_req(1);
            } else {
                q_result_grid_cursor_step(self, pad, 3);
            }
            break;
        case 4:
            switch (toggle_word_step(&self->sell_confirm, pad->pressed_0x04 | pad->held_0x14, 4, 8, 0xFFFF)) {
            case 1:
                q_result_items_sell(self, 3);
                sysSE_req(9);
                return 1;
            case 2:
                self->select_mode = 2;
                break;
            }
            break;
        }
        break;
    case 4:
        if (pad->pressed_0x04 & 0x10) {
            sysSE_req(0);
            return 1;
        }
        break;
    case 5:
        self->page_moved = 0;
        self->cursor = menu_cursor_step_fixed_tail(self->cursor, self->cursor_count, pad->pressed_0x04 | pad->held_0x14, 4,
                                                   8, &self->page_moved);
        if (pad->pressed_0x04 & 0x10) {
            sysSE_req(0);
            return 1;
        }
        break;
    case 6:
        if (self->rank_anim < 40) {
            if (self->rank_anim == 0 && user->hunter_rank_0x3DE4 > self->rank_before) {
                sysSE_bank20_req(4);
                q_result_swap_start(self);
            }
            self->rank_anim++;
        }
        if (pad->pressed_0x04 & 0x10) {
            sysSE_req(0);
            return 1;
        }
        break;
    case 7:
        self->page_moved = 0;
        anim = self->hunt_anim[self->cursor];
        if (anim < 7) {
            if (self->hunt_recs[self->cursor].new_flags != 0 && anim == 0) {
                q_result_swap_start(self);
            }
            self->hunt_anim[self->cursor]++;
        } else {
            if (self->hunt_recs[self->cursor].new_flags != 0 && anim == 7) {
                sysSE_bank20_req(2);
                self->hunt_anim[self->cursor]++;
            }
            self->cursor = menu_cursor_step_fixed_tail(self->cursor, self->cursor_count,
                                                       pad->pressed_0x04 | pad->held_0x14, 4, 8, &self->page_moved);
        }
        if (pad->pressed_0x04 & 0x10) {
            sysSE_req(0);
            return 1;
        }
        break;
    case 8:
        if (self->unlock_wait > 0) {
            self->unlock_wait--;
        }
        if (self->unlock_wait <= 0 && (pad->pressed_0x04 & 0x10)) {
            sysSE_req(0);
            if (++self->cursor >= self->cursor_count) {
                return 1;
            }
            self->unlock_wait = 10;
            q_result_swap_start(self);
            self->unlock_text = q_result_unlock_next(self);
        }
        break;
    }
    return 0;
}

/* 0x80399BFC (0x90): Draws the phase's countdown (in seconds) unless the screen is closing. */
void q_result_timer_draw(QResultScreen* self)
{
    _mh_ivec2_ pos;
    char text[0x40];

    if (self->sub_state != 3) {
        get_lsp_data(0x11EF, &pos);
        draw_sprite_idx(0x11F0, &pos);
        sprintf(text, "%d", self->phase_timer / 30);
        draw_font_idx(0x11F1, (s8*)text, 2, &pos);
    }
}

/* 0x80399C8C (0xB8): Draws the screen's header: the title frame, the phase label and the option marker. */
void q_result_header_draw(QResultScreen* self)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x127C, &pos);
    draw_sprite_anim_ary(q_result_header_ids, 5, &pos);
    if (self->phase == 7) {
        draw_sprite_anim_idx(0x127F, 5, &pos);
    } else {
        draw_sprite_anim_idx(0x127E, 5, &pos);
    }
    if (get_option_cfg(7) == 0) {
        draw_sprite_anim_idx(0x1281, 5, &pos);
    } else {
        draw_sprite_anim_idx(0x1282, 5, &pos);
    }
    q_result_timer_draw(self);
}

/* 0x80399D44 (0x8): Draws the money panel at layout 0x11F2. */
void q_result_draw_icon_value(QResultScreen* self)
{
    menu_money_draw(0x11F2);
}

/* 0x80399D4C (0xA8): The message frame's rectangle: the anchor (widened by the wide-screen offset) and the width
 * of the narrow (`frame_kind` 0) or wide frame. */
void q_result_msg_frame_rect(u8 frame_kind, s16* x, s16* y, u16* width, u16* height)
{
    *x = 12;
    *y = 368;
    *x += get_wide_offset(2);
    if (width != NULL) {
        if (frame_kind == 0) {
            *width = 304;
        } else {
            *width = 400;
        }
    }
    if (height != NULL) {
        *height = 96;
    }
}

/* 0x80399DF4 (0xF0): Draws the message frame and its first line: message `row` of set 11, with `value` filled in
 * when `with_value` is set. */
void q_result_font_print_row(s16 row, u8 with_value, s32 value, u8 frame_kind)
{
    s16 x;
    s16 y;
    u16 width;
    u16 height;
    s16 text_x;
    s16 text_y;

    q_result_msg_frame_rect(frame_kind, &x, &y, &width, &height);
    draw_window_frame(x, y, width, height, 0x1E1C0DB2);
    font_set_size(18, 18);
    if (row >= 0) {
        text_x = x + 20;
        text_y = y + 16;
        if (with_value == 0) {
            font_print_ex(text_x, text_y, 0, (s8*)q_result_msg_string_alt(row));
        } else {
            font_print_ex(text_x, text_y, 0, (s8*)q_result_msg_string_alt(row), value);
        }
    }
}

/* 0x80399EE4 (0x18): Draws message `row` in the message frame `frame_kind`. */
void q_result_font_print_a(s16 row, u8 frame_kind)
{
    q_result_font_print_row(row, 0, 0, frame_kind);
}

/* 0x80399EFC (0x1C): Draws message `row` with `value` filled in, in the message frame `frame_kind`. */
void q_result_font_print_b(s16 row, s32 value, u8 frame_kind)
{
    q_result_font_print_row(row, 1, value, frame_kind);
}

/* 0x80399F18 (0xB8): Draws message `row` of set 11 on line `line` of the message frame in colour `color`. */
void q_result_font_print_line(s16 row, s16 color, s16 line)
{
    s16 y;
    s16 x;
    s16 text_x;
    s16 text_y;

    if (row > 0) {
        q_result_msg_frame_rect(0, &x, &y, NULL, NULL);
        text_x = x + 20;
        text_y = y + line * 20 + 16;
        font_set_size(18, 18);
        font_print_ex(text_x, text_y, color, (s8*)q_result_msg_string_alt(row));
    }
}

/* 0x80399FD0 (0x194): Draws the item info panel: the icon, name, rarity, count (red at the cap) and value. */
void q_result_item_info_draw(u16 item, s32 count, s32 max, _mh_ivec2_* unused, _mh_ivec2_* pos)
{
    ItemDataRecord* data = GetItemData(item);
    u32 color = get_rare_color(data->level_0x01);
    _SPR_DATA_ spr;
    char text[0x20];

    draw_sprite_ary(q_result_item_info_ids, pos);
    if (item != 0) {
        draw_font_idx(0x1205, (s8*)ItemName(item), 1, pos);
        spr_data_copy(&spr, get_lsp_data(0x1200, NULL));
        spr.color = color;
        draw_sprite(spr, pos);
        draw_number_idx(0x1201, data->level_0x01 + 1, color, pos);
        spr_data_copy(&spr, get_lsp_data(0x1206, NULL));
        if (count >= max && max != 0) {
            spr.color = 0xFF435DFF;
            sprintf(text, "%d", max);
        } else {
            sprintf(text, "%d", count);
        }
        draw_font(spr, (s8*)text, 1, pos);
        sprintf(text, "%dz", count * data->field_0x010);
        draw_font_idx(0x1203, (s8*)text, 2, pos);
        spr_data_copy(&spr, get_lsp_data(0x11FF, NULL));
        draw_itemicon_item_id(spr, item, pos);
    }
}

/* 0x8039A164 (0x84): Draws the box list row's counts for `item`: how many the box holds and how many the player
 * owns. */
void q_result_item_counts_draw(IdValue* item)
{
    _mh_ivec2_ pos;
    s16 boxed;
    s16 owned;

    if (item != NULL) {
        get_lsp_data(0x11EE, &pos);
        if (item->id != 0) {
            boxed = q_result_box_count_get(item->id);
            owned = q_result_owned_count_get(item->id);
        } else {
            boxed = 0;
            owned = 0;
        }
        menu_item_row_draw_values((u16*)item, &pos, boxed, owned);
    }
}

/* 0x8039A1E8 (0x150): Draws the equipment info panel (icon, rarity, name, price) and the free box slots left. */
void q_result_equip_info_draw(QResultScreen* self, _EQUIP* equip, _mh_ivec2_* pos)
{
    u8 rare;
    u32 color;
    _SPR_DATA_ spr;
    char text[0x10];
    u16 free;

    draw_sprite_ary(q_result_equip_info_ids, pos);
    if (equip != NULL && equip->item_id != 0) {
        draw_equipicon_idx(0x11FF, equip, pos);
        rare = Get_equip_rare(equip);
        color = get_rare_color(rare);
        spr_data_copy(&spr, get_lsp_data(0x1200, NULL));
        spr.color = color;
        draw_sprite(spr, pos);
        draw_number_idx(0x1201, rare + 1, color, pos);
        draw_font_idx(0x1205, (s8*)GetEquipName(equip->kind, equip->item_id), 1, pos);
        sprintf(text, "%dz", equip_sell_price_get(equip));
        draw_font_idx(0x1203, (s8*)text, 2, pos);
    }
    free = userdata_equip_box_free_count();
    sprintf(text, "%d", free - q_result_equip_sel_count(self));
    draw_font_idx(0x1204, (s8*)text, 2, pos);
}

/* 0x8039A338 (0x198): Draws the equipment detail panel for an equipment cell of kind 6..15: the kind's stat block,
 * the decoration slots, the name and the icon. */
void q_result_equip_detail_draw(QResultScreen* self, _EQUIP* equip)
{
    _PLW* plw = &((PlMoveWork*)get_move_work_adrs(2))[self->player_no].pl;
    _mh_ivec2_ pos;

    if (equip != NULL && (u32)(equip->kind - 6) <= 9) {
        get_lsp_data(0x7D5, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x88), &pos);
        switch (equip->kind) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
            equip_detail_melee_draw(plw, equip, 0, 1);
            equip_detail_slots_draw(plw, equip, 0, 1);
            break;
        case 11:
        case 12:
        case 13:
            equip_detail_bowgun_draw(plw, equip, 0, 1);
            equip_detail_slots_draw(plw, equip, 0, 1);
            break;
        case 6:
            equip_detail_kind6_draw(plw, equip, 0, 1);
            equip_detail_slots_draw(plw, equip, 0, 1);
            break;
        }
        get_lsp_data(0x7E2, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x89), &pos);
        get_lsp_data(0x7E6, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(6), &pos);
        draw_font_idx(0x13C, (s8*)GetEquipName(equip->kind, equip->item_id), 0, &pos);
        draw_equipicon_idx(0x13D, equip, &pos);
    }
}

/* 0x8039A4D0 (0x20C): Draws the box list the selected item can be swapped into: two rows of four slots around
 * the list cursor, the extra box's slots only for kind-1 items. */
void q_result_box_list_draw(QResultScreen* self, u8 which)
{
    IdValue* cell;
    ItemDataRecord* data;
    s32 flags;
    s16 index;
    u32 extra;
    IdValue* box;
    u8 enabled;
    MenuListEntry entries[8];
    MenuListEntry* entry;
    s16 col;
    s32 i;
    _mh_ivec2_ pos;

    cell = q_result_grid_entry(self, which);
    data = NULL;
    flags = 0;
    if (cell != NULL) {
        data = GetItemData(cell->id);
    }
    index = self->list_cursor_row * 8;
    if (index >= 0x18) {
        extra = 1;
        box = self->box_extra;
        flags |= 1;
        index -= 0x18;
    } else {
        extra = 0;
        box = self->box_main;
    }
    enabled = 1;
    if (extra == 1 && data != NULL && data->kind_0x00 != 1) {
        enabled = 0;
    }
    memset(entries, 0, sizeof(entries));
    entry = entries;
    col = 0;
    for (i = 0; i < 2; i++) {
        entry[0].present_0x02 = enabled;
        entry[0].field_0x01 = 0;
        if (col == self->list_cursor_col) {
            entry[0].field_0x01 = 1;
        } else {
            entry[0].field_0x01 = 0;
        }
        entry[0].index_0x06 = index;
        col++;
        index++;
        entry[1].present_0x02 = enabled;
        entry[1].field_0x01 = 0;
        if (col == self->list_cursor_col) {
            entry[1].field_0x01 = 1;
        } else {
            entry[1].field_0x01 = 0;
        }
        entry[1].index_0x06 = index;
        col++;
        index++;
        entry[2].present_0x02 = enabled;
        entry[2].field_0x01 = 0;
        if (col == self->list_cursor_col) {
            entry[2].field_0x01 = 1;
        } else {
            entry[2].field_0x01 = 0;
        }
        entry[2].index_0x06 = index;
        col++;
        index++;
        entry[3].present_0x02 = enabled;
        entry[3].field_0x01 = 0;
        if (col == self->list_cursor_col) {
            entry[3].field_0x01 = 1;
        } else {
            entry[3].field_0x01 = 0;
        }
        entry[3].index_0x06 = index;
        col++;
        entry += 4;
        index++;
    }
    if (self->page_moved & 4) {
        flags |= 8;
    }
    if (self->page_moved & 8) {
        flags |= 0x10;
    }
    get_lsp_data(0x274, &pos);
    pos.y += 30;
    menu_frame_draw_blocks(box, entries, self->list_cursor_row + 1, self->list_rows, 0, flags, &pos);
    q_result_item_counts_draw(q_result_list_entry(self));
}

/* 0x8039A6DC (0x150): Draws the info panel for the cell under the grid cursor (blank while the grid is inactive). */
void q_result_info_panel_draw(QResultScreen* self, u8 which, u32 active)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    u16* ids;
    u16 lsp;
    IdValue* cell;

    sprite_frame_apply(&spr, 0x11F3, self->open_frame, &anchor);
    if (which != 2) {
        ids = q_result_panel_anim_a;
        lsp = 0x11F7;
    } else {
        ids = q_result_panel_anim_b;
        lsp = 0x11F8;
    }
    draw_sprite_anim_ary(ids, self->open_frame, &anchor);
    get_lsp_data(lsp, &pos);
    pos.x += anchor.x;
    pos.y += anchor.y;
    if (which != 3) {
        if (active == 1) {
            cell = q_result_grid_entry(self, which);
            if (cell != NULL) {
                q_result_item_info_draw(cell->id, cell->value, GetItemData(cell->id)->max_num_0x003, NULL, &pos);
            }
        } else {
            q_result_item_info_draw(0, 0, 0, NULL, &pos);
        }
    } else {
        q_result_equip_info_draw(self, (active == 1) ? q_result_equip_cell_get(self, which) : NULL, &pos);
    }
}

/* 0x8039A82C (0xB0): Draws the grid's tab: the plain, kept or equipment label. */
void q_result_tab_draw(QResultScreen* self, u8 which)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ anchor;
    u16* ids;
    u8 frame;

    sprite_frame_apply(&spr, 0x120B, self->open_frame, &anchor);
    switch (which) {
    default:
        ids = (q_result_sub_screen_ready(self) == 1) ? q_result_tab_ids_b : q_result_tab_ids_a;
        frame = 0;
        break;
    case 1:
        ids = q_result_tab_ids_b;
        frame = 1;
        break;
    case 2:
        ids = q_result_tab_ids_c;
        frame = 2;
        break;
    }
    draw_sprite_ary(ids, &anchor);
    draw_sprite_anim_ary(q_result_tab_anim, frame, &anchor);
}

/* 0x8039A8DC (0x14C): Draws the equipment grid's tab and its "take" command row (lit while cells are marked). */
void q_result_equip_tab_draw(QResultScreen* self, bool active)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    bool cursor;
    bool marked;

    sprite_frame_apply(&spr, 0x121E, self->open_frame, &anchor);
    draw_sprite_ary(q_result_equip_tab_ids, &anchor);
    draw_sprite_ary(q_result_equip_tab_frame, &anchor);
    get_lsp_data(0x122C, &pos);
    pos.x += anchor.x;
    pos.y += anchor.y;
    draw_sprite_ary(q_result_equip_tab_label, &pos);
    cursor = 0;
    if (self->select_mode == 3 && self->grid_cursor_row == self->grid_rows - 1) {
        put_menu_cursor(q_result_equip_cursor_ids, 0, &pos);
        cursor = 1;
    }
    marked = q_result_equip_sel_count(self) != 0;
    spr_data_copy(&spr, get_lsp_data(0x77, NULL));
    spr.color = GetMenuFontColor(marked, cursor, active, 0);
    draw_font(spr, (s8*)q_result_msg_entry(8, 0), 1, &pos);
}

/* 0x8039AA28 (0xD0): Draws a grid cell's frame layers: the marked backdrop, the cursor blink and the moved flash. */
void q_result_cell_frame_draw(QResultScreen* self, _mh_ivec2_* pos, u32 cursor, u32 marked, u32 moved)
{
    if (marked == 1) {
        draw_sprite_idx(0x1234, pos);
    }
    set_blendmode(4, 1, 1);
    if (cursor == 1) {
        draw_sprite_anim_idx(0x122F, self->cursor_blink, pos);
    }
    if (moved == 1) {
        draw_sprite_anim_idx(0x1230, self->moved_anim, pos);
    }
    set_blendmode(4, 5, 1);
    if (moved == 1 && self->moved_item.id != 0) {
        draw_itemicon_anim_idx(0x1232, self->moved_anim, self->moved_item.id, pos);
    }
}

/* 0x8039AAF8 (0x70): Draws a grid cell's top layers: the moved flash and the waiting marker. */
void q_result_cell_marks_draw(QResultScreen* self, _mh_ivec2_* pos, u32 cursor, u32 marked, u32 moved, u32 waiting)
{
    if (moved == 1) {
        draw_sprite_anim_idx(0x1231, self->moved_anim, pos);
    }
    if (waiting == 1) {
        draw_sprite_anim_idx(0x1233, self->equip_wait, pos);
    }
}

/* 0x8039AB68 (0xB0): Draws one item cell: the frame, the item icon, the marks. */
void q_result_item_cell_draw(QResultScreen* self, _mh_ivec2_* pos, u16 item, u32 cursor, u32 marked, u32 moved,
                             u32 waiting)
{
    _SPR_DATA_ spr;

    q_result_cell_frame_draw(self, pos, cursor, marked, moved);
    if (item != 0) {
        spr_data_copy(&spr, get_lsp_data(0x1232, NULL));
        draw_itemicon_item_id(spr, item, pos);
    } else {
        waiting = 0;
    }
    q_result_cell_marks_draw(self, pos, cursor, marked, moved, waiting);
}

/* 0x8039AC18 (0xB0): Draws one equipment cell: the frame, the equipment icon, the marks. */
void q_result_equip_cell_draw(QResultScreen* self, _mh_ivec2_* pos, _EQUIP* equip, u32 cursor, u32 marked, u32 moved,
                              u32 waiting)
{
    _SPR_DATA_ spr;

    q_result_cell_frame_draw(self, pos, cursor, marked, moved);
    if (equip->kind != 0) {
        spr_data_copy(&spr, get_lsp_data(0x1232, NULL));
        draw_equipicon(spr, equip, pos);
    } else {
        waiting = 0;
    }
    q_result_cell_marks_draw(self, pos, cursor, marked, moved, waiting);
}

/* 0x8039ACC8 (0x2F4): Draws the grid `which` names, eight cells a row, `grid_rows` rows (one less for the
 * equipment grid, whose last row is the command row). */
void q_result_grid_draw(QResultScreen* self, u8 which, u32 active)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ anchor;
    _mh_ivec2_ row_pos;
    _mh_ivec2_ pos;
    IdValue* items = NULL;
    _EQUIP* equip = NULL;
    u32 waiting = 0;
    s32 mode = 0;
    u16* row_lsp;
    u8 rows;
    u32 moved_live;
    u16 index;
    u8 row;
    u8 col;
    u32 cursor;
    u32 marked;
    u32 moved;
    IdValue* item;

    sprite_frame_apply(&spr, 0x120B, self->open_frame, &anchor);
    rows = self->grid_rows;
    switch (which) {
    default:
        row_lsp = (q_result_sub_screen_ready(self) == 1) ? q_result_kept_row_lsp : q_result_grid_row_lsp;
        items = self->grid_items;
        break;
    case 1:
        row_lsp = q_result_kept_row_lsp;
        items = self->grid_kept;
        break;
    case 3:
        row_lsp = q_result_equip_row_lsp;
        switch (self->select_mode) {
        case 0:
            mode = 1;
            break;
        case 1:
            mode = 2;
            waiting = 1;
            break;
        default:
            mode = 3;
            break;
        }
        if (self->equip_ready != 0) {
            equip = self->equip_cells;
        }
        rows--;
        break;
    }
    moved_live = self->moved_anim != 0xFFFF;
    index = 0;
    for (row = 0; row < rows; row++) {
        get_lsp_data(row_lsp[row], &row_pos);
        row_pos.x += anchor.x;
        row_pos.y += anchor.y;
        pos.y = row_pos.y;
        for (col = 0; col < 8; col++) {
            pos.x = row_pos.x + col * 31;
            marked = 0;
            moved = 0;
            cursor = 0;
            if (active == 1 && self->grid_cursor_col == col && self->grid_cursor_row == row) {
                cursor = 1;
            }
            if (moved_live == 1 && self->moved_col == col && self->moved_row == row) {
                moved = 1;
            }
            if (self->equip_ready == 0) {
                switch (mode) {
                case 1:
                    items = q_result_delivered_at(self, index);
                    break;
                case 2:
                    items = q_result_delivered_at(self, index);
                    break;
                }
            }
            if (items != NULL) {
                if (mode == 0) {
                    item = &items[index];
                } else {
                    item = items;
                }
                q_result_item_cell_draw(self, &pos, item->id, cursor, 0, moved, waiting);
            } else if (equip != NULL) {
                if ((1 << (index & 0xF)) & self->equip_sel_bits[index >> 4]) {
                    marked = 1;
                }
                q_result_equip_cell_draw(self, &pos, &equip[index], cursor, marked, moved, waiting);
            }
            index++;
        }
    }
}

/* 0x8039AFBC (0x774): Draws an item page (`mode` 0..2) or the equipment page (3): the command rows, the info panel,
 * the grid, and the help line the state calls for. */
void q_result_page_set(QResultScreen* self, u8 mode)
{
    char** lines;
    s32 show_cmds;
    s32 help;
    s32 note;
    u8 note_line;
    s32 with_value;
    s32 value;
    u32 list_mode;
    u32 left;
    u16 frame_id;
    _SPR_DATA_ spr;
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    u16* rows;
    s32 i;
    bool enabled;
    bool selected;
    bool active;
    IdValue* cell;
    ItemDataRecord* data;
    s16 fit;
    _EQUIP* equip;
    s16 index;

    help = -1;
    note = -1;
    list_mode = 0;
    with_value = 0;
    note_line = 1;
    if (self->open_frame >= 5) {
        q_result_timer_draw(self);
        q_result_draw_icon_value(self);
    }
    switch (mode) {
    default:
        left = q_result_items_left_ck(self, mode);
        switch (self->select_mode) {
        case 1:
            show_cmds = 0;
            break;
        case 3:
            list_mode = 1;
            break;
        default:
            show_cmds = 1;
            if (left == 1) {
                lines = q_result_msg_table_alt(3);
            } else {
                lines = q_result_msg_table_alt(4);
            }
            break;
        }
        break;
    case 3:
        left = q_result_items_left_ck(self, mode);
        switch (self->select_mode) {
        case 0:
            show_cmds = 1;
            lines = q_result_msg_table(7);
            break;
        case 1:
            show_cmds = 1;
            break;
        case 2:
        case 4:
            show_cmds = 1;
            if (left == 1) {
                lines = q_result_msg_table(5);
            } else {
                lines = q_result_msg_table(6);
            }
            break;
        default:
            show_cmds = 0;
            break;
        }
        break;
    }
    if (list_mode == 1) {
        q_result_box_list_draw(self, mode);
        data = GetItemData(q_result_grid_entry(self, mode)->id);
        if ((s16)q_result_list_cursor_index(self) >= 0x18 && data->kind_0x00 != 1) {
            note = 0x11;
        }
        q_result_font_print_a(7, 1);
        q_result_font_print_line(note, 2, 1);
        return;
    }
    switch (mode) {
    default:
        frame_id = 0x11D9;
        break;
    case 1:
        frame_id = 0x11DA;
        break;
    case 2:
        frame_id = 0x11DB;
        break;
    case 3:
        frame_id = 0x11DC;
        break;
    }
    if (show_cmds != 0) {
        sprite_frame_apply(&spr, 0x11D5, self->open_frame, &anchor);
        draw_sprite_anim_ary(q_result_cmd_frame_anim, self->open_frame, &anchor);
        draw_sprite_anim_idx(frame_id, self->open_frame, &anchor);
        if (self->open_frame >= 7) {
            switch (self->cursor_count) {
            default:
                rows = q_result_cmd_rows_4;
                break;
            case 2:
                rows = q_result_cmd_rows_2;
                break;
            case 1:
                rows = q_result_cmd_rows_1;
                break;
            }
            for (i = 0; i < self->cursor_count; i++, rows++, lines++) {
                enabled = TRUE;
                get_lsp_data(*rows, &pos);
                pos.x += anchor.x;
                pos.y += anchor.y;
                draw_sprite_ary(q_result_cmd_frame_ids, &pos);
                if (i == self->cursor) {
                    put_menu_cursor(q_result_cmd_cursor_ids, 0, &pos);
                    selected = TRUE;
                } else {
                    selected = FALSE;
                }
                switch (mode) {
                case 0:
                case 1:
                case 2:
                    if ((u32)i <= 1 && left == 0) {
                        enabled = FALSE;
                    }
                    break;
                case 3:
                    switch (self->select_mode) {
                    case 2:
                    case 4:
                        if (i == 0 && q_result_items_left_ck(self, mode) == 0) {
                            enabled = FALSE;
                        }
                        break;
                    }
                    break;
                }
                spr_data_copy(&spr, get_lsp_data(0x11E8, NULL));
                spr.color = GetMenuFontColor(enabled, selected, 1, 0);
                draw_font(spr, (s8*)*lines, 0, &pos);
            }
        }
        switch (mode) {
        case 0:
        case 1:
        case 2:
            switch (self->select_mode) {
            case 2:
                help = 6;
                with_value = 1;
                value = self->sell_total;
                break;
            default:
                switch (self->cursor) {
                case 0:
                    help = 0;
                    break;
                case 1:
                    help = 1;
                    break;
                case 2:
                    if (left == 1) {
                        help = 2;
                        if (q_result_delivery_left_ck(self, mode) == 1) {
                            note_line = 2;
                            note = 0x12;
                        }
                    } else {
                        help = 3;
                    }
                    break;
                }
                break;
            }
            break;
        case 3:
            switch (self->select_mode) {
            case 0:
                help = 8;
                break;
            case 1:
                help = 9;
                break;
            case 2:
                switch (self->cursor) {
                case 0:
                    note_line = 2;
                    help = 10;
                    if (q_result_items_left_ck(self, mode) == 1 && userdata_equip_box_free_count() == 0) {
                        note = 0x10;
                    }
                    break;
                case 1:
                    if (left == 1) {
                        help = 2;
                    } else {
                        help = 3;
                    }
                    break;
                }
                break;
            case 4:
                help = 6;
                with_value = 1;
                value = self->sell_total;
                break;
            }
            break;
        }
        active = 0;
    } else {
        active = 1;
        switch (mode) {
        case 0:
        case 1:
        case 2:
            help = (self->sell_mode == 0) ? 4 : 5;
            note_line = 2;
            break;
        case 3:
            help = (self->grid_cursor_row == self->grid_rows - 1) + 11;
            note_line = 2;
            break;
        }
    }
    q_result_info_panel_draw(self, mode, active);
    if (mode == 3) {
        q_result_equip_tab_draw(self, active);
    } else {
        q_result_tab_draw(self, mode);
    }
    if (show_cmds == 0) {
        q_result_grid_draw(self, mode, active);
        cell = q_result_grid_entry(self, mode);
        if (cell != NULL) {
            q_result_item_counts_draw(cell);
            if (cell->id != 0) {
                if (GetItemData(cell->id)->field_0x002 & 0x10) {
                    help = 13;
                } else if (self->sell_mode == 0) {
                    switch (q_result_box_fit_ck(cell->id, cell->value, &fit)) {
                    case 1:
                        note = 0xE;
                        break;
                    case 3:
                        note = 0xF;
                        break;
                    }
                } else if (item_slots_room_get(cell->id, ((Q_UserData*)lobby_world_block)->box_0x180,
                                               userdata_box_capacity(lobby_world_block)) <= 0) {
                    note = 0x10;
                }
            }
        } else if (self->grid_cursor_row == self->grid_rows - 1) {
            if (q_result_equip_sel_count(self) == 0) {
                note = 0x13;
            }
        } else {
            equip = q_result_equip_cell_get(self, mode);
            if (equip != NULL) {
                q_result_equip_detail_draw(self, equip);
                index = q_result_grid_cursor_index(self);
                if (equip->kind != 0 && !((1 << (index & 0xF)) & self->equip_sel_bits[index >> 4])) {
                    if (userdata_equip_box_free_count() - q_result_equip_sel_count(self) <= 0) {
                        note = 0x10;
                    }
                }
            }
        }
    } else {
        q_result_grid_draw(self, mode, active);
    }
    if (with_value == 0) {
        q_result_font_print_a(help, 0);
    } else {
        q_result_font_print_b(help, value, 0);
        menu_yes_no_draw(0x1236, self->sell_confirm);
    }
    if (note == 0x12) {
        q_result_font_print_line(note, 5, note_line);
        return;
    }
    q_result_font_print_line(note, 2, note_line);
}

/* 0x8039B730 (0x280): Draws the arena time page (the record rows and the clear time), then the item page over it. */
void q_result_time_page_draw(QResultScreen* self)
{
    QuestRecord* rec = quest_record_get();
    Q_ResultWork* q = get_qResult_work();
    _SPR_DATA_ spr;
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    char text[0x40];
    static u8 q_result_time_rank_kind[3] = {0, 1, 2};
    QResultTextBox* box;
    s32 i;
    s16 minutes;
    s16 seconds;
    s32 elapsed;

    if (q_result_file_ready(self) == 1) {
        sprite_frame_apply(&spr, 0x121B, self->open_frame, &anchor);
        draw_window_frame_style(anchor.x, anchor.y, 0x118, 0x96, 0);
        for (i = 0, box = q_result_time_rows; i < 4; box++, i++) {
            draw_font_order(&box->pos, box->width, box->height, box->color, (s8*)q_result_msg_entry(9, i), box->flags,
                            &anchor);
            if ((u32)((s16)i - 1) <= 2) {
                pos.x = box->pos.x + 0x78;
                pos.y = box->pos.y;
                sprintf(text, "%s", quest_arena_time_text_get(rec, q_result_time_rank_kind[i - 1]));
                strcat(text, q_result_msg_string(15));
                draw_font_order(&pos, box->width, box->height, 0xC0C0C0FF, (s8*)text, 0, &anchor);
            }
        }
        font_flush();
        sprite_frame_apply(&spr, 0x121C, self->open_frame, &anchor);
        draw_window_frame_style(anchor.x, anchor.y, 0x118, 0x1B, 0);
        draw_font_order(&q_result_time_total_row.pos, q_result_time_total_row.width, q_result_time_total_row.height,
                        q_result_time_total_row.color, (s8*)q_result_msg_string(13), q_result_time_total_row.flags,
                        &anchor);
        elapsed = q->elapsed_0x148;
        minutes = elapsed / (s32)(60.0f * Screen_w.frame_scale);
        seconds = (s16)(elapsed % (s32)(60.0f * Screen_w.frame_scale)) / (s32)Screen_w.frame_scale;
        pos.x = q_result_time_total_row.pos.x + 0x96;
        pos.y = q_result_time_total_row.pos.y;
        sprintf(text, q_result_msg_string(14), minutes, seconds);
        draw_font_order(&pos, q_result_time_total_row.width, q_result_time_total_row.height, 0xC0C0C0FF, (s8*)text, 0,
                        &anchor);
        q_result_page_set(self, 2);
        return;
    }
    q_result_page_set(self, 0);
}

/* 0x8039B9B0 (0x8): Draws the kept-items page. */
void q_result_page_step_a(QResultScreen* self)
{
    q_result_page_set(self, 1);
}

/* 0x8039B9B8 (0x8): Draws the equipment page. */
void q_result_page_step_b(QResultScreen* self)
{
    q_result_page_set(self, 3);
}

/* 0x8039B9C0 (0x638): Draws the reward page the cursor is on: the totals, the quest rewards, the hunted monsters
 * or the kept items, with the page total and the arrows. */
void q_result_reward_page_draw(QResultScreen* self)
{
    s32 total = 0;
    Q_UserData* user = get_userdata();
    _mh_ivec2_ row_pos;
    _mh_ivec2_ anchor;
    char count_text[0x10];
    char text[0x40];
    _SPR_DATA_ label;
    _SPR_DATA_ count_spr;
    _SPR_DATA_ value_spr;
    s32 i;
    u16* lsp;
    s32 value;
    u32 bonus;
    s32* zenny;
    IdValue* item;
    ItemDataRecord* data;

    q_result_header_draw(self);
    get_lsp_data(0x1237, &anchor);
    draw_sprite_idx(q_result_reward_page_ids[self->cursor], &anchor);
    spr_data_copy(&value_spr, get_lsp_data(0x1250, NULL));
    value_spr.pos.x += 38;
    spr_data_copy(&count_spr, get_lsp_data(0x124F, NULL));
    switch (self->cursor) {
    case 0:
        for (i = 0, lsp = q_result_reward_row_lsp; i < 4; i++, lsp++) {
            switch (i) {
            case 0:
                value = self->zenny_total;
                total += value;
                break;
            case 1:
                value = self->hunt_zenny_total;
                total += value;
                break;
            case 2:
                value = self->item_value_total;
                total += value;
                break;
            case 3:
                value = total;
                break;
            default:
                continue;
            }
            spr_data_copy(&label, get_lsp_data(*lsp, &row_pos));
            label.pos.x -= 34;
            draw_font(label, (s8*)q_result_msg_entry(0, i), 0, &anchor);
            value_spr.pos.y = row_pos.y;
            sprintf(text, "%dz", value);
            draw_font(value_spr, (s8*)text, 2, &anchor);
        }
        break;
    case 1:
        for (i = 0, lsp = q_result_zenny_row_lsp; i < 5; i++) {
            spr_data_copy(&label, get_lsp_data(*lsp, &row_pos));
            label.pos.x -= 34;
            switch (i) {
            case 0:
                value = self->zenny_rows[0];
                total += value;
                bonus = 0;
                break;
            case 1:
                if (self->reward_shown[0] != 1) {
                    continue;
                }
                value = self->zenny_rows[1];
                total += value;
                bonus = 1;
                break;
            case 2:
                if (self->reward_shown[1] != 1) {
                    continue;
                }
                value = self->zenny_rows[2];
                total += value;
                bonus = 1;
                break;
            case 3:
                if (self->reward_shown[2] != 1) {
                    continue;
                }
                value = self->zenny_rows[3];
                total += value;
                bonus = 1;
                break;
            case 4:
                if (self->skill_zenny == 1) {
                    draw_font(label, (s8*)q_result_msg_string(20), 0, &anchor);
                }
                continue;
            default:
                continue;
            }
            sprintf(text, q_result_msg_entry(1, i));
            if (bonus == 1 && self->reward_cut == 1) {
                strcat(text, q_result_msg_string(0));
            }
            draw_font(label, (s8*)text, 0, &anchor);
            value_spr.pos.y = row_pos.y;
            sprintf(text, "%dz", value);
            draw_font(value_spr, (s8*)text, 2, &anchor);
            lsp++;
        }
        break;
    case 2:
        for (i = 0, zenny = self->hunt_count, lsp = q_result_hunt_row_lsp; i < 2; i++, zenny++) {
            if (zenny[0] != 0) {
                sprintf(text, "%s", str_tbl_33_get(self->hunt_monster[i]));
                sprintf(count_text, q_result_msg_string(9), zenny[0]);
                value = zenny[2] * zenny[0];
                total += value;
                spr_data_copy(&label, get_lsp_data(*lsp, &row_pos));
                label.pos.x -= 34;
                draw_font(label, (s8*)text, 0, &anchor);
                count_spr.pos.y = row_pos.y;
                draw_font(count_spr, (s8*)count_text, 0, &anchor);
                value_spr.pos.y = row_pos.y;
                sprintf(text, "%dz", value);
                draw_font(value_spr, (s8*)text, 2, &anchor);
                lsp++;
            }
        }
        break;
    case 3:
        for (i = 0, item = self->kept_items, lsp = q_result_item_row_lsp; i < self->kept_item_count;
             i++, item++, lsp++) {
            data = GetItemData(item->id);
            sprintf(text, "%s", (char*)ItemName(item->id));
            sprintf(count_text, q_result_msg_string(9), item->value);
            value = data->field_0x010 * item->value;
            total += value;
            spr_data_copy(&label, get_lsp_data(*lsp, &row_pos));
            label.pos.x -= 34;
            draw_font(label, (s8*)text, 0, &anchor);
            count_spr.pos.y = row_pos.y;
            draw_font(count_spr, (s8*)count_text, 0, &anchor);
            value_spr.pos.y = row_pos.y;
            sprintf(text, "%dz", value);
            draw_font(value_spr, (s8*)text, 2, &anchor);
        }
        break;
    }
    spr_data_copy(&label, get_lsp_data(0x124E, NULL));
    label.pos.x -= 24;
    if (self->cursor == 0) {
        draw_font(label, (s8*)q_result_msg_string(10), 0, &anchor);
        sprintf(text, "%dz", user->zenny_0x18);
    } else {
        draw_font(label, (s8*)q_result_msg_string(8), 0, &anchor);
        sprintf(text, "%dz", total);
    }
    spr_data_copy(&label, get_lsp_data(0x1251, NULL));
    label.pos.x += 24;
    draw_font(label, (s8*)text, 2, &anchor);
    PutPageArrow(q_result_page_arrow_ids, self->cursor, self->cursor_count, self->page_moved, &anchor, 0);
}

/* 0x8039BFF8 (0x734): Draws the hunter points page: the quest points, the skill note, the hunted monsters, the
 * faint penalty and the total, then the hunter points and the rank (with its rank-up animation). */
void q_result_points_page_draw(QResultScreen* self)
{
    s32 total = 0;
    Q_UserData* user = get_userdata();
    s32 skill_shown = 0;
    s32 faint_shown = 0;
    u8 hunt = 0;
    u32 normal;
    _mh_ivec2_ anchor;
    _mh_ivec2_ row_pos;
    _mh_ivec2_ frame_pos;
    _SPR_DATA_ label;
    _SPR_DATA_ count_spr;
    _SPR_DATA_ value_spr;
    char count_text[0x10];
    char text[0x40];
    u8 hunt_done[2];
    u8 i;
    s32 draw_values;
    s32 have;
    u8 count;
    s32 value;
    u16* lsp;
    s32* points;
    u16* total_lsp;
    u16 rank;
    u16 frame;

    normal = q_result_phase_is_2(self) != 1;
    q_result_header_draw(self);
    get_lsp_data(0x1237, &anchor);
    get_lsp_data(0x1252, &frame_pos);
    draw_sprite_ary(q_result_points_frame_ids, &frame_pos);
    spr_data_copy(&value_spr, get_lsp_data(0x1250, NULL));
    spr_data_copy(&count_spr, get_lsp_data(0x124F, NULL));
    value_spr.pos.x += 28;
    hunt_done[0] = 0;
    hunt_done[1] = 0;
    lsp = q_result_item_row_lsp;
    points = self->point_rows;
    total_lsp = q_result_item_row_lsp;
    for (i = 0; i < 8; i++, points++, total_lsp++) {
        draw_values = 1;
        have = 0;
        count = 0;
        if (i <= 6) {
            spr_data_copy(&label, get_lsp_data(*lsp, &row_pos));
            label.pos.x -= 20;
            if (normal == 0) {
                if (i == 0) {
                    sprintf(text, "%s", q_result_msg_string(7));
                    if (self->reward_cut == 1) {
                        strcat(text, q_result_msg_string(0));
                    }
                    value = self->point_rows[1] + self->point_rows[2] + self->point_rows[0];
                    total += value;
                    have = 1;
                }
            } else if (i <= 2) {
                if (self->reward_shown[i] != 1) {
                    continue;
                }
                sprintf(text, "%s", q_result_msg_entry(2, i));
                if (self->reward_cut == 1) {
                    strcat(text, q_result_msg_string(0));
                }
                value = points[0];
                total += value;
                have = 1;
            }
            if (have == 0) {
                if (skill_shown == 0 && self->skill_points == 1) {
                    sprintf(text, q_result_msg_string(20));
                    skill_shown = 1;
                    draw_values = 0;
                } else if (normal != 0 && hunt < 2 && hunt_done[hunt] == 0 && self->hunt_count[hunt] != 0) {
                    hunt_done[hunt] = 1;
                    count = self->hunt_count[hunt];
                    sprintf(text, "%s", str_tbl_33_get(self->hunt_monster[hunt]));
                    sprintf(count_text, q_result_msg_string(9), count);
                    value = count * self->hunt_points[hunt];
                    total += value;
                    hunt++;
                } else if (faint_shown == 0 && self->faint_count > 0) {
                    sprintf(text, q_result_msg_string(5));
                    value = self->faint_count * -20;
                    total += value;
                    faint_shown = 1;
                } else {
                    continue;
                }
            }
        } else if (i == 7) {
            spr_data_copy(&label, get_lsp_data(*total_lsp, &row_pos));
            label.pos.x -= 20;
            sprintf(text, "%s", q_result_msg_string(8));
            value = total;
        } else {
            continue;
        }
        draw_font(label, (s8*)text, 0, &anchor);
        if (draw_values != 0) {
            if (count != 0) {
                count_spr.pos.y = row_pos.y;
                draw_font(count_spr, (s8*)count_text, 0, &anchor);
            }
            value_spr.pos.y = row_pos.y;
            sprintf(text, "%d", value);
            draw_font(value_spr, (s8*)text, 2, &anchor);
        }
        lsp++;
    }
    draw_font_idx(0x124E, (s8*)q_result_msg_string(1), 0, &anchor);
    sprintf(text, "%d", user->hunter_points_0x3DE0);
    draw_font_idx(0x1251, (s8*)text, 2, &anchor);
    if (user->hunter_rank_0x3DE4 > self->rank_before && (self->rank_state == 0 || self->rank_anim < 40)) {
        draw_sprite_anim_ary(q_result_rank_up_ids, self->rank_anim, &frame_pos);
        draw_font_anim_idx(0x1255, self->rank_anim, (s8*)q_result_msg_string(12), 5, &frame_pos);
    } else {
        switch (self->rank_state) {
        case 0:
            break;
        case 1:
            draw_sprite_ary(q_result_rank_ids, &frame_pos);
            draw_font_idx(0x1254, (s8*)q_result_msg_string(4), 5, &frame_pos);
            break;
        case 2:
            draw_sprite_ary(q_result_rank_ids, &frame_pos);
            draw_font_idx(0x1254, (s8*)q_result_msg_string(11), 5, &frame_pos);
            break;
        default:
            draw_sprite_ary(q_result_rank_ids, &frame_pos);
            draw_font_idx(0x1254, (s8*)q_result_msg_string(3), 5, &frame_pos);
            break;
        }
    }
    draw_font_idx(0x1256, (s8*)q_result_msg_string(2), 0, &frame_pos);
    rank = user->hunter_rank_0x3DE4;
    if (rank > self->rank_before) {
        frame = self->rank_anim % 10;
    } else {
        frame = 0;
    }
    if (rank < 10) {
        sprintf(text, "%d", rank);
        draw_font_anim_idx(0x125D, frame, (s8*)text, 0, &frame_pos);
        return;
    }
    if (rank < 100) {
        sprintf(text, "%d", rank / 10);
        draw_font_anim_idx(0x125F, frame, (s8*)text, 0, &frame_pos);
        sprintf(text, "%d", user->hunter_rank_0x3DE4 % 10);
        draw_font_anim_idx(0x125E, frame, (s8*)text, 0, &frame_pos);
        return;
    }
    sprintf(text, "%d", rank / 100);
    draw_font_anim_idx(0x1260, frame, (s8*)text, 0, &frame_pos);
    sprintf(text, "%d", (user->hunter_rank_0x3DE4 % 100) / 10);
    draw_font_anim_idx(0x1261, frame, (s8*)text, 0, &frame_pos);
    sprintf(text, "%d", user->hunter_rank_0x3DE4 % 10);
    draw_font_anim_idx(0x1262, frame, (s8*)text, 0, &frame_pos);
}

/* 0x8039C72C (0x24C): Draws one size row: the monster's icon and name, its largest and smallest size (or the
 * "none" text) with their crown marks, and the row's record label. */
void q_result_size_row_draw(QResultScreen* self, u8 which, u8 monster, u16 size_max, u16 size_min, _mh_ivec2_* pos)
{
    char text[0x40];
    f32 size;

    draw_sprite_ary(q_result_size_row_ids, pos);
    draw_monstericon_idx(0x126D, monster, pos);
    draw_font_idx(0x1274, str_tbl_33_get(monster), 0, pos);
    draw_font_idx(0x1275, (s8*)q_result_msg_string(16), 0, pos);
    if (size_max == 0) {
        sprintf(text, "%s", (char*)get_str_tbl(0x44)[2]);
    } else {
        monster_size_value_get(monster, size_max, &size);
        sprintf(text, q_result_msg_string(18), size);
    }
    strcat(text, q_result_msg_string(19));
    draw_font_idx(0x1276, (s8*)text, 0, pos);
    switch (monster_size_crown_get(monster, size_max)) {
    case 1:
        draw_sprite_uv_idx(0x1270, 27, pos);
        break;
    case 2:
        draw_sprite_uv_idx(0x1270, 28, pos);
        break;
    }
    draw_font_idx(0x1277, (s8*)q_result_msg_string(17), 0, pos);
    if (size_min == 0) {
        sprintf(text, "%s", (char*)get_str_tbl(0x44)[2]);
    } else {
        monster_size_value_get(monster, size_min, &size);
        sprintf(text, q_result_msg_string(18), size);
    }
    strcat(text, q_result_msg_string(19));
    draw_font_idx(0x1278, (s8*)text, 0, pos);
    if (monster_size_crown_get(monster, size_min) == 3) {
        draw_sprite_uv_idx(0x1271, 27, pos);
    }
    if (which == 0) {
        draw_sprite_idx(0x1272, pos);
        return;
    }
    draw_sprite_idx(0x1273, pos);
}

/* 0x8039C978 (0x1B0): Draws the size page for the row under the cursor: the save's record and the hunt's sizes,
 * the page arrows and the new-record flash. */
void q_result_size_page_draw(QResultScreen* self)
{
    QResultHuntRec* rec = &self->hunt_recs[self->cursor];
    Q_UserData* user = get_userdata();
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u8 monster;

    q_result_header_draw(self);
    get_lsp_data(0x1263, &anchor);
    draw_sprite_ary(q_result_size_frame_ids, &anchor);
    monster = rec->monster;
    get_lsp_data(0x1267, &pos);
    pos.x += anchor.x;
    pos.y += anchor.y;
    q_result_size_row_draw(self, 0, monster, user->sizes_0x3BC0[monster].size_max, user->sizes_0x3BC0[monster].size_min,
                           &pos);
    get_lsp_data(0x1268, &pos);
    pos.x += anchor.x;
    pos.y += anchor.y;
    q_result_size_row_draw(self, 1, monster, rec->size_max, rec->size_min, &pos);
    if (self->cursor_count < 10) {
        PutPageArrow(q_result_page_arrow_ids, self->cursor, self->cursor_count, self->page_moved, &anchor, 0);
    } else {
        PutPageArrow(q_result_page_arrow_ids, self->cursor, self->cursor_count, self->page_moved, &anchor, 1);
    }
    font_flush();
    if (rec->new_flags != 0) {
        sprite_frame_apply(&spr, 0x1265, self->hunt_anim[self->cursor], NULL);
        spr.pos.x -= 40;
        draw_sprite(spr, &anchor);
    }
}

/* 0x8039CB28 (0x110): Draws the unlock notice page: the frame and, when one is on show, the notice text centred
 * in it. */
void q_result_unlock_page_draw(QResultScreen* self)
{
    _mh_ivec2_ pos;
    s16 lines;
    char text[0x80];

    q_result_header_draw(self);
    get_lsp_data(0x1279, &pos);
    draw_sprite_idx(0x127A, &pos);
    if (self->unlock_text != 0xFF) {
        strcpy(text, q_result_unlock_text_tbl[system_w.field_0x09][self->unlock_text]);
        font_set_size(22, 22);
        pos.x = menu_text_block_center_x(text, (s16)(pos.x + 291), 22, &lines);
        pos.y += (s16)(190 - (s32)(0.5f * (22.0f * lines)));
        font_print_ex(pos.x, pos.y, 1, (s8*)text);
    }
}

/* 0x8039CC38 (0xE0): Draws the page the running phase shows. */
void q_result_draw(QResultScreen* self)
{
    get_qResult_work();
    if (self->sub_state != 3) {
        switch (self->phase) {
        case 1:
            if (self->sub_state == 1) {
                q_result_time_page_draw(self);
            }
            break;
        case 2:
            if (self->sub_state == 1) {
                q_result_page_step_a(self);
            }
            break;
        case 3:
            if (self->sub_state == 1) {
                q_result_page_step_b(self);
            }
            break;
        case 5:
            q_result_reward_page_draw(self);
            break;
        case 6:
            q_result_points_page_draw(self);
            break;
        case 7:
            q_result_size_page_draw(self);
            break;
        case 8:
            q_result_unlock_page_draw(self);
            break;
        }
    }
}

/* 0x8039CD18 (0x4): The screen's empty per-frame hook. */
void q_result_noop(QResultScreen* self)
{
}

/* 0x8039CD1C (0x98): How many of `item` the result's item box holds (the gunner box first, when it is in use). */
s16 q_result_box_count_get(u16 item)
{
    Q_UserData* user = get_userdata();
    Q_ResultWork* q = get_qResult_work();
    s16 count = 0;

    if (userdata_gunner_ck(user) == 1) {
        count = item_count_find(item, &q->items_0x154[0x18], 8);
    }
    if (count == 0) {
        count = item_count_find(item, &q->items_0x154[0], 0x18);
    }
    return count;
}

/* 0x8039CDB4 (0x120): Whether `count` of `item` fit into the result's item box: 0 all of them (`fit` = `count`),
 * 1 the stack is full, 2 only `fit` of them, 3 no slot is free. */
u8 q_result_box_fit_ck(u16 item, s16 count, s16* fit)
{
    Q_UserData* user = get_userdata();
    Q_ResultWork* q = get_qResult_work();
    ItemDataRecord* data;
    s16 held;
    u8 result;

    *fit = 0;
    data = GetItemData(item);
    held = q_result_box_count_get(item);
    if (held > 0) {
        if (held == data->max_num_0x003) {
            result = 1;
            *fit = 0;
        } else {
            *fit = data->max_num_0x003 - held;
            if (*fit < count) {
                result = 2;
            } else {
                result = 0;
                *fit = count;
            }
        }
    } else {
        result = 3;
        *fit = 0;
        if (data->kind_0x00 == 1 && userdata_gunner_ck(user) == 1 &&
            (u16)item_slots_free_count(&q->box_0x154[0x18], 8) != 0) {
            result = 0;
            *fit = count;
        } else if ((u16)item_slots_free_count(&q->box_0x154[0], 0x18) != 0) {
            result = 0;
            *fit = count;
        }
    }
    return result;
}

/* 0x8039CED4 (0xD4): Puts `count` of `item` into the result's item box: kind-1 items the main box does not hold
 * go to the gunner box first. */
void q_result_box_take(u16 item, s16 count)
{
    Q_UserData* user = get_userdata();
    Q_ResultWork* q = get_qResult_work();

    if (GetItemData(item)->kind_0x00 != 1 || userdata_gunner_ck(user) != 1 ||
        item_pair_index_find(item, &q->box_0x154[0], 0x18) != -1 ||
        (u16)item_take(item, count, &q->box_0x154[0x18], 8, 1, 0) > 4) {
        item_take(item, count, &q->box_0x154[0], 0x18, 1, 0);
    }
}

/* 0x8039CFA8 (0xC0): How many of `item` the player owns: the pouch the equipment set uses plus the item box. */
s16 q_result_owned_count_get(u16 item)
{
    s16 count;
    s32 size;

    if (userdata_gunner_ck(get_userdata()) == 1) {
        size = userdata_pouch_size(0);
        count = item_count_find(item, userdata_pouch_get(lobby_world_block, 0), size);
    } else {
        size = userdata_pouch_size(1);
        count = item_count_find(item, userdata_pouch_get(lobby_world_block, 1), size);
    }
    return count + (s16)item_slots_count_sum(item, ((Q_UserData*)lobby_world_block)->box_0x180,
                                             userdata_box_capacity(lobby_world_block));
}

/* 0x8039D068 (0x60): Stores (`item`, `count`) into slot `index` (0..32) of the result's item box. */
void q_result_box_slot_set(u16 item, s16 count, u16 index)
{
    Q_ResultWork* q = get_qResult_work();

    if (index <= 32) {
        q->box_0x154[index].id = item;
        q->box_0x154[index].value = count;
    }
}

/* 0x8039D0C8 (0x48): Resets one player's box record: a 4x4 grid, two boxes, cursor home. */
void q_result_box_record_init(_multi_result_work* rec, u8 player_no)
{
    rec->state = 0;
    rec->player_no = player_no;
    rec->field_0x02 = 0;
    rec->field_0x03 = 2;
    rec->cursor_x = 0;
    rec->box_w = 4;
    rec->cursor_y = 0;
    rec->box_h = 4;
    rec->field_0x0E = 0;
    rec->field_0x08 = 0xFF;
    rec->saved_cursor_x = 0;
    rec->saved_cursor_y = 0;
    rec->field_0x0C = 0;
}

/* 0x8039D110 (0x168): Enters `phase` of the multiplayer box band: phase 1 resets every player's box record and
 * files the quest time into the VS records it beats, 8 arms the swap timer, 9 parks the box cursor. */
void q_result_phase_enter(QResultScreen* self, s32 phase)
{
    u32 updated = 0;
    Q_ResultWork* q = get_qResult_work();
    MultiResultRecordArray* recs = (MultiResultRecordArray*)self;
    s32 i;
    s32 slot;
    u32* times;

    self->phase = phase;
    self->select_mode = 0;
    self->phase_timer = 120.0f * Screen_w.frame_scale;
    self->phase_frame = 0;
    switch (phase) {
    case 1:
        slot = (s16)(q->progress_0x1E0 - 9000);
        for (i = 0; i < self->field_0x0007; i++) {
            _vs_user_data* user;

            q_result_box_record_init(&recs->records[i], i);
            user = get_vsUser_work(i);
            if (self->field_0x0007 == 1) {
                times = user->slot_a_0x6C;
            } else {
                times = user->slot_b_0x94;
            }
            if (times[slot] == 0 || q->elapsed_0x148 < times[slot]) {
                times[slot] = q->elapsed_0x148;
                updated = 1;
            }
        }
        if (updated == 1) {
            for (i = 0; i < self->field_0x0007; i++) {
                recs->records[i].field_0x08 = 0;
            }
        }
        break;
    case 8:
        q_result_swap_start(self);
        break;
    case 9:
        self->box_msg = -1;
        self->box_player = 0;
        recs->records[0].field_0x03 = 4;
        recs->records[0].field_0x02 = 0;
        break;
    }
}

} /* extern "C" */

/* The box band's phase latch: enter the phase the screen names, then step the sub-state on.  Cases 0
 * and 1 enter the same phase - the target keeps the two bodies separately, so the source does too. */
void multi_box_phase_apply(QResultScreen* self) {
    switch (self->phase) {
    case 0:
        q_result_phase_enter(self, 1);
        self->sub_state++;
        break;
    case 1:
        q_result_phase_enter(self, 1);
        self->sub_state++;
        break;
    case 5:
        system_copy_filter_clear();
        q_result_phase_enter(self, 5);
        self->sub_state++;
        break;
    case 8:
        q_result_phase_enter(self, 8);
        self->sub_state++;
        break;
    case 9:
        q_result_phase_enter(self, 9);
        self->sub_state++;
        break;
    }
}

/* Returns the box grid slot the cursor is on: `x + y * width` of the record's own cursor. */
u16 multi_box_cursor_index(_multi_result_work* box) {
    return (u16)(box->cursor_x + box->cursor_y * box->box_w);
}

/* The grid slot the cursor is on: the player's own box when the record's player number is 0, the
 * shared one otherwise. */
MultiResultBoxItem* multi_box_cursor_item_get(_multi_result_work* box) {
    MultiResultBoxGrids* work = box->work;
    MultiResultBoxItem* items = box->player_no == 0 ? work->my_box : work->other_box;

    return &items[multi_box_cursor_index(box)];
}

/* Whether the player's box still has anything in it: 1 as soon as one of the 16 slots carries an
 * item with a positive count. */
u32 multi_box_rem_exist_ck(_multi_result_work* box) {
    MultiResultBoxGrids* work = box->work;
    MultiResultBoxItem* items = box->player_no == 0 ? work->my_box : work->other_box;
    u32 i;

    if (items != NULL) {
        /* Two groups of eight slots, the eight checks written out: retail keeps one pointer walking
         * 0x20 B per group with fixed offsets 0..0x1C (an inner `for` over eight slots makes MWCC index
         * the second group through a second pointer and costs the function 4 B).  RESIDUAL: the
         * target also keeps a *dead* loop counter - `li r3, 0` plus `addi r3, r3, 7` per group, which
         * also puts the walking pointer in r4 - and MWCC eliminates it from every spelling tried
         * (for-increment, body increment, hoisted init), so this function is 2 instructions short
         * (316 B vs 324 B, 96.27 %). */
        for (i = 0; i < 14; i += 7) {
            if (items[0].item_id != 0 && items[0].count > 0) return 1;
            if (items[1].item_id != 0 && items[1].count > 0) return 1;
            if (items[2].item_id != 0 && items[2].count > 0) return 1;
            if (items[3].item_id != 0 && items[3].count > 0) return 1;
            if (items[4].item_id != 0 && items[4].count > 0) return 1;
            if (items[5].item_id != 0 && items[5].count > 0) return 1;
            if (items[6].item_id != 0 && items[6].count > 0) return 1;
            if (items[7].item_id != 0 && items[7].count > 0) return 1;
            items += 8;
        }
    }
    return 0;
}

/* Credits every item the player's box still holds into the VS user block's point counter, then
 * empties the 16 slots.  The `items != NULL` guard is retail's own (the grids hang off `work`). */
void multi_box_grid_clear(_multi_result_work* box, _vs_user_data* user) {
    MultiResultBoxGrids* work = box->work;
    MultiResultBoxItem* items = box->player_no == 0 ? work->my_box : work->other_box;

    if (items != NULL) {
        u32 i;

        for (i = 0; i < 16; i++) {
            if (items[i].item_id != 0 && items[i].count > 0) {
                score_add_clamped(GetItemData(items[i].item_id)->field_0x010 * items[i].count,
                                  &user->point_0x18);
                items[i].item_id = 0;
                items[i].count = 0;
            }
        }
    }
}

/* 0x8039D5B0 (0x8C): steps the box record's grid cursor with the pad's pressed and auto-repeat words (columns on
 * left/right, rows on up/down), inside the grid's width and height. */
extern "C" void multi_box_cursor_clamp(_multi_result_work* box, PadButtons* pad) {
    box->cursor_x = menu_cursor_step(box->cursor_x, box->box_w, pad->pressed_0x04 | pad->held_0x14, 4, 8);
    box->cursor_y = menu_cursor_step(box->cursor_y, box->box_h, pad->pressed_0x04 | pad->held_0x14, 1, 2);
}

/* 0x8039D63C (0x308): one frame of a player's box screen: the timers, then either the box choice (confirm opens
 * the box or banks what is left, cancel jumps to the last entry) or the grid (confirm takes the item under the
 * cursor into the player's VS item slots, up to 99, cancel goes back). */
extern "C" void multi_box_grid_step(_multi_result_work* box) {
    PadButtons* pad = &Psw[box->player_no].button_0x2C0;
    _vs_user_data* user = get_vsUser_work(box->player_no);
    MultiResultBoxItem* item;
    s32 held;
    s32 room;

    if (box->field_0x08 <= 2) {
        box->field_0x08++;
    }
    if (++box->field_0x0E > 40) {
        box->field_0x0E = 0;
    }
    if (box->field_0x0C > 0) {
        if (++box->field_0x0C >= 12) {
            box->field_0x0C = 0;
        }
    }
    switch (box->state) {
    case 0:
        if ((pad->pressed_0x04 & 0x10) != 0) {
            sysSE_req(0);
            switch (box->field_0x02) {
            case 0:
                if (multi_box_rem_exist_ck(box) == 1) {
                    box->state = 1;
                    sysSE_req(0);
                }
                break;
            case 1:
                if (multi_box_rem_exist_ck(box) == 1) {
                    multi_box_grid_clear(box, user);
                    box->state = 0xFF;
                    sysSE_req(9);
                } else {
                    box->state = 0xFF;
                    sysSE_req(0);
                }
                break;
            }
        } else if ((pad->pressed_0x04 & 0x20) != 0) {
            box->field_0x02 = box->field_0x03 - 1;
            sysSE_req(1);
        } else if (multi_box_rem_exist_ck(box) == 1) {
            box->field_0x02 = menu_cursor_step(box->field_0x02, box->field_0x03, pad->pressed_0x04 | pad->held_0x14, 1, 2);
        }
        break;
    case 1:
        if ((pad->pressed_0x04 & 0x10) != 0) {
            item = multi_box_cursor_item_get(box);
            if (item->item_id != 0) {
                held = item_count_find(item->item_id, (IdValue*)user->item_0x2C, 16);
                if (held < 99) {
                    if (item_take(item->item_id, item->count, (IdValue*)user->item_0x2C, 16, 0, 99) == 3) {
                        room = 99 - held;
                        if (item->count > room) {
                            item->count -= (s16)room;
                        } else {
                            item->item_id = 0;
                            item->count = 0;
                        }
                    } else {
                        item->item_id = 0;
                        item->count = 0;
                    }
                    box->saved_cursor_x = box->cursor_x;
                    box->saved_cursor_y = box->cursor_y;
                    box->field_0x0C = 1;
                    sysSE_req(8);
                } else {
                    sysSE_req(2);
                }
            } else {
                sysSE_req(2);
            }
            if (multi_box_rem_exist_ck(box) == 0) {
                box->state = 0;
                box->field_0x02 = box->field_0x03 - 1;
            }
        } else if ((pad->pressed_0x04 & 0x20) != 0) {
            box->state = 0;
            sysSE_req(1);
        } else {
            multi_box_cursor_clamp(box, pad);
        }
        break;
    }
}

/* Whether the screen may enter the phase `mode` names: 1 is refused while the quest result is still
 * showing, and 8 waits until every player has seven of the eight slots settled. */
u32 multi_box_phase_ck(QResultScreen* self, u8 mode) {
    u16 mask = 0;
    u8 n = 0;
    s32 i;

    switch (mode) {
    case 0:
        return 0;
    case 1:
        if (q_result_phase_is_2(self) == 1 || q_result_phase_is_3(self) == 1) {
            return 0;
        }
        break;
    case 8: {
        s32 i;

        for (i = 0; i < self->field_0x0007; i++) {
            _vs_user_data* user = get_vsUser_work(i);

            mask |= user->ready_mask_0xBC;
            n = 0;
            if (user->slot_a_0x6C[0] != 0 || user->slot_b_0x94[0] != 0) n++;
            if (user->slot_a_0x6C[1] != 0 || user->slot_b_0x94[1] != 0) n++;
            if (user->slot_a_0x6C[2] != 0 || user->slot_b_0x94[2] != 0) n++;
            if (user->slot_a_0x6C[3] != 0 || user->slot_b_0x94[3] != 0) n++;
            if (user->slot_a_0x6C[4] != 0 || user->slot_b_0x94[4] != 0) n++;
            if (user->slot_a_0x6C[5] != 0 || user->slot_b_0x94[5] != 0) n++;
            if (user->slot_a_0x6C[6] != 0 || user->slot_b_0x94[6] != 0) n++;
            if (n >= 7) break;
        }
        if (n >= 7) {
            if ((mask & 0x380) != 0x380) {
                break;
            }
            return 0;
        }
        return 0;
    }
    }
    return 1;
}

/* Advances the screen's phase: each phase names the next one, and the phase only latches once the
 * box band says the screen is ready for it.  Returns 1 once a phase was latched. */
u32 multi_box_phase_step(QResultScreen* self, u8 mode) {
    u32 next;

    for (;;) {
        switch (mode) {
        case 0:
            if (q_result_phase_is_2(self) == 1 || q_result_phase_is_3(self) == 1) {
                next = 9;
            } else {
                next = 1;
            }
            break;
        case 1:
            next = 5;
            break;
        case 5:
            next = 8;
            break;
        case 8:
            next = 9;
            break;
        default:
            return 0;
        }
        if (multi_box_phase_ck(self, (u8)next) == 0) {
            mode = (u8)next;
            continue;
        }
        self->phase = next;
        return 1;
    }
}

/* The two `system_w` bytes the box save keeps while the player's options are swapped in. */
u8 multi_box_save_keep[2];

/* 0x8039DBC8 (0x780): Runs the box band's current phase: the grid choice (1), the reward (5), the ready flags (8) and
 * the save that writes each player's box back to the remote or the save file (9); returns 1 when the phase is done. */
s32 multi_box_phase_input(QResultScreen* self)
{
    PadButtons* pad = &Psw[0].button_0x2C0;
    _multi_result_work* rec = ((MultiResultRecordArray*)self)->records;
    _vs_user_data* vs;
    Q_UserData* user;
    s32 result;
    s32 i;
    s32 player;
    s32 slot;

    switch (self->phase) {
    case 1:
        ai_npc_reaction_forward();
        for (i = 0; i < self->field_0x0007; i++, rec++) {
            multi_box_grid_step(rec);
        }
        result = 0;
        rec = (_multi_result_work*)self->box_records;
        for (i = 0; i < self->field_0x0007; i++) {
            if (rec->state != 0xFF) {
                result = 1;
                break;
            }
            rec++;
        }
        if (result == 0) {
            return 1;
        }
        break;
    case 5:
        if (pad->pressed_0x04 & 0x10) {
            sysSE_req(0);
            for (player = 0; player < self->field_0x0007; player++) {
                score_add_clamped(self->zenny_total, &get_vsUser_work(player)->point_0x18);
            }
            return 1;
        }
        break;
    case 8:
        if (pad->pressed_0x04 & 0x10) {
            for (player = 0; player < self->field_0x0007; player++) {
                get_vsUser_work(player)->ready_mask_0xBC |= 0x380;
            }
            sysSE_req(0);
            return 1;
        }
        break;
    case 9:
        switch (self->select_mode) {
        case 0:
            system_w.hbm_disabled = 1;
            if (system_w.vs_player_done_0x89c[self->box_player] == 1) {
                vs_user_checksum_set(self->box_player);
                wpad_memory_access_init(self->box_player, self->box_player);
                self->select_mode = 1;
            } else {
                vs = get_vsUser_work(self->box_player);
                readDataFile_init(vs->player_0xCF, -1);
                system_w.vs_load_player_0x7e8 = vs->player_0xCF;
                self->select_mode = 4;
                self->phase_step = 0;
            }
            break;
        case 1:
            result = read_wpad_memory(&self->box_vs_user, 0);
            switch (result) {
            case 0:
                break;
            case 1:
                if (ck_mydata_vs(get_vsUser_work(self->box_player), &self->box_vs_user) == 0) {
                    sysSE_req(5);
                    self->select_mode = 110;
                    system_w.hbm_disabled = 0;
                } else {
                    wpad_memory_access_init(self->box_player, self->box_player);
                    self->select_mode = 2;
                }
                break;
            case -3:
            case -2:
                sysSE_req(5);
                self->box_msg = 7;
                self->select_mode = 100;
                self->unlock_wait = 0;
                system_w.hbm_disabled = 0;
                break;
            default:
                sysSE_req(5);
                self->unlock_wait = 0;
                self->select_mode = 110;
                system_w.hbm_disabled = 0;
                break;
            }
            break;
        case 2:
            result = write_wpad_memory(self->box_player, 1);
            switch (result) {
            case 0:
                self->box_msg = 6;
                break;
            case 1:
            case -100:
                self->box_msg = -1;
                self->box_player++;
                if (self->box_player < self->field_0x0007) {
                    self->select_mode = 0;
                } else {
                    sysSE_req(8);
                    self->select_mode++;
                    system_w.hbm_disabled = 0;
                }
                break;
            case 101:
                sysSE_req(5);
                self->box_msg = 9;
                break;
            case -6:
            case -5:
                sysSE_req(5);
                self->box_msg = 8;
                self->select_mode = 100;
                self->unlock_wait = 0;
                system_w.hbm_disabled = 0;
                break;
            default:
                sysSE_req(5);
                self->box_msg = 7;
                self->select_mode = 100;
                self->unlock_wait = 0;
                system_w.hbm_disabled = 0;
                break;
            }
            break;
        case 3:
            if (pad->pressed_0x04 & 0x10) {
                if (rec->field_0x02 == 4) {
                    self->select_mode = 0;
                    self->box_msg = -1;
                    self->box_player = 0;
                } else {
                    self->box_result = rec->field_0x02 + 1;
                    sysSE_req(0);
                    return 1;
                }
            } else {
                rec->field_0x02 = menu_cursor_step(rec->field_0x02, rec->field_0x03, pad->pressed_0x04 | pad->held_0x14, 1, 2);
            }
            break;
        case 4:
            switch (self->phase_step) {
            case 0:
                if (readDataFile() != 0) {
                    vs = get_vsUser_work(self->box_player);
                    setPlayerSave2Userdata(system_w.vs_save_slot_0x89e[self->box_player]);
                    user = get_userdata();
                    for (slot = 0; slot < 16; slot++) {
                        item_pair_copy(&user->vs_items_0x5364[slot], &vs->item_0x2C[slot]);
                    }
                    for (slot = 0; slot < 10; slot++) {
                        user->vs_slot_a_0x52E4[slot] = vs->slot_a_0x6C[slot];
                        user->vs_slot_b_0x530C[slot] = vs->slot_b_0x94[slot];
                    }
                    user->event_bits_0x52E2 = vs->ready_mask_0xBC;
                    user->vs_point_0x53A4 = vs->point_0x18;
                    multi_box_save_keep[0] = system_w.save_keep_0x910[0];
                    multi_box_save_keep[1] = system_w.save_keep_0x910[1];
                    memcpy(option_w, vs->option_0xD0, 31);
                    if (createDataFile_init(vs->player_0xCF, system_w.vs_save_slot_0x89e[self->box_player], 0) == 1) {
                        self->phase_step++;
                    } else {
                        self->select_mode = 3;
                        system_w.hbm_disabled = 0;
                    }
                }
                break;
            case 1:
                result = game_save_wait();
                switch (result) {
                case 1:
                    self->box_msg = -1;
                    self->box_player++;
                    if (self->box_player < self->field_0x0007) {
                        self->select_mode = 0;
                    } else {
                        sysSE_req(8);
                        self->select_mode = 3;
                        system_w.hbm_disabled = 0;
                    }
                    break;
                case -2:
                case -1:
                    sysSE_req(5);
                    self->box_msg = chg_nand_err2msgcode();
                    self->select_mode = 200;
                    self->unlock_wait = 0;
                    system_w.hbm_disabled = 0;
                    break;
                }
                if (result != 0) {
                    system_w.save_keep_0x910[0] = multi_box_save_keep[0];
                    system_w.save_keep_0x910[1] = multi_box_save_keep[1];
                }
                break;
            }
            break;
        case 100:
        case 110:
        case 200:
            if (self->unlock_wait < 30) {
                self->unlock_wait++;
            } else if (Psw[self->box_player].button_0x2C0.pressed_0x04 & 0x10) {
                sysSE_req(0);
                self->box_msg = -1;
                self->box_player++;
                if (self->box_player < self->field_0x0007) {
                    self->select_mode = 0;
                } else {
                    self->select_mode = 3;
                    system_w.hbm_disabled = 0;
                }
            }
            break;
        }
        break;
    }
    return 0;
}

/* 0x8039E348 (0x284): The multiplayer box screen's task: sets the screen up from the result record and the
 * delivered items, then runs the box phases until the closing fade ends and returns `box_result`. */
BOOL multi_box_result_step(Q_MoveWork* move)
{
    QResultScreen* self = (QResultScreen*)move->result_buffer_0x150;
    Q_ResultWork* q = get_qResult_work();
    Q_ItemWork* work = move_work_item_work_get();
    MultiResultRecordArray* recs;
    s32 i;

    q_result_noop(self);
    switch (self->state) {
    case 0:
        if (file_loading_ck(NULL, NULL) == 1) {
            break;
        }
        pad_connect_disp_set(0);
        recs = (MultiResultRecordArray*)self;
        recs->records[0].work = (MultiResultBoxGrids*)self;
        recs->records[1].work = (MultiResultBoxGrids*)self;
        if (system_w.field_0x8af == 0) {
            self->field_0x0007 = 1;
        } else {
            self->field_0x0007 = 2;
        }
        self->state++;
        self->sub_state = 0;
        self->zenny_rows[0] = 0;
        self->zenny_rows[1] = 0;
        self->zenny_rows[2] = 0;
        self->zenny_rows[3] = 0;
        self->point_rows[0] = 0;
        self->point_rows[1] = 0;
        self->point_rows[2] = 0;
        q->field_0x1E8 = 0;
        if (q_result_phase_is_3(self) == 0 && q_result_phase_is_2(self) == 0) {
            q->field_0x1E4 = work->reward_0x2E8;
            for (i = 0; i < 16; i++) {
                item_pair_copy(&self->grid_items[i], item_pair_copy(&self->grid_kept[i], &q->box_items_0x3F4[i]));
            }
        }
        self->zenny_total = q->field_0x1E4;
        self->swap_counter = 0;
        multi_box_phase_step(self, 0);
        break;
    case 1:
        if (self->swap_timer > 0) {
            self->swap_timer--;
        }
        switch (self->sub_state) {
        case 0:
            system_copy_filter_arm();
            multi_box_phase_apply(self);
            break;
        case 1:
            if (multi_box_phase_input(self) == 1) {
                self->sub_state++;
            }
            break;
        case 2:
            self->phase_timer++;
            if (self->phase_timer > 20) {
                self->phase_timer = 0;
                if (multi_box_phase_step(self, self->phase) == 0) {
                    self->sub_state++;
                    system_copy_filter_clear();
                    self->phase_timer = 16;
                    if (self->phase_timer < self->swap_timer) {
                        self->phase_timer = self->swap_timer;
                    }
                } else {
                    self->sub_state = 0;
                    q_result_phase_apply(self);
                }
            }
            break;
        case 3:
            if (self->phase_timer > 0) {
                self->phase_timer--;
                break;
            }
            return self->box_result;
        }
        subTransSet((u32)q_result_draw_task, 0, NULL);
        break;
    }
    return FALSE;
}
