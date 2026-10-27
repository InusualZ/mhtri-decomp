/*
 * lobby/lb_screen_step.cpp - the lobby smithy (lobby screen 15): its menus, the make, upgrade and decoration pages,
 *   their steps and their draws.
 * RANGE. .text 0x8035FC18-0x80365C84 (59 functions, all written, defined in address order); .data 0x805ED950-0x805EDAB4,
 *   .bss 0x806BF3C0-0x806BF440, .sdata 0x80793348-0x807933B0, .sbss 0x80794BE8-0x80794BF0, extab, extabindex.  The
 *   `.data`/`.sdata`/`.bss` contents are emitted here (the templates first, the jump tables, then each draw's static
 *   sprite tables) and are byte-identical to the target's.
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off` (retail keeps the narrowing `clrlwi`s of `u8`/`u16`
 *   arguments, e.g. `lb_smith_records_copy`'s mode).  `#pragma optimization_level 4` around `lb_smith_make_step`,
 *   `lb_screen15_step` and `lb_smith_make_draw` (playbook 108: their switches test case ranges first with one unsigned
 *   compare); file-wide the level costs `lb_smith_kakou_apply` one row, and `opt_common_subs off` helps no row.
 * NAMES. The file stem is a GUESS kept from the registration; no `__FILE__` string covers it.  `seisan_data`,
 *   `kakou_data` and `deco_sei_tbl_ptr` are the map's names (make, upgrade and decoration recipe tables), and
 *   `lb_screen15_step` was named by the hub pass.  GUESS (from each body and its callers): every `lb_smith_*` function,
 *   `lb_smith_menu_tmpls` and every `LbSmith*` type; the foreign callees this unit named are in the name registry.
 * RESIDUALS. Register colouring or instruction scheduling only, unless noted:
 *  - `lb_smith_make_page_build`, `lb_smith_deco_page_build`, `lb_smith_attach_page_build`, `lb_smith_detach_list_build`,
 *    `lb_smith_deco_goods_draw`: the loop's saved registers (the declaration orders are the best a hill climb found);
 *  - `lb_smith_deco_draw`, `lb_smith_attach_draw`, `lb_smith_detach_draw`, `lb_smith_kakou_draw`: retail computes the
 *    page-draw mode before the scroll address (`addi r3,0x154` after the branch); ours has it at initpcode, before the
 *    branch, because the frontend folds the mode statement back into the call argument (a separate `mode`, a scroll
 *    variable, an inline helper, a `u8`/`bool` view and per-function pragmas leave it there);
 *  - `lb_smith_kakou_preview_set`: the same argument order for `lb_scroll_materials_set`'s recipe address (8 bytes);
 *  - `lb_smith_make_preview_set`: the three gun-part records' stack slots (retail ascending, a 4-byte gap after the
 *    first);
 *  - `lb_smith_kakou_open`, `lb_smith_sort_labels_draw`, `lb_smith_deco_info_draw`: retail narrows the argument in its
 *    own register (`clrlwi r4,r4`/`extsb r4,r4`), ours through r0;
 *  - `lb_smith_pl_turn_step`: retail returns from the busy test through `bne`/`b` (4 bytes; nested, else-return and
 *    inline-helper shapes all give `beq`);
 *  - `lb_smith_make_step`, `lb_smith_kakou_step`: the previous tab byte sits in r26 (the set pane's slot register) in
 *    retail, in r29 here.
 *   Data: `.sbss` is 4 bytes short of the claimed 8 (only `deco_sei_tbl_ptr` is referenced; 0x80794BEC is not), and
 *   `.sdata` ends 4 zero bytes short of the target's 104 (alignment fill).
 *   flipcheck: 39 foreign callees carry registry names the map does not have yet.
 * SHAPES. A callee whose retail argument is wider or narrower than its owner's declaration is called through a `_c1`
 *   view (`lb_cmd_pressed_ck(...) == 1` is an unsigned compare in retail); a `bool` parameter passes a flag on to
 *   `GetMenu*Color` unnormalised.  A page-build loop keeps its `last` bound and an `s32` count; refusal bits start at 0
 *   and are ORed in.  The goods-window loops index the rows and the layout table by the loop counter.  A centred text
 *   sums `base + spr.pos + (size >> 1)` and subtracts the line offset in a second statement.  Retail's `addi 0xFE`
 *   range tests are `(u8)(x + 0xFE) <= 1`.  The upgrade recipe is `kakou_data[kind]` then `+= item`.
 */
#include "types.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#include "Pl/plw.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_scroll_work.h"
#include "fn_80047398/lobby_world_block.h"
#include "fn_80047398/userdata_gunner_ck.h"
#include "hud/layout.h"
#include "hud/cockpit.h"
#include "hud/ainpc_page_hold_clear.h"
#include "hud/draw_lsp_sprite_variant.h"
#include "lobby/lb_list_init.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_scroll_init.h"
#include "lobby/lb_scroll_kind_set.h"
#include "lobby/lb_list_pane_init.h"
#include "lobby/lb_equip_kind_ck.h"
#include "lobby/lb_equip_entry_apply.h"
#include "lobby/lb_equip_flags_ck.h"
#include "lobby/lb_equip_kind_se_set.h"
#include "lobby/lb_page_record_empty_ck.h"
#include "lobby/lb_kakou_icon_build.h"
#include "lobby/lb_equip_page_draw.h"
#include "lobby/lb_title_frame_draw.h"
#include "lobby/lb_pane_draw.h"
#include "lobby/lb_equip_cell_draw.h"
#include "lobby/lb_equip_cell_draw_layout5.h"
#include "lobby/lb_anim_mark_draw.h"
#include "lobby/lb_font_print.h"
#include "lobby/LbStr.h"
#include "lobby/lb_scroll_tables_set.h"
#include "lobby/lb_equip_set_pane_step.h"
#include "lobby/lb_equip_records_copy.h"
#include "lobby/lb_eqshop_sort_init.h"
#include "lobby/lb_npc_scene_start.h"
#include "lobby/lb_menu_page_step.h"
#include "lobby/lb_unlock_cond_last_ck.h"
#include "lobby/lb_deli_list_build.h"
#include "menu/menu_message.h"
#include "menu/menu_item.h"
#include "menu/menu_row.h"
#include "menu/ItemExp.h"
#include "menu/get_pop_dat_ptr.h"
#include "menu/menu_pad_map_set.h"
#include "ef/eft052.h"
#include "ef/eft052_page_put.h"
#include "ef/fn_800CDB2C.h"
#include "ef/system_lobby_flag_get.h"
#include "ef/system_lobby_flag_set.h"
#include "enemy/em020_ai.h"
#include "sound/fn_800D7F54.h"
#include "mh3_pad/Psw.h"
#include "userdata_item.h"
#include "fn_80047398.h"
#include "userdata_equip_mask_get.h"
#include "userdata_equip_flag_ck.h"
#include "userdata_equip_box_worn_ck.h"
#include "fn_8004CAD8.h"
#include "color_rgba_copy.h"
#include "Pl/fn_8027D684.h"
#include "Pl/equip_color_set.h"
#include "Pl/pl_net_slot_bind.h"
#include "Pl/equip_deco_count.h"
#include "Pl/pl_status_sheet_get.h"
#include "Pl/item_kind_name_get.h"

#include "font/flfnt.h"
#include "MSL_C/printf.h"

/* One list template `lb_choice_init` copies (the 0x14-byte layout the retail tables use; `lobby/lb_list.h`'s
 * `LbListTmpl` carries 4 bytes more): the row count, string group, first string, help line, help table, decide sound
 * and flags.  size: 0x14 */
typedef struct LbSmithListTmpl {
    /* +0x00 */ s16 count_0x00;
    /* +0x02 */ u8 kind_0x02;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ s16 str_base_0x04;
    /* +0x06 */ s16 help_str_0x06;
    /* +0x08 */ s16* help_tbl_0x08;
    /* +0x0C */ s32 se_ok_0x0C;
    /* +0x10 */ u32 flags_0x10;
} LbSmithListTmpl;

/* One make recipe of `seisan_data`/`deco_sei_tbl_ptr` (0x18 B rows; the gun-part tables use 0x24 B rows, read through
 * the same head): the result item and the four materials.  size: 0x18 */
typedef struct LbSmithRecipe {
    /* +0x00 */ u8 unused_0x00[2];
    /* +0x02 */ u16 item_0x02;
    /* +0x04 */ LbItemCount materials_0x04[4];
    /* +0x14 */ u8 unused_0x14[4];
} LbSmithRecipe;

/* One upgrade recipe row of `kakou_data`: the three materials.  size: 0x18 */
typedef struct LbSmithKakouRecipe {
    /* +0x00 */ LbItemCount materials_0x00[3];
    /* +0x0C */ u8 unused_0x0C[0xC];
} LbSmithKakouRecipe;

/* One goods row of the make page: the kind, its state bits (0x40 empty, 2/4/0x10 refused), the item, the price and
 * the recipe it came from (a weapon/armour row, a gun-part row, or the decoration's species record).  size: 0x18 */
typedef struct LbSmithGoods {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 flags_0x02;
    /* +0x04 */ u16 item_0x04;
    /* +0x06 */ u8 pad_0x06[2];
    /* +0x08 */ s32 price_0x08;
    /* +0x0C */ LbSmithRecipe* recipe_0x0C;
    /* +0x10 */ LbSmithRecipe* gun_recipe_0x10;
    /* +0x14 */ ItemSpeciesRecord* species_0x14;
} LbSmithGoods;

/* One decoration row of the attach/detach pages: the species record, the price, the decoration item, its species id,
 * its slot and its state bits.  size: 0x10 */
typedef struct LbSmithDeco {
    /* +0x00 */ ItemSpeciesRecord* species_0x00;
    /* +0x04 */ s32 price_0x04;
    /* +0x08 */ u16 item_0x08;
    /* +0x0A */ u16 species_id_0x0A;
    /* +0x0C */ u16 slot_0x0C;
    /* +0x0E */ u16 flags_0x0E;
} LbSmithDeco;

/* This unit's view of the sort chooser `lb_eqshop_sort_init` fills (`LbShopSortWork` in `lobby/fn_801E0ADC.cpp`): three
 * lists, the row-to-value tables of the second and third, the two picked values, the step and the kind.  size: 0x84 */
typedef struct LbSmithSort {
    /* +0x00 */ LbList lists_0x00[3];
    /* +0x60 */ u8 values_b_0x60[0x10];
    /* +0x70 */ u8 values_a_0x70[0x10];
    /* +0x80 */ u8 pick_b_0x80;
    /* +0x81 */ u8 pick_a_0x81;
    /* +0x82 */ u8 step_0x82;
    /* +0x83 */ u8 kind_0x83;
} LbSmithSort;

/* This unit's view of the 0x234-byte equipment box pane (`LbEntryPane` in `lobby/lb_pane_ui.cpp`): the redraw flag and
 * the box entry the digit input picked.  size: 0x234 */
typedef struct LbSmithPane {
    /* +0x000 */ u8 unused_0x000[8];
    /* +0x008 */ u16 redraw_0x008;
    /* +0x00A */ u8 unused_0x00A[0x14];
    /* +0x01E */ s16 box_0x01E;
    /* +0x020 */ u8 unused_0x020[0x214];
} LbSmithPane;

/* This unit's view of the equipment-set pane `lb_equip_set_pane_init` fills: the three box entries it equips.
 * size: 0x94 */
typedef struct LbSmithSetPane {
    /* +0x00 */ u8 unused_0x00[4];
    /* +0x04 */ u16 box_0x04[3];
    /* +0x0A */ u8 unused_0x0A[0x8A];
} LbSmithSetPane;

/* The smithy's menu work (`lobby_w.menu_0xAC`, cleared over 0x2000 bytes when the smithy opens).  size: 0x90E (the
 * block is 0x2000; nothing past +0x90E is read) */
typedef struct LbSmithWork {
    /* +0x000 */ u8 state_0x000;          /* the screen state `lb_screen15_step` dispatches on */
    /* +0x001 */ u8 step_0x001;           /* the page step */
    /* +0x002 */ u8 unused_0x002[2];
    /* +0x004 */ u8 hold_0x004;
    /* +0x005 */ u8 held_0x005;           /* the tab button is held */
    /* +0x006 */ u8 unused_0x006;
    /* +0x007 */ u8 kind_0x007;           /* the equipment kind on show */
    /* +0x008 */ u8 model_set_0x008;      /* the player's model shows a preview */
    /* +0x009 */ u8 unused_0x009;
    /* +0x00A */ u16 page_moved_0x00A;
    /* +0x00C */ s16 cursor_0x00C;
    /* +0x00E */ u8 unused_0x00E[4];
    /* +0x012 */ s16 page_0x012;
    /* +0x014 */ s16 pages_0x014;
    /* +0x016 */ u16 pick_0x016;          /* the goods row being bought */
    /* +0x018 */ s16 count_0x018;         /* the rows on the page */
    /* +0x01A */ u8 unused_0x01A[2];
    /* +0x01C */ LbSmithSort sort_0x01C;
    /* +0x0A0 */ LbList top_0x0A0;        /* weapons, armour, decorations, delivery */
    /* +0x0C0 */ LbList mode_0x0C0;       /* make or upgrade (decorations: make, attach, detach) */
    /* +0x0E0 */ LbList kind_list_0x0E0;
    /* +0x100 */ LbList part_list_0x100;  /* the gun part */
    /* +0x120 */ u8 unused_0x120[8];
    /* +0x128 */ s16 kakou_cursor_0x128;
    /* +0x12A */ s16 kakou_count_0x12A;
    /* +0x12C */ u8 tab_kinds_0x12C[8];
    /* +0x134 */ LbList tab_list_0x134;
    /* +0x154 */ LbScrollWork scroll_0x154;
    /* +0x1D4 */ _EQUIP row_0x1D4;
    /* +0x1E0 */ u8 unused_0x1E0[0x18];
    /* +0x1F8 */ u16 box_0x1F8[3];        /* the box entries the bought pieces went to */
    /* +0x1FE */ s16 stored_0x1FE;
    /* +0x200 */ s32 confirm_0x200;
    /* +0x204 */ _PLW* plw_0x204;
    /* +0x208 */ u8* npc_0x208;
    /* +0x20C */ _EQUIP kakou_rows_0x20C[4];
    /* +0x23C */ s32 kakou_price_0x23C[4];
    /* +0x24C */ u16 kakou_flags_0x24C[4];
    /* +0x254 */ LbItemCount materials_0x254[4];
    /* +0x264 */ LbSmithGoods goods_0x264[8];
    /* +0x324 */ LbSmithDeco decos_0x324[8];
    /* +0x3A4 */ LbSmithPane pane_0x3A4;
    /* +0x5D8 */ LbSmithPane set_box_0x5D8;
    /* +0x80C */ LbSmithSetPane set_0x80C;
    /* +0x8A0 */ _EQUIP records_0x8A0[6];
    /* +0x8E8 */ _EQUIP record_0x8E8;
    /* +0x8F4 */ _EQUIP records_0x8F4[2];
    /* +0x90C */ s16 wait_0x90C;
} LbSmithWork;

/* This unit's view of the user data `lobby_world_block` points at: the money and the equipment box.  size: 0x39A0
 * (approximate: the block continues past the box) */
typedef struct LbSmithUser {
    /* +0x0000 */ u8 unused_0x0000[0x18];
    /* +0x0018 */ s32 money_0x0018;
    /* +0x001C */ u8 unused_0x001C[0xDE4];
    /* +0x0E00 */ _EQUIP box_0x0E00[0x3A2];
} LbSmithUser;

/* The smithy's list templates: the top menu, the make/upgrade menus of weapons, armour and decorations, the weapon
 * and armour kinds, and the top menu with its fourth row blinking. */
LbSmithListTmpl lb_smith_menu_tmpls[7] = {
    {3, 0, 0, 0x9E, 0x3A, NULL, -1, 3},
    {2, 0, 0, 0xA2, 0x3E, NULL, 5, 3},
    {2, 0, 0, 0xA4, 0x40, NULL, 5, 3},
    {3, 0, 0, 0xA6, 0x42, NULL, 5, 3},
    {7, 0, 0, 0xB8, -1, NULL, 5, 3},
    {5, 0, 0, 0xB3, -1, NULL, 5, 3},
    {4, 0, 0, 0x9E, 0x3A, NULL, -1, 3},
};

/* The make recipe tables by kind, the upgrade tables by kind and the decoration recipe table. */
extern "C" {
LbSmithRecipe* seisan_data[16];
LbSmithKakouRecipe* kakou_data[16];
LbSmithRecipe* deco_sei_tbl_ptr;
}

#define SMITH_USER ((LbSmithUser*)lobby_world_block)

#pragma peephole off

/* Call views (the `_c1` convention): retail passes the user data to the profile send, whose owner declares no
 * parameter. */
#define lobby_net_profile_part1_send_c1 ((s32 (*)(u8*))lobby_net_profile_part1_send)
#define chk_pointer_c1 ((u32 (*)(void))chk_pointer)
#define toggle_word_step_c1 ((s32 (*)(s32*, u16, s32, s32, u16))toggle_word_step)
#define lb_kakou_icon_build_c1 ((u16 (*)(_EQUIP*, s32, _EQUIP*, s32*))lb_kakou_icon_build)
#define lb_equip_cell_draw_c1 ((void (*)(u16, _EQUIP*, u8))lb_equip_cell_draw)
#define lb_equip_page_draw_c1 ((void (*)(struct LbScrollWork*, s32))lb_equip_page_draw)
#define menu_text_block_center_x_c1 ((s16 (*)(char*, s16, s16, s16*))menu_text_block_center_x)
#define lb_cmd_pressed_ck_c1 ((u32 (*)(u16))lb_cmd_pressed_ck)

extern "C" {

void lb_smith_records_copy(_PLW* plw, LbSmithWork* self, u8 mode);
void lb_smith_pl_refresh(_PLW* plw, LbSmithWork* self);
void lb_smith_rows_clear(LbSmithWork* self, s16 page);
void lb_smith_make_page_build(LbSmithWork* self, s16 page);
void lb_smith_tabs_init(LbSmithWork* self, u8 mode);
void lb_smith_make_preview_set(LbSmithWork* self);
void lb_smith_pl_turn_reset(_PLW* plw);
void lb_smith_pl_turn_step(_PLW* plw);
void lb_smith_pl_view_step(LbSmithWork* self);
void lb_smith_pl_model_reset(LbSmithWork* self);
void lb_smith_box_pane_open(LbSmithWork* self);
void lb_smith_preview_wait_set(LbSmithWork* self);
void lb_smith_deco_preview_set(LbSmithWork* self);
void lb_smith_deco_page_build(LbSmithWork* self, s16 page);
void lb_smith_attach_preview_set(LbSmithWork* self);
void lb_smith_attach_page_build(LbSmithWork* self, s16 page);
s32 lb_smith_detach_price_get(u16 item);
void lb_smith_kakou_preview_set(LbSmithWork* self);
u32 lb_smith_gun_part_ck(u8 kind);

/* 0x8035FC18 (0x18): copies the player's equipment records into the work for mode `mode`. */
void lb_smith_records_copy(_PLW* plw, LbSmithWork* self, u8 mode)
{
    lb_equip_records_copy(plw, mode, self->records_0x8A0, &self->record_0x8E8, self->records_0x8F4);
}

/* 0x8035FC30 (0x50): restores the player's records and refreshes its weapon type. */
void lb_smith_pl_refresh(_PLW* plw, LbSmithWork* self)
{
    lb_equip_records_copy(plw, 1, self->records_0x8A0, &self->record_0x8E8, self->records_0x8F4);
    plw->field_0x002 = Get_pl_type(&plw->equipB, &plw->equipC);
}

/* 0x8035FC80 (0x88): opens the smithy for the player `plw` and the NPC record `npc`. */
void lb_smith_open(_PLW* plw, u8* npc)
{
    LbSmithWork* self = (LbSmithWork*)lobby_w.menu_0xAC;

    memset(self, 0, 0x2000);
    self->plw_0x204 = plw;
    self->npc_0x208 = npc;
    lobby_w.state_0x000 = 15;
    lobby_w.active_0x008 = 1;
    lobby_w.param_0x12D = 0;
    lb_talk_page_open(0);
    lb_smith_records_copy(plw, self, 0);
}

/* 0x8035FD08 (0x6C): closes the smithy, sending the player's profile when an equipped piece changed. */
void lb_smith_close(void)
{
    LbSmithWork* self = (LbSmithWork*)lobby_w.menu_0xAC;

    system_lobby_flag_set(0);
    userdata_pl_look_apply(self->plw_0x204, lobby_world_block);
    if (lobby_w.param_0x12D != 0) {
        lobby_w.param_0x12D = 0;
        lobby_net_profile_part1_send_c1(lobby_world_block);
    }
    lb_panel_close();
}

/* 0x8035FD74 (0x90): clears the goods and decoration rows for page `page`. */
void lb_smith_rows_clear(LbSmithWork* self, s16 page)
{
    self->count_0x018 = 0;
    self->page_0x012 = page;
    memset(self->goods_0x264, 0, sizeof(self->goods_0x264));
    self->goods_0x264[0].flags_0x02 = 0x40;
    self->goods_0x264[1].flags_0x02 = 0x40;
    self->goods_0x264[2].flags_0x02 = 0x40;
    self->goods_0x264[3].flags_0x02 = 0x40;
    self->goods_0x264[4].flags_0x02 = 0x40;
    self->goods_0x264[5].flags_0x02 = 0x40;
    self->goods_0x264[6].flags_0x02 = 0x40;
    self->goods_0x264[7].flags_0x02 = 0x40;
    memset(self->decos_0x324, 0, sizeof(self->decos_0x324));
    self->decos_0x324[0].flags_0x0E = 0x40;
    self->decos_0x324[1].flags_0x0E = 0x40;
    self->decos_0x324[2].flags_0x0E = 0x40;
    self->decos_0x324[3].flags_0x0E = 0x40;
}

/* 0x8035FE04 (0x24C): builds page `page` of the make goods of the current kind. */
void lb_smith_make_page_build(LbSmithWork* self, s16 page)
{
    LbSmithRecipe* table;
    s32 n;
    LbSmithRecipe* gun_table;
    s32 idx;
    s32 gun;
    s32 last;
    s32 plain;
    u16 item;
    u8 flags;
    s32 first;
    LbSmithGoods* goods;

    userdata_equip_box_free_count();
    first = page * 8;
    last = first + 7;
    lb_smith_rows_clear(self, page);
    table = NULL;
    gun_table = NULL;
    if ((u32)(self->kind_0x007 - 11) <= 2) {
        gun_table = seisan_data[self->kind_0x007];
        gun = 1;
    } else {
        table = seisan_data[self->kind_0x007];
        gun = 0;
    }
    n = 0;
    self->count_0x018 = 0;
    idx = -1;
    plain = gun == 0;
    for (;;) {
        if (!plain) {
            idx = lb_seisan_gun_row_next(self->kind_0x007, self->sort_0x01C.pick_b_0x80, idx, gun_table);
        } else {
            idx = lb_seisan_row_next(self->kind_0x007, self->sort_0x01C.pick_b_0x80, idx, table);
        }
        if (idx == -1) {
            break;
        }
        if (gun == 0) {
            item = table[idx].item_0x02;
        } else {
            item = ((LbSmithRecipe*)((u8*)gun_table + idx * 0x24))->item_0x02;
        }
        if (lb_equip_kind_ck(self->sort_0x01C.pick_a_0x81, self->kind_0x007, item) != 1) {
            continue;
        }
        if (first <= n && n <= last) {
            flags = lb_seisan_row_flags_get(self->kind_0x007, idx, 0);
            goods = &self->goods_0x264[self->count_0x018];
            goods->kind_0x00 = self->kind_0x007;
            goods->item_0x04 = item;
            goods->price_0x08 = lb_seisan_price_get(self->kind_0x007, item);
            if (userdata_equip_flag_ck(self->kind_0x007, item, 1) == 0) {
                userdata_equip_flag_set(self->kind_0x007, item, 1);
                userdata_equip_flag_set(self->kind_0x007, item, 2);
            }
            if (gun == 0) {
                goods->recipe_0x0C = &table[idx];
                goods->gun_recipe_0x10 = NULL;
            } else {
                goods->gun_recipe_0x10 = (LbSmithRecipe*)((u8*)gun_table + idx * 0x24);
                goods->recipe_0x0C = NULL;
            }
            goods->flags_0x02 = flags;
            self->count_0x018++;
        }
        n++;
    }
    if (self->cursor_0x00C >= self->count_0x018) {
        self->cursor_0x00C = self->count_0x018 - 1;
        if (self->cursor_0x00C < 0) {
            self->cursor_0x00C = 0;
        }
    }
    self->pages_0x014 = menu_page_count(n, 8);
    if (self->pages_0x014 == 0) {
        self->pages_0x014++;
    }
    lb_smith_make_preview_set(self);
}

/* 0x80360050 (0x254): sets the stats tabs up for mode `mode` (1 make, 2 upgrade, 4 decorations, 5 decoration make). */
void lb_smith_tabs_init(LbSmithWork* self, u8 mode)
{
    s16 count;

    memset(self->tab_kinds_0x12C, 0, 8);
    switch (mode) {
    case 5:
        self->tab_kinds_0x12C[0] = 8;
        count = 1;
        break;
    case 4:
        self->tab_kinds_0x12C[0] = 8;
        self->tab_kinds_0x12C[1] = 2;
        self->tab_kinds_0x12C[2] = 4;
        count = 3;
        break;
    case 1:
        if (lb_equip_kind_group_get(self->kind_0x007) != 2 || userdata_gunner_ck(lobby_world_block) == 0) {
            self->tab_kinds_0x12C[0] = 1;
            self->tab_kinds_0x12C[1] = 2;
            self->tab_kinds_0x12C[2] = 3;
            self->tab_kinds_0x12C[3] = 4;
            count = 4;
        } else {
            self->tab_kinds_0x12C[0] = 1;
            self->tab_kinds_0x12C[1] = 2;
            self->tab_kinds_0x12C[2] = 5;
            self->tab_kinds_0x12C[3] = 6;
            self->tab_kinds_0x12C[4] = 3;
            self->tab_kinds_0x12C[5] = 4;
            count = 6;
        }
        break;
    case 2:
        switch (lb_equip_kind_group_get(self->kind_0x007)) {
        case 1:
            self->tab_kinds_0x12C[0] = 1;
            self->tab_kinds_0x12C[1] = 2;
            self->tab_kinds_0x12C[2] = 3;
            self->tab_kinds_0x12C[3] = 4;
            count = 4;
            break;
        case 2:
            if (userdata_gunner_ck(lobby_world_block) == 1) {
                self->tab_kinds_0x12C[0] = 1;
                self->tab_kinds_0x12C[1] = 2;
                self->tab_kinds_0x12C[2] = 5;
                self->tab_kinds_0x12C[3] = 6;
                self->tab_kinds_0x12C[4] = 3;
                self->tab_kinds_0x12C[5] = 4;
                count = 6;
            } else {
                self->tab_kinds_0x12C[0] = 1;
                self->tab_kinds_0x12C[1] = 2;
                self->tab_kinds_0x12C[2] = 3;
                self->tab_kinds_0x12C[3] = 4;
                count = 4;
            }
            break;
        case 3:
            self->tab_kinds_0x12C[0] = 1;
            self->tab_kinds_0x12C[1] = 2;
            self->tab_kinds_0x12C[2] = 3;
            self->tab_kinds_0x12C[3] = 4;
            count = 4;
            break;
        }
        break;
    }
    lb_list_init(&self->tab_list_0x134, count, 0, 0, -1, 0, 0, 3, 0);
    lb_scroll_init(self->plw_0x204, &self->scroll_0x154,
                   self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00], mode);
    self->scroll_0x154.flags_0x10 |= 2;
}

/* 0x803602A4 (0x2E0): feeds the selected goods row to the stats page: the compared and the previewed pieces and
 * the materials. */
void lb_smith_make_preview_set(LbSmithWork* self)
{
    _PLW* plw = self->plw_0x204;
    _EQUIP part_b;
    _EQUIP part_c;
    s32 gun_split;
    _EQUIP part_a;
    LbSmithGoods* goods;

    if (self->count_0x018 == 0) {
        return;
    }
    lb_row_init(&self->row_0x1D4, self->goods_0x264[self->cursor_0x00C].kind_0x00,
                self->goods_0x264[self->cursor_0x00C].item_0x04);
    if ((u32)(self->scroll_0x154.kind_0x00 - 5) <= 1 || (s32)self->scroll_0x154.kind_0x00 == 10) {
        gun_split = 0;
        if (plw->equipB.kind == 11) {
            if (Gunner_opt_ok_ck(&plw->equipB) == 0) {
                lb_scroll_tables_set(&self->scroll_0x154, &plw->equipB, &plw->equipC, NULL, NULL);
                gun_split = 1;
            } else {
                lb_equip_compare_set(&self->scroll_0x154, &self->row_0x1D4);
            }
        } else {
            lb_equip_compare_set(&self->scroll_0x154, &self->row_0x1D4);
        }
        if (self->row_0x1D4.kind == 11) {
            if (Gunner_opt_ok_ck(&self->row_0x1D4) == 0) {
                lb_row_init(&part_b, 12, self->row_0x1D4.item_id);
                lb_row_init(&part_c, 13, self->row_0x1D4.item_id);
                lb_scroll_tables_set(&self->scroll_0x154, NULL, NULL, &self->row_0x1D4, &part_b);
            } else if (gun_split != 0) {
                lb_row_init(&part_b, 12, 0);
                lb_row_init(&part_c, 13, 0);
                lb_scroll_tables_set(&self->scroll_0x154, NULL, NULL, &self->row_0x1D4, &part_b);
            } else {
                lb_equip_base_set(&self->scroll_0x154, &self->row_0x1D4);
            }
        } else if (gun_split != 0) {
            if (self->row_0x1D4.kind == 12) {
                lb_row_init(&part_a, 11, 0);
                lb_row_init(&part_b, 12, self->row_0x1D4.item_id);
                lb_row_init(&part_c, 13, 0);
                lb_scroll_tables_set(&self->scroll_0x154, NULL, NULL, &part_a, &part_b);
            } else if (self->row_0x1D4.kind == 13) {
                lb_row_init(&part_a, 11, 0);
                lb_row_init(&part_b, 12, 0);
                lb_row_init(&part_c, 13, self->row_0x1D4.item_id);
                lb_scroll_tables_set(&self->scroll_0x154, NULL, NULL, &part_a, &part_b);
            } else {
                lb_equip_base_set(&self->scroll_0x154, &self->row_0x1D4);
            }
        } else {
            lb_equip_base_set(&self->scroll_0x154, &self->row_0x1D4);
        }
    } else {
        lb_equip_compare_set(&self->scroll_0x154, &self->row_0x1D4);
        lb_equip_base_set(&self->scroll_0x154, &self->row_0x1D4);
    }
    goods = &self->goods_0x264[self->cursor_0x00C];
    if (goods->recipe_0x0C != NULL) {
        lb_scroll_materials_set(&self->scroll_0x154, goods->recipe_0x0C->materials_0x04, 4);
    } else if (goods->gun_recipe_0x10 != NULL) {
        lb_seisan_materials_get(goods->kind_0x00, goods->item_0x04, 0, self->materials_0x254);
        lb_scroll_materials_set(&self->scroll_0x154, self->materials_0x254, 4);
    }
    lb_scroll_flag_set(&self->scroll_0x154, self->goods_0x264[self->cursor_0x00C].kind_0x00);
}

/* 0x80360584 (0xBC): opens the make goods of the picked kind on the first page. */
void lb_smith_make_open(LbSmithWork* self)
{
    static u8 weapon_kinds[8] = {7, 8, 9, 10, 11, 15, 14, 0};
    static u8 armor_kinds[8] = {5, 1, 2, 3, 4, 0, 0, 0};
    static u8 gun_part_kinds[4] = {11, 12, 13, 0};

    self->sort_0x01C.pick_b_0x80 = 0;
    self->sort_0x01C.pick_a_0x81 = 0;
    self->cursor_0x00C = 0;
    if (self->top_0x0A0.cursor_0x00 == 0) {
        self->kind_0x007 = weapon_kinds[self->kind_list_0x0E0.cursor_0x00];
        if (self->kind_0x007 == 11) {
            self->kind_0x007 = gun_part_kinds[self->part_list_0x100.cursor_0x00];
        }
    } else {
        self->kind_0x007 = armor_kinds[self->kind_list_0x0E0.cursor_0x00];
    }
    self->wait_0x90C = 0;
    lb_smith_tabs_init(self, 1);
    lb_smith_make_page_build(self, 0);
    lb_smith_make_preview_set(self);
    self->model_set_0x008 = 0;
    lb_smith_pl_turn_reset(self->plw_0x204);
}

/* 0x80360640 (0x180): makes the picked goods: puts the piece in the box, charges the price and takes the materials. */
void lb_smith_make_buy(LbSmithWork* self)
{
    _EQUIP row;
    LbSmithRecipe* recipe;
    s32 i;

    lb_row_init(&row, self->goods_0x264[self->pick_0x016].kind_0x00, self->goods_0x264[self->pick_0x016].item_0x04);
    self->box_0x1F8[0] = 0xFFFF;
    self->box_0x1F8[1] = 0xFFFF;
    self->box_0x1F8[2] = 0xFFFF;
    if (row.kind == 12) {
        self->box_0x1F8[1] = userdata_equip_box_add_new((struct Q_UserData*)lobby_world_block, row.kind, row.item_id);
    } else if (row.kind == 13) {
        self->box_0x1F8[2] = userdata_equip_box_add_new((struct Q_UserData*)lobby_world_block, row.kind, row.item_id);
    } else {
        self->box_0x1F8[0] = userdata_equip_box_add_new((struct Q_UserData*)lobby_world_block, row.kind, row.item_id);
    }
    score_add_clamped(-self->goods_0x264[self->pick_0x016].price_0x08, &SMITH_USER->money_0x0018);
    sysSE_req(9);
    recipe = self->goods_0x264[self->pick_0x016].recipe_0x0C;
    if (recipe != NULL) {
        for (i = 0; i < 4; i++) {
            eft052_page_count_add(recipe->materials_0x04[i].item_0x00, recipe->materials_0x04[i].count_0x02);
        }
    } else {
        lb_seisan_materials_get(self->goods_0x264[self->cursor_0x00C].kind_0x00,
                                self->goods_0x264[self->cursor_0x00C].item_0x04, 0, self->materials_0x254);
        for (i = 0; i < 4; i++) {
            eft052_page_count_add(self->materials_0x254[i].item_0x00, self->materials_0x254[i].count_0x02);
        }
    }
    self->tab_list_0x134.cursor_0x00 = 0;
    lb_scroll_kind_set(&self->scroll_0x154, self->tab_kinds_0x12C[0]);
    lb_smith_pl_model_reset(self);
    pl_model_refresh_req(self->plw_0x204);
}

/* 0x803607C0 (0x4C): equips the made piece and refreshes the player's records. */
void lb_smith_bought_equip(LbSmithWork* self)
{
    lb_equip_entry_apply(self->box_0x1F8[0]);
    userdata_pl_look_apply(self->plw_0x204, lobby_world_block);
    lb_smith_records_copy(self->plw_0x204, self, 2);
}

/* 0x8036080C (0xC): resets the player model's turn. */
void lb_smith_pl_turn_reset(_PLW* plw)
{
    plw->field_0x032 = 0;
}

/* 0x80360818 (0x8C): turns the player model with the shoulder buttons. */
void lb_smith_pl_turn_step(_PLW* plw)
{
    u16 press;

    if (chk_pointer_c1() != 1) {
        if (lobby_net_busy_ck() == 1) {
            return;
        }
        press = Psw[0].button_0x2C0.press_0x08;
        if ((press & 0xC00) != 0) {
            if ((press & 0x800) != 0) {
                plw->field_0x032 -= 0x400;
            }
            if ((press & 0x400) != 0) {
                plw->field_0x032 += 0x400;
            }
        }
    }
}

/* 0x803608A4 (0xA0): sets the lobby view for the model preview while the model tab is shown. */
void lb_smith_pl_view_step(LbSmithWork* self)
{
    if (self->scroll_0x154.kind_0x00 != 3) {
        return;
    }
    if ((u8)(lobby_w.field_0x0B0 + 0xFF) <= 1) {
        system_lobby_flag_set(0);
        pl_model_refresh_req(self->plw_0x204);
        return;
    }
    switch (self->top_0x0A0.cursor_0x00) {
    case 0:
        system_lobby_flag_set(2);
        lb_smith_pl_turn_step(self->plw_0x204);
        break;
    case 1:
        system_lobby_flag_set(1);
        lb_smith_pl_turn_step(self->plw_0x204);
        break;
    }
}

/* 0x80360944 (0x6C): puts the player's own equipment back on its model. */
void lb_smith_pl_model_reset(LbSmithWork* self)
{
    lb_smith_pl_refresh(self->plw_0x204, self);
    if (self->scroll_0x154.kind_0x00 == 3) {
        pl_model_refresh_req(self->plw_0x204);
    } else if (self->model_set_0x008 != 0) {
        pl_model_refresh_req(self->plw_0x204);
    }
    userdata_pl_look_apply(self->plw_0x204, lobby_world_block);
}

/* 0x803609B0 (0x90): opens the box pane on the made piece. */
void lb_smith_box_pane_open(LbSmithWork* self)
{
    s16 box;

    lb_list_pane_init((struct LbEntryPane*)&self->pane_0x3A4, 6, self->plw_0x204);
    if (self->box_0x1F8[0] != 0xFFFF) {
        box = self->box_0x1F8[0];
    } else if (self->box_0x1F8[1] != 0xFFFF) {
        box = self->box_0x1F8[1];
    } else {
        box = self->box_0x1F8[2] != 0xFFFF ? (s16)self->box_0x1F8[2] : (s16)-1;
    }
    lb_list_pane_pos_set((struct LbEntryPane*)&self->pane_0x3A4, box, -1, -1);
    sysSE_req(8);
}

/* 0x80360A40 (0x18): holds the model preview back for 15 frames while the model tab is shown. */
void lb_smith_preview_wait_set(LbSmithWork* self)
{
    if (self->scroll_0x154.kind_0x00 == 3) {
        self->wait_0x90C = 15;
    }
}

#pragma optimization_level 4
/* 0x80360A58 (0xA08): steps the make page: the goods list and its tabs, the confirm, the box pane and the gun-part
 * set pane; returns 2 when the page closes. */
s32 lb_smith_make_step(LbSmithWork* self)
{
    s32 ret = 0;
    u8 tab;
    s16 prev_tab;
    u8 mask;
    _EQUIP row;
    _EQUIP part_b;
    _EQUIP part_c;
    u8 rgb[4];
    s32 rgba;
    s32 b;
    u16 box_a;
    u16 box_b;
    u16 box_c;
    s32 slot;
    s32 i;

    if (self->wait_0x90C > 0) {
        self->wait_0x90C--;
    }
    switch (self->step_0x001) {
    case 0:
        if (file_loading_ck(NULL, NULL) == 1 || self->wait_0x90C > 0 || lb_page_record_empty_ck(self->plw_0x204) == 0) {
            lb_smith_pl_view_step(self);
            break;
        }
        if (lb_cmd_held_ck(0x80) != 0 && self->count_0x018 != 0) {
            if (self->held_0x005 == 0) {
                sysSE_req(5);
            }
            self->held_0x005 = 1;
            if (lb_cmd_repeat_ck(3) != 0) {
                prev_tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
                self->tab_list_0x134.cursor_0x00 = menu_cursor_step(self->tab_list_0x134.cursor_0x00,
                                                                    self->tab_list_0x134.count_0x06,
                                                                    lb_cmd_repeat_get(), 1, 2);
                lb_scroll_kind_set(&self->scroll_0x154,
                                   self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]);
                lb_smith_make_preview_set(self);
                if (prev_tab != self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00] &&
                    self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00] == 3) {
                    pl_model_refresh_req(self->plw_0x204);
                }
            }
        } else {
            self->held_0x005 = 0;
            if (lb_cmd_pressed_ck(0x10) != 0) {
                if (self->goods_0x264[self->cursor_0x00C].flags_0x02 == 0) {
                    self->step_0x001 = 1;
                    self->pick_0x016 = self->cursor_0x00C;
                    userdata_equip_flag_clear(self->goods_0x264[self->cursor_0x00C].kind_0x00,
                                              self->goods_0x264[self->cursor_0x00C].item_0x04, 2);
                    if ((u8)equip_kind_table_class(self->kind_0x007) == 0) {
                        mask = userdata_equip_mask_get((struct Q_UserData*)lobby_world_block);
                        if (lb_equip_flags_ck(self->goods_0x264[self->cursor_0x00C].kind_0x00,
                                              self->goods_0x264[self->cursor_0x00C].item_0x04, mask) == 1) {
                            self->step_0x001 = 1;
                        } else {
                            self->step_0x001 = 4;
                        }
                    } else {
                        self->step_0x001 = 1;
                    }
                    self->confirm_0x200 = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                lb_smith_pl_model_reset(self);
                ret = 2;
                sysSE_req(1);
                break;
            } else if (lb_cmd_pressed_ck(0x40) != 0) {
                tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
                if ((u32)(tab - 3) > 1 && (u32)(tab - 9) > 1) {
                    self->step_0x001 = 5;
                    lb_eqshop_sort_init((struct LbShopSortWork*)&self->sort_0x01C, self->kind_0x007);
                    sysSE_req(5);
                }
            } else if (lb_cmd_repeat_ck(3) != 0) {
                tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
                if (tab != 4 && tab != 10) {
                    self->cursor_0x00C = menu_cursor_step(self->cursor_0x00C, self->count_0x018, lb_cmd_repeat_get(), 1, 2);
                    lb_smith_make_preview_set(self);
                    lb_smith_preview_wait_set(self);
                }
            } else if (lb_cmd_repeat_ck(0xC) != 0) {
                tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
                if (tab != 4 && tab != 10) {
                    self->page_0x012 = menu_cursor_step_forward(self->page_0x012, self->pages_0x014, lb_cmd_repeat_get(),
                                                                4, 8, 6, &self->page_moved_0x00A);
                    lb_smith_make_page_build(self, self->page_0x012);
                    lb_smith_preview_wait_set(self);
                }
            } else if (lobby_cmd_release_ck(0xC00) != 0 && self->count_0x018 != 0) {
                self->scroll_0x154.cursor_0x08 = menu_cursor_step_fixed_tail(
                    self->scroll_0x154.cursor_0x08, self->scroll_0x154.count_0x0A, lobby_cmd_release_get(), 0x800, 0x400,
                    &self->scroll_0x154.moved_0x06);
            }
        }
        lb_smith_pl_view_step(self);
        if (file_loading_ck(NULL, NULL) == 1 || lb_page_record_empty_ck(self->plw_0x204) == 0) {
            break;
        }
        if (system_lobby_flag_get() != 0) {
            if (self->count_0x018 == 0) {
                pl_model_refresh_req(self->plw_0x204);
                break;
            }
            lb_row_init(&row, self->goods_0x264[self->cursor_0x00C].kind_0x00,
                        self->goods_0x264[self->cursor_0x00C].item_0x04);
            switch ((u8)equip_kind_table_class(row.kind)) {
            case 0:
                if (EnableChangeColor(&row) == 1) {
                    rgba = equip_color_rgba_get(equip_color_index_get(&row));
                    color_rgba_copy(rgb, (u8*)&rgba);
                    b = (rgb[2] & 0xF8) >> 3;
                    equip_color_set(&row, b | (((rgb[0] & 0xF8) << 8) | ((rgb[1] & 0xFC) << 3)));
                }
                pl_equip_model_set(self->plw_0x204, (u8*)&row);
                break;
            case 1:
                switch (row.kind) {
                case 11:
                    if (Gunner_opt_ok_ck(&row) == 0) {
                        lb_row_init(&part_b, 12, row.item_id);
                        lb_row_init(&part_c, 13, row.item_id);
                    } else {
                        lb_row_init(&part_b, 12, 0);
                        lb_row_init(&part_c, 13, 0);
                    }
                    pl_equip_models_set(self->plw_0x204, (u8*)&row, (u8*)&part_b, (u8*)&part_c);
                    break;
                default:
                    pl_equip_models_set(self->plw_0x204, (u8*)&row, NULL, NULL);
                    break;
                }
                break;
            case 2:
                lb_row_init(&row, 11, 0);
                switch (self->goods_0x264[self->cursor_0x00C].kind_0x00) {
                case 12:
                    lb_row_init(&part_b, 12, self->goods_0x264[self->cursor_0x00C].item_0x04);
                    lb_row_init(&part_c, 13, 0);
                    pl_equip_models_set(self->plw_0x204, (u8*)&row, (u8*)&part_b, (u8*)&part_c);
                    break;
                case 13:
                    lb_row_init(&part_b, 12, 0);
                    lb_row_init(&part_c, 13, self->goods_0x264[self->cursor_0x00C].item_0x04);
                    pl_equip_models_set(self->plw_0x204, (u8*)&row, (u8*)&part_b, (u8*)&part_c);
                    break;
                }
                break;
            }
            self->model_set_0x008 = 1;
            lb_equip_kind_se_set(&row);
        } else {
            lb_smith_pl_refresh(self->plw_0x204, self);
        }
        break;
    case 5:
        lb_smith_pl_view_step(self);
        switch (lb_eqshop_sort_step((struct LbShopSortWork*)&self->sort_0x01C)) {
        case 1:
            self->step_0x001 = 0;
            self->page_0x012 = 0;
            self->cursor_0x00C = 0;
            lb_smith_make_page_build(self, 0);
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 1:
        lb_smith_pl_view_step(self);
        switch (toggle_word_step_c1(&self->confirm_0x200, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            lb_smith_tabs_init(self, 1);
            lb_smith_pl_model_reset(self);
            pl_model_refresh_req(self->plw_0x204);
            self->step_0x001 = 2;
            if (game_ready_ck() == 0) {
                lb_npc_scene_start(1);
            } else {
                lb_npc_scene_start(0);
            }
            lb_smith_make_buy(self);
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 2:
        ainpc_page_hold_clear();
        pl_model_refresh_req(self->plw_0x204);
        if (lb_npc_scene_done_ck() == 1) {
            lb_smith_make_page_build(self, self->page_0x012);
            if ((u8)equip_kind_table_class(self->kind_0x007) == 0) {
                if (lb_item_flags_ck(&SMITH_USER->box_0x0E00[self->box_0x1F8[0]],
                                     userdata_equip_mask_get((struct Q_UserData*)lobby_world_block)) == 1) {
                    self->step_0x001 = 3;
                } else {
                    self->step_0x001 = 7;
                    lb_smith_box_pane_open(self);
                }
            } else {
                self->step_0x001 = 3;
            }
        }
        break;
    case 3:
        ainpc_page_hold_clear();
        pl_model_refresh_req(self->plw_0x204);
        switch (toggle_word_step_c1(&self->confirm_0x200, lobby_cmd_trig_get(), 4, 8, 0xFFFE)) {
        case 1:
            if ((u32)(self->kind_0x007 - 11) > 2) {
                self->step_0x001 = 0;
                lb_smith_bought_equip(self);
                lb_smith_tabs_init(self, 1);
                sysSE_stop(3);
            } else {
                self->step_0x001 = 6;
                box_a = 0xFFFF;
                box_b = 0xFFFF;
                box_c = 0xFFFF;
                if (self->kind_0x007 == 11) {
                    box_a = self->box_0x1F8[0];
                    slot = 0;
                }
                if (self->kind_0x007 == 12) {
                    box_b = self->box_0x1F8[1];
                    slot = 1;
                }
                if (self->kind_0x007 == 13) {
                    box_c = self->box_0x1F8[2];
                    slot = 2;
                }
                lb_list_pane_init((struct LbEntryPane*)&self->set_box_0x5D8, 0, self->plw_0x204);
                lb_equip_set_pane_init((struct LbEquipSetPane*)&self->set_0x80C, slot, box_a, box_b, box_c, 1,
                                       self->plw_0x204);
                sysSE_stop(0x2B);
            }
            break;
        case 2:
            self->step_0x001 = 7;
            lb_smith_box_pane_open(self);
            break;
        }
        break;
    case 4:
        lb_smith_pl_view_step(self);
        switch (lb_yes_no_step(&self->confirm_0x200)) {
        case 1:
            self->step_0x001 = 1;
            self->confirm_0x200 = 0;
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 6:
        lb_smith_pl_view_step(self);
        switch (lb_equip_set_pane_step((struct LbEquipSetPane*)&self->set_0x80C, (struct LbEntryPane*)&self->set_box_0x5D8,
                                       0)) {
        case 1:
            self->step_0x001 = 0;
            for (i = 0; i < 3; i++) {
                if (self->set_0x80C.box_0x04[i] != 0xFFFF) {
                    lb_equip_entry_apply(self->set_0x80C.box_0x04[i]);
                }
            }
            userdata_pl_look_apply(self->plw_0x204, lobby_world_block);
            lb_entry_pane_list_reset((struct LbEntryPane*)&self->pane_0x3A4, 0);
            lb_smith_records_copy(self->plw_0x204, self, 2);
            lb_smith_tabs_init(self, 1);
            break;
        case 2:
            self->step_0x001 = 7;
            lb_smith_box_pane_open(self);
            break;
        }
        break;
    case 7:
        if (lb_cmd_pressed_ck(0x30) != 0) {
            self->step_0x001 = 0;
            lb_smith_make_page_build(self, self->page_0x012);
            sysSE_req(0);
        }
        break;
    }
    if (self->count_0x018 == 0) {
        pl_model_refresh_req(self->plw_0x204);
    }
    return ret;
}

#pragma optimization_level reset

/* 0x80361460 (0x60): feeds the selected decoration recipe's materials and item to the stats page. */
void lb_smith_deco_preview_set(LbSmithWork* self)
{
    lb_scroll_materials_set(&self->scroll_0x154,
                            self->goods_0x264[self->cursor_0x00C].recipe_0x0C->materials_0x04, 4);
    lb_scroll_item_set(&self->scroll_0x154, self->goods_0x264[self->cursor_0x00C].item_0x04);
}

/* 0x803614C0 (0x17C): builds page `page` of the decoration recipes the slot filter passes. */
void lb_smith_deco_page_build(LbSmithWork* self, s16 page)
{
    s32 first;
    s32 n;
    LbSmithRecipe* table;
    LbSmithRecipe* recipe;
    s32 idx;
    u16 item;
    s32 last;
    u16 flags;
    ItemDataRecord* data;
    ItemSpeciesRecord* species;
    LbSmithGoods* goods;

    first = page * 8;
    last = first + 7;
    lb_smith_rows_clear(self, page);
    table = deco_sei_tbl_ptr;
    n = 0;
    self->count_0x018 = 0;
    idx = 0;
    for (;;) {
        if (idx == -1) {
            break;
        }
        recipe = &table[idx];
        item = recipe->item_0x02;
        flags = lb_deco_row_flags_get(idx);
        if ((flags & 0x10) == 0) {
            data = GetItemData(item);
            species = get_item_species_data(data->species);
            if (lb_deco_class_ck(self->sort_0x01C.pick_a_0x81, data->species) == 1) {
                if (first <= n && n <= last) {
                    goods = &self->goods_0x264[self->count_0x018];
                    goods->kind_0x00 = self->kind_0x007;
                    goods->item_0x04 = item;
                    goods->price_0x08 = eft052_item_half_get(item);
                    goods->recipe_0x0C = recipe;
                    goods->gun_recipe_0x10 = NULL;
                    goods->species_0x14 = species;
                    goods->flags_0x02 = flags;
                    self->count_0x018++;
                }
                n++;
            }
        }
        idx++;
        if (table[idx].item_0x02 == 0xFFFF) {
            idx = -1;
        }
    }
    if (self->cursor_0x00C >= self->count_0x018) {
        self->cursor_0x00C = self->count_0x018 - 1;
        if (self->cursor_0x00C < 0) {
            self->cursor_0x00C = 0;
        }
    }
    self->pages_0x014 = menu_page_count(n, 8);
    if (self->pages_0x014 == 0) {
        self->pages_0x014++;
    }
    lb_smith_deco_preview_set(self);
}

/* 0x8036163C (0x50): opens the decoration recipes on the first page. */
void lb_smith_deco_open(LbSmithWork* self)
{
    self->hold_0x004 = 0;
    self->sort_0x01C.pick_b_0x80 = 0;
    self->sort_0x01C.pick_a_0x81 = 0;
    self->cursor_0x00C = 0;
    lb_smith_tabs_init(self, 5);
    lb_smith_deco_page_build(self, 0);
}

/* 0x8036168C (0xA8): makes the picked decoration: takes the materials, charges the price and puts it in the pages. */
void lb_smith_deco_buy(LbSmithWork* self)
{
    LbSmithGoods* goods = &self->goods_0x264[self->pick_0x016];
    s32 i;

    for (i = 0; i < 4; i++) {
        eft052_page_count_add(goods->recipe_0x0C->materials_0x04[i].item_0x00,
                              goods->recipe_0x0C->materials_0x04[i].count_0x02);
    }
    score_add_clamped(-goods->price_0x08, &SMITH_USER->money_0x0018);
    sysSE_req(9);
    eft052_page_put(self->plw_0x204, goods->item_0x04, 1, 1);
}

/* 0x80361734 (0xB4): sets the decoration sort chooser up: the filter-or-order list first. */
void lb_smith_deco_sort_init(LbSmithSort* sort)
{
    sort->step_0x82 = 0;
    sort->kind_0x83 = 0xFF;
    memset(&sort->lists_0x00[0], 0, sizeof(LbList));
    memset(&sort->lists_0x00[1], 0, sizeof(LbList));
    memset(&sort->lists_0x00[2], 0, sizeof(LbList));
    memset(sort->values_b_0x60, 0, sizeof(sort->values_b_0x60));
    memset(sort->values_a_0x70, 0, sizeof(sort->values_a_0x70));
    lb_list_init(&sort->lists_0x00[0], 2, 2, 0xDF, 0x55, 0, 0, 3, 0);
}

/* 0x803617E8 (0xB0): sets the second list up for the first list's choice. */
void lb_smith_deco_sort_sub_init(LbSmithSort* sort)
{
    switch (sort->lists_0x00[0].cursor_0x00) {
    case 0:
        lb_list_init(&sort->lists_0x00[1], 2, 2, 0xE1, -1, 0, -1, 3, 0);
        break;
    case 1:
        lb_list_init(&sort->lists_0x00[1], 2, 2, 0xE3, -1, 0, -1, 3, 0);
        sort->values_b_0x60[0] = 0;
        sort->values_b_0x60[1] = 0x12;
        break;
    }
}

/* 0x80361898 (0xD4): takes the second list's choice: 1 once a value is picked, else the third list is set up. */
u8 lb_smith_deco_sort_pick(LbSmithSort* sort)
{
    u8 ret = 0;

    switch (sort->lists_0x00[0].cursor_0x00) {
    case 0:
        switch (sort->lists_0x00[1].cursor_0x00) {
        case 1:
            sort->pick_a_0x81 = 0;
            ret = 1;
            break;
        case 0:
            lb_list_init(&sort->lists_0x00[2], 3, 6, 0xE5, -1, 0, -1, 3, 0);
            sort->values_a_0x70[0] = 11;
            sort->values_a_0x70[1] = 12;
            sort->values_a_0x70[2] = 13;
            break;
        }
        break;
    case 1:
        sort->pick_b_0x80 = sort->values_b_0x60[sort->lists_0x00[1].cursor_0x00];
        ret = 1;
        break;
    }
    return ret;
}

/* 0x8036196C (0x160): steps the decoration sort chooser: 1 decided, 2 cancelled. */
s32 lb_smith_deco_sort_step(LbSmithSort* sort)
{
    s32 ret = 0;

    switch (sort->step_0x82) {
    case 0:
        switch (lb_choice_step(&sort->lists_0x00[0])) {
        case 1:
            sort->step_0x82 = 1;
            sort->lists_0x00[1].cursor_0x00 = 0;
            lb_smith_deco_sort_sub_init(sort);
            break;
        case 2:
            ret = 2;
            break;
        }
        break;
    case 1:
        switch (lb_choice_step(&sort->lists_0x00[1])) {
        case 1:
            sort->lists_0x00[2].cursor_0x00 = 0;
            if (lb_smith_deco_sort_pick(sort) == 0) {
                sort->step_0x82 = 2;
                sysSE_req(0);
            } else {
                ret = 1;
                sysSE_req(0xD);
            }
            break;
        case 2:
            sort->step_0x82 = 0;
            break;
        }
        break;
    case 2:
        switch (lb_choice_step(&sort->lists_0x00[2])) {
        case 1:
            if (sort->lists_0x00[0].cursor_0x00 != 1) {
                sort->pick_a_0x81 = sort->values_a_0x70[sort->lists_0x00[2].cursor_0x00];
            } else {
                sort->pick_b_0x80 = sort->values_b_0x60[sort->lists_0x00[2].cursor_0x00];
            }
            ret = 1;
            sysSE_req(0xD);
            break;
        case 2:
            sort->step_0x82 = 1;
            break;
        }
        break;
    }
    return ret;
}

/* 0x80361ACC (0x238): steps the decoration make page; returns 2 when it closes. */
s32 lb_smith_deco_step(LbSmithWork* self)
{
    s32 ret = 0;

    switch (self->step_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (self->goods_0x264[self->cursor_0x00C].flags_0x02 == 0) {
                self->step_0x001 = 1;
                self->pick_0x016 = self->cursor_0x00C;
                self->confirm_0x200 = 0;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            ret = 2;
            sysSE_req(1);
        } else if (lb_cmd_pressed_ck(0x40) != 0) {
            self->step_0x001 = 5;
            lb_smith_deco_sort_init(&self->sort_0x01C);
            sysSE_req(5);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->cursor_0x00C = menu_cursor_step(self->cursor_0x00C, self->count_0x018, lb_cmd_repeat_get(), 1, 2);
            lb_smith_deco_preview_set(self);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->page_0x012 = menu_cursor_step_forward(self->page_0x012, self->pages_0x014, lb_cmd_repeat_get(), 4, 8, 6,
                                                        &self->page_moved_0x00A);
            lb_smith_deco_page_build(self, self->page_0x012);
        }
        break;
    case 1:
        switch (toggle_word_step_c1(&self->confirm_0x200, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            lb_smith_deco_buy(self);
            lb_smith_deco_page_build(self, self->page_0x012);
            self->step_0x001 = 0;
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 5:
        switch (lb_smith_deco_sort_step(&self->sort_0x01C)) {
        case 1:
            self->step_0x001 = 0;
            self->page_0x012 = 0;
            self->cursor_0x00C = 0;
            lb_smith_deco_page_build(self, 0);
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    }
    return ret;
}

/* 0x80361D04 (0xCC): previews the selected decoration set in (attach) or taken out of (detach) the box piece. */
void lb_smith_attach_preview_set(LbSmithWork* self)
{
    _EQUIP rec;
    LbSmithDeco* deco;

    equip_record_copy(&rec, &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E]);
    equip_record_copy(&self->row_0x1D4, &rec);
    deco = &self->decos_0x324[self->cursor_0x00C];
    if (deco->flags_0x0E == 0) {
        switch (self->state_0x000) {
        case 7:
            equip_deco_attach(&self->row_0x1D4, deco->species_id_0x0A);
            break;
        case 8:
            equip_deco_detach(&self->row_0x1D4, deco->slot_0x0C);
            break;
        }
    }
    lb_scroll_tables_set(&self->scroll_0x154, &rec, NULL, &self->row_0x1D4, NULL);
    lb_scroll_item_set(&self->scroll_0x154, self->decos_0x324[self->cursor_0x00C].item_0x08);
}

/* 0x80361DD0 (0x24): opens the box pane for the attach page. */
void lb_smith_attach_open(LbSmithWork* self)
{
    self->sort_0x01C.pick_b_0x80 = 0;
    self->sort_0x01C.pick_a_0x81 = 0;
    self->cursor_0x00C = 0;
    lb_list_pane_init((struct LbEntryPane*)&self->pane_0x3A4, 9, self->plw_0x204);
}

/* 0x80361DF4 (0xB8): sets the selected decoration in the box piece and charges the price. */
void lb_smith_attach_apply(LbSmithWork* self)
{
    LbSmithDeco* deco = &self->decos_0x324[self->cursor_0x00C];

    equip_deco_attach(&SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E], deco->species_id_0x0A);
    userdata_pl_look_apply(self->plw_0x204, lobby_world_block);
    lb_smith_records_copy(self->plw_0x204, self, 2);
    eft052_page_count_add(deco->item_0x08, 1);
    score_add_clamped(-deco->price_0x04, &SMITH_USER->money_0x0018);
    if (userdata_equip_box_worn_ck(lobby_world_block, self->pane_0x3A4.box_0x01E) == 1) {
        lobby_w.param_0x12D = 1;
    }
}

/* 0x80361EAC (0x198): builds page `page` of the held decorations the slot filter passes. */
void lb_smith_attach_page_build(LbSmithWork* self, s16 page)
{
    s32 n;
    s32 first;
    _EQUIP* box;
    s32 item;
    s32 last;
    u16 flags;
    ItemDataRecord* data;
    ItemSpeciesRecord* species;
    LbSmithDeco* deco;

    first = page * 8;
    last = first + 7;
    lb_smith_rows_clear(self, page);
    box = &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E];
    n = 0;
    self->count_0x018 = 0;
    item = 0;
    do {
        data = GetItemData(item);
        if (data->kind_0x00 == 3 && userdata_item_count_total(item, lobby_world_block) > 0) {
            species = get_item_species_data(data->species);
            if (lb_deco_class_ck(self->sort_0x01C.pick_a_0x81, data->species) == 1) {
                if (first <= n && n <= last) {
                    deco = &self->decos_0x324[self->count_0x018];
                    deco->item_0x08 = item;
                    deco->species_id_0x0A = data->species;
                    deco->species_0x00 = species;
                    deco->price_0x04 = species->unused_0x004;
                    flags = 0;
                    if (deco->price_0x04 > SMITH_USER->money_0x0018) {
                        flags |= 4;
                    }
                    if (equip_deco_fit_ck(box, deco->species_id_0x0A) == 0) {
                        flags |= 0x20;
                    }
                    deco->flags_0x0E = flags;
                    self->count_0x018++;
                }
                n++;
            }
        }
        item++;
    } while (item < 0x2EB);
    if (self->cursor_0x00C >= self->count_0x018) {
        self->cursor_0x00C = self->count_0x018 - 1;
        if (self->cursor_0x00C < 0) {
            self->cursor_0x00C = 0;
        }
    }
    self->pages_0x014 = menu_page_count(n, 8);
    if (self->pages_0x014 == 0) {
        self->pages_0x014++;
    }
    lb_smith_attach_preview_set(self);
}

/* 0x80362044 (0x454): steps the attach page: the box pane, the decoration list, the confirm and the sort chooser;
 * returns 2 when it closes. */
s32 lb_smith_attach_step(LbSmithWork* self)
{
    s32 ret = 0;
    LbSmithPane* pane = &self->pane_0x3A4;
    _EQUIP* box;
    u8 tab;

    lb_entry_pane_frame_begin((struct LbEntryPane*)pane);
    switch (self->step_0x001) {
    case 0:
        if (lb_entry_pane_menu_step((struct LbEntryPane*)pane, 0, 0) == 0) {
            if (lb_cmd_pressed_ck_c1(0x10) == 1) {
                pane->box_0x01E = lb_digit_pane_value((struct LbEntryPane*)pane);
                box = &SMITH_USER->box_0x0E00[pane->box_0x01E];
                if (box->kind != 0 && equip_deco_slots_free(box) != 0) {
                    self->kind_0x007 = box->kind;
                    self->step_0x001 = 1;
                    self->cursor_0x00C = 0;
                    lb_smith_tabs_init(self, 4);
                    lb_smith_attach_page_build(self, 0);
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
                lb_scroll_kind_set(&self->scroll_0x154,
                                   self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]);
            } else if (lb_cmd_pressed_ck_c1(0x20) == 1) {
                sysSE_req(1);
                ret = 2;
            } else {
                lb_entry_pane_input((struct LbEntryPane*)pane);
            }
        }
        break;
    case 1:
        if (lb_cmd_held_ck(0x80) != 0 && self->count_0x018 != 0) {
            if (self->held_0x005 == 0) {
                sysSE_req(5);
            }
            self->held_0x005 = 1;
            if (lb_cmd_repeat_ck(3) != 0) {
                self->tab_list_0x134.cursor_0x00 = menu_cursor_step(self->tab_list_0x134.cursor_0x00,
                                                                    self->tab_list_0x134.count_0x06,
                                                                    lb_cmd_repeat_get(), 1, 2);
                lb_scroll_kind_set(&self->scroll_0x154,
                                   self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]);
                lb_smith_attach_preview_set(self);
            }
        } else {
            self->held_0x005 = 0;
            if (lb_cmd_pressed_ck_c1(0x10) == 1) {
                if (self->decos_0x324[self->cursor_0x00C].flags_0x0E == 0) {
                    self->step_0x001 = 2;
                    self->confirm_0x200 = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
            } else if (lb_cmd_pressed_ck_c1(0x20) == 1) {
                self->step_0x001 = 0;
                sysSE_req(1);
            } else if (lobby_cmd_release_ck(0xC00) != 0) {
                if (self->count_0x018 != 0) {
                    self->scroll_0x154.cursor_0x08 = menu_cursor_step_fixed_tail(
                        self->scroll_0x154.cursor_0x08, self->scroll_0x154.count_0x0A, lobby_cmd_release_get(), 0x800,
                        0x400, &self->scroll_0x154.moved_0x06);
                }
            } else {
                tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
                if (tab != 4 && tab != 10) {
                    if (lb_cmd_pressed_ck(0x40) != 0 && (self->count_0x018 != 0 || self->sort_0x01C.pick_a_0x81 != 0)) {
                        self->step_0x001 = 3;
                        lb_smith_deco_sort_init(&self->sort_0x01C);
                        sysSE_req(5);
                    } else if (lb_cmd_repeat_ck(3) != 0) {
                        self->cursor_0x00C = menu_cursor_step(self->cursor_0x00C, self->count_0x018, lb_cmd_repeat_get(),
                                                              1, 2);
                        lb_smith_attach_preview_set(self);
                    } else if (lb_cmd_repeat_ck(0xC) != 0) {
                        self->page_0x012 = menu_cursor_step_forward(self->page_0x012, self->pages_0x014,
                                                                    lb_cmd_repeat_get(), 4, 8, 6, &self->page_moved_0x00A);
                        lb_smith_attach_page_build(self, self->page_0x012);
                    }
                }
            }
        }
        break;
    case 2:
        switch (toggle_word_step_c1(&self->confirm_0x200, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            lb_smith_attach_apply(self);
            lb_smith_attach_page_build(self, self->page_0x012);
            sysSE_stop(0xA);
            self->step_0x001 = 0;
            break;
        case 2:
            self->step_0x001 = 1;
            break;
        }
        break;
    case 3:
        switch (lb_smith_deco_sort_step(&self->sort_0x01C)) {
        case 1:
            self->step_0x001 = 1;
            self->page_0x012 = 0;
            self->cursor_0x00C = 0;
            lb_smith_attach_page_build(self, 0);
            break;
        case 2:
            self->step_0x001 = 1;
            break;
        }
        break;
    }
    return ret;
}

/* 0x80362498 (0x1C): opens the box pane for the detach page. */
void lb_smith_detach_open(LbSmithWork* self)
{
    self->cursor_0x00C = 0;
    lb_list_pane_init((struct LbEntryPane*)&self->pane_0x3A4, 10, self->plw_0x204);
}

/* 0x803624B4 (0xBC): takes the selected decoration out of the box piece into the item box and charges the fee. */
void lb_smith_detach_apply(LbSmithWork* self)
{
    LbSmithDeco* deco = &self->decos_0x324[self->cursor_0x00C];

    equip_deco_detach(&SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E], deco->slot_0x0C);
    userdata_pl_look_apply(self->plw_0x204, lobby_world_block);
    lb_smith_records_copy(self->plw_0x204, self, 2);
    item_box_store(deco->item_0x08, 1, &self->stored_0x1FE);
    score_add_clamped(-deco->price_0x04, &SMITH_USER->money_0x0018);
    if (userdata_equip_box_worn_ck(lobby_world_block, self->pane_0x3A4.box_0x01E) == 1) {
        lobby_w.param_0x12D = 1;
    }
}

/* 0x80362570 (0x8): the fee for taking a decoration out. */
s32 lb_smith_detach_price_get(u16 item)
{
    return 10;
}

/* 0x80362578 (0x120): lists the decorations set in the box piece `equip`. */
void lb_smith_detach_list_build(LbSmithWork* self, _EQUIP* equip)
{
    u16 ids[3];
    LbSmithDeco* deco;
    s16 i;
    u16 flags;
    u16* id;

    memset(self->decos_0x324, 0, sizeof(self->decos_0x324));
    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
    self->count_0x018 = equip_deco_count(equip);
    equip_deco_ids_get(equip, ids, 1);
    i = 0;
    deco = self->decos_0x324;
    id = ids;
    for (; i < self->count_0x018; i++) {
        deco->species_id_0x0A = *id;
        deco->species_0x00 = get_item_species_data(*id);
        deco->item_0x08 = deco->species_0x00->item_id;
        deco->slot_0x0C = i;
        deco->price_0x04 = lb_smith_detach_price_get(deco->species_id_0x0A);
        flags = 0;
        if (deco->price_0x04 > SMITH_USER->money_0x0018) {
            flags |= 4;
        }
        if (eft052_page_count_ck(deco->item_0x08, 1) <= 0) {
            flags |= 2;
        }
        deco->flags_0x0E = flags;
        deco++;
        id++;
    }
    self->cursor_0x00C = 0;
    lb_smith_attach_preview_set(self);
}

/* 0x80362698 (0x38C): steps the detach page: the box pane, the decoration list and the confirm; returns 2 when it
 * closes. */
s32 lb_smith_detach_step(LbSmithWork* self)
{
    s32 ret = 0;
    _EQUIP* box;
    u8 tab;

    lb_entry_pane_frame_begin((struct LbEntryPane*)&self->pane_0x3A4);
    switch (self->step_0x001) {
    case 0:
        if (lb_entry_pane_menu_step((struct LbEntryPane*)&self->pane_0x3A4, 0, 0) == 0) {
            if (lb_cmd_pressed_ck_c1(0x10) == 1) {
                self->pane_0x3A4.box_0x01E = lb_digit_pane_value((struct LbEntryPane*)&self->pane_0x3A4);
                box = &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E];
                if (box->kind != 0 && equip_deco_count(box) != 0) {
                    self->kind_0x007 = box->kind;
                    self->step_0x001 = 1;
                    self->cursor_0x00C = 0;
                    lb_smith_tabs_init(self, 4);
                    lb_smith_detach_list_build(self, box);
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
                lb_scroll_kind_set(&self->scroll_0x154,
                                   self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]);
            } else if (lb_cmd_pressed_ck_c1(0x20) == 1) {
                sysSE_req(1);
                ret = 2;
            } else {
                lb_entry_pane_input((struct LbEntryPane*)&self->pane_0x3A4);
            }
        }
        break;
    case 1:
        if (lb_cmd_held_ck(0x80) != 0 && self->count_0x018 != 0) {
            if (self->held_0x005 == 0) {
                sysSE_req(5);
            }
            self->held_0x005 = 1;
            if (lb_cmd_repeat_ck(3) != 0) {
                self->tab_list_0x134.cursor_0x00 = menu_cursor_step(self->tab_list_0x134.cursor_0x00,
                                                                    self->tab_list_0x134.count_0x06,
                                                                    lb_cmd_repeat_get(), 1, 2);
                lb_scroll_kind_set(&self->scroll_0x154,
                                   self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]);
                lb_smith_attach_preview_set(self);
            }
        } else {
            self->held_0x005 = 0;
            if (lb_cmd_pressed_ck_c1(0x10) == 1) {
                if (self->decos_0x324[self->cursor_0x00C].flags_0x0E == 0) {
                    self->step_0x001 = 2;
                    self->confirm_0x200 = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
            } else if (lb_cmd_pressed_ck_c1(0x20) == 1) {
                self->step_0x001 = 0;
                sysSE_req(1);
            } else if (lobby_cmd_release_ck(0xC00) != 0) {
                if (self->count_0x018 != 0) {
                    self->scroll_0x154.cursor_0x08 = menu_cursor_step_fixed_tail(
                        self->scroll_0x154.cursor_0x08, self->scroll_0x154.count_0x0A, lobby_cmd_release_get(), 0x800,
                        0x400, &self->scroll_0x154.moved_0x06);
                }
            } else {
                tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
                if (tab != 4 && tab != 10 && lb_cmd_repeat_ck(3) != 0) {
                    self->cursor_0x00C = menu_cursor_step(self->cursor_0x00C, self->count_0x018, lb_cmd_repeat_get(), 1,
                                                          2);
                    lb_smith_attach_preview_set(self);
                }
            }
        }
        break;
    case 2:
        switch (toggle_word_step_c1(&self->confirm_0x200, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            lb_smith_detach_apply(self);
            sysSE_stop(0xB);
            self->step_0x001 = 3;
            break;
        case 2:
            self->step_0x001 = 1;
            break;
        }
        break;
    case 3:
        if (lb_cmd_pressed_ck(0x30) != 0) {
            self->step_0x001 = 0;
            sysSE_req(0);
        }
        break;
    }
    return ret;
}

/* 0x80362A24 (0x348): feeds the selected upgrade path to the stats page: the box piece against the upgraded piece
 * (a gun's three parts side by side) and the materials. */
void lb_smith_kakou_preview_set(LbSmithWork* self)
{
    _EQUIP base;
    _EQUIP compare;
    _EQUIP base_sub[2];
    _EQUIP compare_sub[2];
    s32 gun_split;
    _EQUIP* row;

    if (self->kakou_count_0x12A == 0) {
        return;
    }
    if (lb_equip_kind_group_get(self->kind_0x007) == 2 && userdata_gunner_ck(lobby_world_block) == 1 &&
        (u8)(self->scroll_0x154.kind_0x00 + 0xFB) <= 1) {
        self->scroll_0x154.flags_0x10 |= 2;
        gun_split = 0;
        memcpy(&base, &self->plw_0x204->equipB, sizeof(_EQUIP));
        memcpy(base_sub, &self->plw_0x204->equipC, sizeof(base_sub));
        memcpy(&compare, &self->plw_0x204->equipB, sizeof(_EQUIP));
        memcpy(compare_sub, &self->plw_0x204->equipC, sizeof(compare_sub));
        if (self->plw_0x204->equipB.kind == 11 && Gunner_opt_ok_ck(&self->plw_0x204->equipB) == 0) {
            gun_split = 1;
        }
        row = &self->kakou_rows_0x20C[self->kakou_cursor_0x128];
        switch (row->kind) {
        case 11:
            memcpy(&base, row, sizeof(_EQUIP));
            if (Gunner_opt_ok_ck(&base) == 0) {
                lb_row_init(&base_sub[0], 12, base.item_id);
                lb_row_init(&base_sub[1], 13, base.item_id);
            } else if (gun_split != 0) {
                lb_row_init(&base_sub[0], 12, 0);
                lb_row_init(&base_sub[1], 13, 0);
            }
            if (Gunner_opt_ok_ck(&SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E]) == 0) {
                memcpy(&compare, &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E], sizeof(_EQUIP));
                lb_row_init(&compare_sub[0], 12, compare.item_id);
                lb_row_init(&compare_sub[1], 13, compare.item_id);
            } else if (gun_split == 0) {
                memcpy(&compare, &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E], sizeof(_EQUIP));
            }
            break;
        }
        lb_scroll_tables_set(&self->scroll_0x154, &compare, compare_sub, &base, base_sub);
        lb_scroll_flag_set(&self->scroll_0x154,
                           self->kakou_rows_0x20C[self->kakou_cursor_0x128].kind);
    } else {
        self->scroll_0x154.flags_0x10 &= ~2;
        lb_scroll_tables_set(&self->scroll_0x154,
                             &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E], NULL,
                             &self->kakou_rows_0x20C[self->kakou_cursor_0x128], NULL);
    }
    if (self->top_0x0A0.cursor_0x00 == 0) {
        if (lb_smith_gun_part_ck(self->kind_0x007) == 1) {
            row = &self->kakou_rows_0x20C[self->kakou_cursor_0x128];
            lb_seisan_materials_get(row->kind, row->item_id, row->deco_count, self->materials_0x254);
            lb_scroll_materials_set(&self->scroll_0x154, self->materials_0x254, 4);
            return;
        }
        row = &self->kakou_rows_0x20C[self->kakou_cursor_0x128];
        lb_scroll_materials_set(&self->scroll_0x154,
                                kakou_data[row->kind][row->item_id].materials_0x00, 3);
        return;
    }
    row = &self->kakou_rows_0x20C[self->kakou_cursor_0x128];
    if (row->kind != 0 && row->item_id != 0) {
        lb_kakou_materials_get(row->kind, row->item_id, row->deco_count, self->materials_0x254);
        lb_scroll_materials_set(&self->scroll_0x154, self->materials_0x254, 4);
    }
}

/* 0x80362D6C (0x60): opens the box pane for the weapon or armour upgrade page. */
void lb_smith_kakou_open(LbSmithWork* self)
{
    u8 slot = self->top_0x0A0.cursor_0x00 == 0 ? 7 : 8;

    lb_list_pane_init((struct LbEntryPane*)&self->pane_0x3A4, slot, self->plw_0x204);
    self->model_set_0x008 = 0;
    lb_smith_pl_turn_reset(self->plw_0x204);
}

/* 0x80362DCC (0x24): whether equipment kind `kind` is a gun part (11-13). */
u32 lb_smith_gun_part_ck(u8 kind)
{
    return (u32)(kind - 11) <= 2;
}

/* 0x80362DF0 (0x154): sets the upgrade paths of the box piece `equip` up: their refusal bits and materials. */
void lb_smith_kakou_rows_set(LbSmithWork* self, _EQUIP* equip)
{
    s32 i;

    self->kakou_cursor_0x128 = 0;
    lb_smith_tabs_init(self, 2);
    if (self->top_0x0A0.cursor_0x00 == 0) {
        if (lb_smith_gun_part_ck(self->kind_0x007) == 1) {
            self->kakou_count_0x12A = 1;
            self->kakou_flags_0x24C[0] = lb_kakou_gun_flags_get(self->kakou_rows_0x20C[0].kind,
                                                                self->kakou_rows_0x20C[0].item_id,
                                                                self->kakou_rows_0x20C[0].deco_count);
            lb_seisan_materials_get(self->kakou_rows_0x20C[0].kind, self->kakou_rows_0x20C[0].item_id,
                                    self->kakou_rows_0x20C[0].deco_count, self->materials_0x254);
        } else {
            self->kakou_count_0x12A = 0;
            for (i = 0; i < 4; i++) {
                if (self->kakou_rows_0x20C[i].item_id != 0) {
                    self->kakou_flags_0x24C[i] = lb_kakou_weapon_flags_get(self->kakou_rows_0x20C[i].kind,
                                                                           self->kakou_rows_0x20C[i].item_id, equip);
                    self->kakou_count_0x12A++;
                    if (userdata_equip_flag_ck(self->kakou_rows_0x20C[i].kind, self->kakou_rows_0x20C[i].item_id,
                                               1) == 0) {
                        userdata_equip_flag_set(self->kakou_rows_0x20C[i].kind, self->kakou_rows_0x20C[i].item_id, 1);
                        userdata_equip_flag_set(self->kakou_rows_0x20C[i].kind, self->kakou_rows_0x20C[i].item_id, 2);
                    }
                }
            }
        }
    } else {
        self->kakou_count_0x12A = 1;
        self->kakou_flags_0x24C[0] = lb_kakou_armor_flags_get(self->kakou_rows_0x20C[0].kind,
                                                              self->kakou_rows_0x20C[0].item_id,
                                                              self->kakou_rows_0x20C[0].deco_count);
        lb_kakou_materials_get(self->kakou_rows_0x20C[0].kind, self->kakou_rows_0x20C[0].item_id,
                               self->kakou_rows_0x20C[0].deco_count, self->materials_0x254);
    }
    lb_smith_kakou_preview_set(self);
}

/* 0x80362F44 (0x304): upgrades the box piece along the selected path: takes the materials (a weapon also returns its
 * decorations), charges the price and writes the upgraded piece back. */
void lb_smith_kakou_apply(LbSmithWork* self)
{
    _EQUIP* rec;
    LbSmithKakouRecipe* recipe;
    u16 ids[3];
    u16 stored;
    s32 count;
    s16 i;
    s32 j;

    if (self->top_0x0A0.cursor_0x00 == 0) {
        if (lb_smith_gun_part_ck(self->kind_0x007) == 1) {
            rec = &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E];
            equip_gun_level_up(rec);
            for (j = 0; j < 4; j++) {
                if (self->materials_0x254[j].item_0x00 != 0 && self->materials_0x254[j].count_0x02 > 0) {
                    eft052_page_count_add(self->materials_0x254[j].item_0x00, self->materials_0x254[j].count_0x02);
                }
            }
        } else {
            rec = &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E];
            count = equip_deco_count(rec);
            equip_deco_ids_get(rec, ids, 1);
            for (i = 0; i < count; i++) {
                item_box_store(get_item_species_data(ids[i])->item_id, 1, &stored);
            }
            rec = &self->kakou_rows_0x20C[self->kakou_cursor_0x128];
            recipe = kakou_data[rec->kind];
            recipe += rec->item_id;
            for (j = 0; j < 3; j++) {
                if (recipe->materials_0x00[j].item_0x00 != 0 && recipe->materials_0x00[j].count_0x02 > 0) {
                    eft052_page_count_add(recipe->materials_0x00[j].item_0x00, recipe->materials_0x00[j].count_0x02);
                }
            }
        }
    } else if (self->top_0x0A0.cursor_0x00 == 1) {
        rec = &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E];
        equip_armor_level_up(rec);
        for (j = 0; j < 4; j++) {
            if (self->materials_0x254[j].item_0x00 != 0 && self->materials_0x254[j].count_0x02 > 0) {
                eft052_page_count_add(self->materials_0x254[j].item_0x00, self->materials_0x254[j].count_0x02);
            }
        }
    }
    score_add_clamped(-self->kakou_price_0x23C[self->kakou_cursor_0x128], &SMITH_USER->money_0x0018);
    sysSE_req(9);
    userdata_equip_box_set(lobby_world_block, self->pane_0x3A4.box_0x01E, rec);
    userdata_pl_look_apply(self->plw_0x204, lobby_world_block);
    if (userdata_equip_box_worn_ck(lobby_world_block, self->pane_0x3A4.box_0x01E) == 1) {
        lobby_w.param_0x12D = 1;
    }
    lb_smith_records_copy(self->plw_0x204, self, 2);
    self->kind_0x007 = rec->kind;
    if (userdata_equip_box_worn_ck(lobby_world_block, self->pane_0x3A4.box_0x01E) == 0) {
        if ((u8)equip_kind_table_class(rec->kind) == 0) {
            if (lb_item_flags_ck(&SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E],
                                 userdata_equip_mask_get((struct Q_UserData*)lobby_world_block)) == 1) {
                self->box_0x1F8[0] = self->pane_0x3A4.box_0x01E;
            } else {
                self->box_0x1F8[0] = 0xFFFF;
            }
        } else {
            self->box_0x1F8[0] = self->pane_0x3A4.box_0x01E;
        }
    } else {
        self->box_0x1F8[0] = 0xFFFF;
    }
    self->tab_list_0x134.cursor_0x00 = 0;
    lb_scroll_kind_set(&self->scroll_0x154, self->tab_kinds_0x12C[0]);
    lb_smith_pl_model_reset(self);
    pl_model_refresh_req(self->plw_0x204);
}

/* 0x80363248 (0x7E4): steps the upgrade page: the box pane, the upgrade paths and their tabs, the confirm, the box
 * pane after the upgrade and the gun-part set pane; returns 2 when it closes. */
s32 lb_smith_kakou_step(LbSmithWork* self)
{
    s32 ret = 0;
    LbSmithPane* pane = &self->pane_0x3A4;
    _EQUIP* box;
    u8 tab;
    s16 prev_tab;
    _EQUIP row;
    _EQUIP part_b;
    _EQUIP part_c;
    u8 rgb[4];
    s32 rgba;
    s32 b;
    u16 box_a;
    u16 box_b;
    u16 box_c;
    s32 slot;
    s32 i;

    self->pane_0x3A4.redraw_0x008 = 0;
    if (self->top_0x0A0.cursor_0x00 != 0) {
        menu_pad_map_set(0, 3);
    }
    lb_entry_pane_frame_begin((struct LbEntryPane*)pane);
    switch (self->step_0x001) {
    case 0:
        if (lb_page_record_empty_ck(self->plw_0x204) != 0 &&
            lb_entry_pane_menu_step((struct LbEntryPane*)&self->pane_0x3A4, 0, 0) == 0) {
            if (lb_cmd_pressed_ck_c1(0x10) == 1) {
                pane->box_0x01E = lb_digit_pane_value((struct LbEntryPane*)pane);
                box = &SMITH_USER->box_0x0E00[pane->box_0x01E];
                self->kind_0x007 = box->kind;
                if (box->kind != 0) {
                    if (lb_kakou_icon_build_c1(box, self->top_0x0A0.cursor_0x00 != 0, self->kakou_rows_0x20C,
                                               self->kakou_price_0x23C) == 0) {
                        self->step_0x001 = 1;
                        lb_smith_kakou_rows_set(self, box);
                        lb_scroll_kind_set(&self->scroll_0x154,
                                           self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]);
                        lb_smith_kakou_preview_set(self);
                        sysSE_req(0);
                    } else {
                        sysSE_req(2);
                    }
                } else {
                    sysSE_req(2);
                }
            } else if (lb_cmd_pressed_ck_c1(0x20) == 1) {
                sysSE_req(1);
                ret = 2;
            } else {
                lb_entry_pane_input((struct LbEntryPane*)pane);
            }
        }
        break;
    case 1:
        if (file_loading_ck(NULL, NULL) == 1 || self->wait_0x90C > 0 || lb_page_record_empty_ck(self->plw_0x204) == 0) {
            lb_smith_pl_view_step(self);
            break;
        }
        if (lb_cmd_held_ck(0x80) != 0) {
            if (self->held_0x005 == 0) {
                sysSE_req(5);
            }
            self->held_0x005 = 1;
            if (lb_cmd_repeat_ck(3) != 0) {
                prev_tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
                self->tab_list_0x134.cursor_0x00 = menu_cursor_step(self->tab_list_0x134.cursor_0x00,
                                                                    self->tab_list_0x134.count_0x06,
                                                                    lb_cmd_repeat_get(), 1, 2);
                lb_scroll_kind_set(&self->scroll_0x154,
                                   self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]);
                lb_smith_kakou_preview_set(self);
                if (prev_tab != self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00] &&
                    self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00] == 3) {
                    pl_model_refresh_req(self->plw_0x204);
                }
            }
        } else {
            self->held_0x005 = 0;
            if (lb_cmd_pressed_ck(0x10) != 0) {
                userdata_equip_rec_flag_clear(&self->kakou_rows_0x20C[self->kakou_cursor_0x128], 2);
                if (self->kakou_flags_0x24C[self->kakou_cursor_0x128] == 0) {
                    self->step_0x001 = 2;
                    self->confirm_0x200 = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                self->step_0x001 = 0;
                sysSE_req(1);
                lb_smith_pl_model_reset(self);
                break;
            } else if (lobby_cmd_release_ck(0xC00) != 0) {
                self->scroll_0x154.cursor_0x08 = menu_cursor_step_fixed_tail(
                    self->scroll_0x154.cursor_0x08, self->scroll_0x154.count_0x0A, lobby_cmd_release_get(), 0x800, 0x400,
                    &self->scroll_0x154.moved_0x06);
            } else if (lb_cmd_repeat_ck(3) != 0) {
                tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
                if (tab != 4 && tab != 10) {
                    self->kakou_cursor_0x128 = menu_cursor_step(self->kakou_cursor_0x128, self->kakou_count_0x12A,
                                                                lb_cmd_repeat_get(), 1, 2);
                    lb_smith_kakou_preview_set(self);
                }
            }
        }
        lb_smith_pl_view_step(self);
        if (file_loading_ck(NULL, NULL) == 1 || lb_page_record_empty_ck(self->plw_0x204) == 0) {
            break;
        }
        if (system_lobby_flag_get() != 0) {
            equip_record_copy(&row, &self->kakou_rows_0x20C[self->kakou_cursor_0x128]);
            switch ((u8)equip_kind_table_class(row.kind)) {
            case 0:
                if (EnableChangeColor(&row) == 1) {
                    rgba = equip_color_rgba_get(equip_color_index_get(&row));
                    color_rgba_copy(rgb, (u8*)&rgba);
                    b = (rgb[2] & 0xF8) >> 3;
                    equip_color_set(&row, b | (((rgb[0] & 0xF8) << 8) | ((rgb[1] & 0xFC) << 3)));
                }
                pl_equip_model_set(self->plw_0x204, (u8*)&row);
                break;
            case 1:
                switch (row.kind) {
                case 11:
                    if (Gunner_opt_ok_ck(&row) == 0) {
                        lb_row_init(&part_b, 12, row.item_id);
                        lb_row_init(&part_c, 13, row.item_id);
                    } else {
                        lb_row_init(&part_b, 12, 0);
                        lb_row_init(&part_c, 13, 0);
                    }
                    pl_equip_models_set(self->plw_0x204, (u8*)&row, (u8*)&part_b, (u8*)&part_c);
                    break;
                default:
                    pl_equip_models_set(self->plw_0x204, (u8*)&row, NULL, NULL);
                    break;
                }
                break;
            }
            self->model_set_0x008 = 1;
            lb_equip_kind_se_set(&row);
        } else {
            lb_smith_pl_refresh(self->plw_0x204, self);
        }
        break;
    case 2:
        lb_smith_pl_view_step(self);
        switch (toggle_word_step_c1(&self->confirm_0x200, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            lb_smith_kakou_apply(self);
            if (self->box_0x1F8[0] != 0xFFFF) {
                self->step_0x001 = 3;
                self->confirm_0x200 = 0;
            } else {
                self->step_0x001 = 0;
            }
            break;
        case 2:
            self->step_0x001 = 1;
            break;
        }
        break;
    case 3:
        lb_smith_pl_view_step(self);
        switch (toggle_word_step_c1(&self->confirm_0x200, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            if ((u32)(self->kind_0x007 - 11) > 2) {
                self->step_0x001 = 0;
                lb_smith_bought_equip(self);
                sysSE_stop(3);
            } else {
                self->step_0x001 = 5;
                box_a = 0xFFFF;
                box_b = 0xFFFF;
                box_c = 0xFFFF;
                if (self->kind_0x007 == 11) {
                    box_a = self->box_0x1F8[0];
                    slot = 0;
                }
                if (self->kind_0x007 == 12) {
                    box_b = self->box_0x1F8[0];
                    slot = 1;
                }
                if (self->kind_0x007 == 13) {
                    box_c = self->box_0x1F8[0];
                    slot = 2;
                }
                lb_list_pane_init((struct LbEntryPane*)&self->set_box_0x5D8, 0, self->plw_0x204);
                lb_equip_set_pane_init((struct LbEquipSetPane*)&self->set_0x80C, slot, box_a, box_b, box_c, 1,
                                       self->plw_0x204);
                sysSE_stop(0x2B);
            }
            break;
        case 2:
            self->step_0x001 = 0;
            lb_smith_pl_model_reset(self);
            break;
        }
        break;
    case 5:
        lb_smith_pl_view_step(self);
        switch (lb_equip_set_pane_step((struct LbEquipSetPane*)&self->set_0x80C, (struct LbEntryPane*)&self->set_box_0x5D8,
                                       0)) {
        case 1:
            self->step_0x001 = 0;
            lb_smith_pl_model_reset(self);
            for (i = 0; i < 3; i++) {
                if (self->set_0x80C.box_0x04[i] != 0xFFFF) {
                    lb_equip_entry_apply(self->set_0x80C.box_0x04[i]);
                }
            }
            userdata_pl_look_apply(self->plw_0x204, lobby_world_block);
            lb_entry_pane_list_reset((struct LbEntryPane*)&self->pane_0x3A4, 0);
            lb_smith_records_copy(self->plw_0x204, self, 2);
            break;
        case 2:
            self->step_0x001 = 0;
            lb_smith_pl_model_reset(self);
            break;
        }
        break;
    case 4:
        lb_smith_pl_view_step(self);
        switch (lb_yes_no_step(&self->confirm_0x200)) {
        case 1:
            self->step_0x001 = 2;
            self->confirm_0x200 = 0;
            break;
        case 2:
            self->step_0x001 = 0;
            lb_smith_pl_model_reset(self);
            break;
        }
        break;
    }
    return ret;
}

/* 0x80363A2C (0x30): whether the smithy shows the decoration attach or detach page (the skill page previews it). */
u32 lb_smith_deco_page_ck(void)
{
    u8 state = ((LbSmithWork*)lobby_w.menu_0xAC)->state_0x000;

    if (state == 7) {
        return 1;
    }
    return state == 8;
}

#pragma optimization_level 4
/* 0x80363A5C (0x534): the smithy's screen step: the menus, the pages and the close; queues the page draw. */
void lb_screen15_step(void)
{
    LbSmithWork* self = (LbSmithWork*)lobby_w.menu_0xAC;

    self->page_moved_0x00A = 0;
    self->scroll_0x154.moved_0x06 = 0;
    system_lobby_flag_set(0);
    switch (self->state_0x000) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            self->state_0x000++;
            if (self->npc_0x208[2] == 8 && lb_unlock_cond_last_ck(4) == 1) {
                lb_choice_init(&self->top_0x0A0, (const LbListTmpl*)&lb_smith_menu_tmpls[6], 0, 8);
            } else {
                lb_choice_init(&self->top_0x0A0, (const LbListTmpl*)&lb_smith_menu_tmpls[0], 0, 0);
            }
            sysSE_stop(0x29);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&self->top_0x0A0)) {
        case 1:
            self->state_0x000 = 2;
            switch (self->top_0x0A0.cursor_0x00) {
            case 0:
            case 1:
            case 2:
                sysSE_req(5);
                lb_choice_init(&self->mode_0x0C0,
                               (const LbListTmpl*)&lb_smith_menu_tmpls[self->top_0x0A0.cursor_0x00 + 1], 0, 0);
                break;
            case 3:
                sysSE_req(0x1B);
                self->state_0x000 = 9;
                lb_deli_list_build(self->plw_0x204, self->npc_0x208);
                break;
            }
            break;
        case 2:
            self->state_0x000 = 11;
            lb_talk_page_open(1);
            break;
        }
        break;
    case 9:
        ainpc_page_hold_set();
        switch (lb_deli_menu_step()) {
        case 1:
            return;
        case 2:
            self->state_0x000 = 1;
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        switch (lb_choice_step(&self->mode_0x0C0)) {
        case 1:
            self->kind_list_0x0E0.cursor_0x00 = 0;
            switch (self->top_0x0A0.cursor_0x00) {
            case 0:
            case 1:
                switch (self->mode_0x0C0.cursor_0x00) {
                case 0:
                    self->hold_0x004 = 0;
                    self->state_0x000 = 3;
                    if (self->top_0x0A0.cursor_0x00 == 0) {
                        lb_choice_init(&self->kind_list_0x0E0, (const LbListTmpl*)&lb_smith_menu_tmpls[4], 0, 0);
                        if (game_ready_ck() == 0) {
                            if (userdata_progress_flag_ck(0x2F) == 0) {
                                self->kind_list_0x0E0.count_0x06 -= 2;
                            } else if (userdata_progress_flag_ck(0x32) == 0) {
                                self->kind_list_0x0E0.count_0x06 -= 1;
                            }
                        }
                    } else if (self->top_0x0A0.cursor_0x00 == 1) {
                        lb_choice_init(&self->kind_list_0x0E0, (const LbListTmpl*)&lb_smith_menu_tmpls[5], 0, 0);
                    }
                    break;
                case 1:
                    self->state_0x000 = 10;
                    self->step_0x001 = 0;
                    lb_smith_kakou_open(self);
                    break;
                }
                break;
            case 2:
                switch (self->mode_0x0C0.cursor_0x00) {
                case 0:
                    self->state_0x000 = 6;
                    lb_smith_deco_open(self);
                    break;
                case 1:
                    self->state_0x000 = 7;
                    lb_smith_attach_open(self);
                    break;
                case 2:
                    self->state_0x000 = 8;
                    lb_smith_detach_open(self);
                    break;
                }
                break;
            }
            break;
        case 2:
            self->state_0x000 = 1;
            break;
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if (lb_page_record_empty_ck(self->plw_0x204) == 1) {
            switch (lb_choice_step(&self->kind_list_0x0E0)) {
            case 1:
                if (self->kind_list_0x0E0.cursor_0x00 == 4 && self->top_0x0A0.cursor_0x00 == 0) {
                    self->state_0x000 = 4;
                    self->part_list_0x100.cursor_0x00 = 0;
                    lb_list_init(&self->part_list_0x100, 3, 5, 0xAE, -1, 0, 0, 3, 0);
                } else if (self->hold_0x004 == 0) {
                    self->state_0x000 = 5;
                    self->step_0x001 = 0;
                    lb_smith_make_open(self);
                }
                break;
            case 2:
                self->state_0x000 = 2;
                break;
            }
        }
        break;
    case 4:
        ainpc_page_hold_set();
        switch (lb_choice_step(&self->part_list_0x100)) {
        case 1:
            if (self->hold_0x004 == 0) {
                self->state_0x000 = 5;
                self->step_0x001 = 0;
                lb_smith_make_open(self);
            }
            break;
        case 2:
            self->state_0x000 = 3;
            break;
        }
        break;
    case 5:
        ainpc_page_hold_set();
        if ((u32)(lb_smith_make_step(self) - 1) <= 1) {
            self->state_0x000 = 3;
        }
        break;
    case 10:
        ainpc_page_hold_set();
        if ((u32)(lb_smith_kakou_step(self) - 1) <= 1) {
            self->state_0x000 = 2;
        }
        break;
    case 6:
        ainpc_page_hold_set();
        if ((u32)(lb_smith_deco_step(self) - 1) <= 1) {
            self->state_0x000 = 2;
        }
        break;
    case 7:
        ainpc_page_hold_set();
        if ((u32)(lb_smith_attach_step(self) - 1) <= 1) {
            self->state_0x000 = 2;
        }
        break;
    case 8:
        ainpc_page_hold_set();
        if ((u32)(lb_smith_detach_step(self) - 1) <= 1) {
            self->state_0x000 = 2;
        }
        break;
    case 11:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_smith_close();
        }
        break;
    }
    if (self->state_0x000 != 11 && self->state_0x000 != 0) {
        subTransSet((u32)lb_menu_page_step, 0, 0);
    }
}

#pragma optimization_level reset

/* 0x80363F90 (0x328): draws the make goods window: the title, the page arrows, the goods rows and the sort labels. */
void lb_smith_make_goods_draw(LbSmithWork* self, s32 active)
{
    u16 title;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _EQUIP row;
    _SPR_DATA_ label;
    _SPR_DATA_ value;
    _SPR_DATA_* lsp;
    s32 i;
    s32 enabled;
    s32 picked;

    switch (self->kind_0x007) {
    case 11:
        title = 0x1920;
        break;
    case 12:
        title = 0x1921;
        break;
    case 13:
        title = 0x1922;
        break;
    default:
        title = lb_eqshop_goods_titles[self->kind_0x007];
        break;
    }
    get_lsp_data(0x1915, &base);
    draw_sprite_ary(lb_eqshop_goods_frame, &base);
    draw_sprite_ary(lb_eqshop_goods_frame_sub, &base);
    draw_sprite_idx(title, &base);
    lb_page_arrow_draw(0x1914, self->page_0x012, self->pages_0x014, self->page_moved_0x00A, active);
    if (self->count_0x018 == 0) {
        lb_text_center_print(0x1938, (s8*)LbStr(3, 0x74), 2, 0x12, &base, 0);
    } else {
        for (i = 0; i < self->count_0x018; i++) {
            enabled = self->goods_0x264[i].flags_0x02 == 0;
            if (i == self->cursor_0x00C) {
                picked = 1;
                lb_row_init(&row, self->goods_0x264[self->cursor_0x00C].kind_0x00,
                            self->goods_0x264[self->cursor_0x00C].item_0x04);
                lb_equip_cell_draw_c1(0x18FE, &row, 1);
            } else {
                picked = 0;
            }
            lsp = get_lsp_data(lb_eqshop_goods_row_lsp[i], NULL);
            pos.x = base.x + lsp->pos.x;
            pos.y = base.y + lsp->pos.y;
            lb_row_init(&row, self->goods_0x264[i].kind_0x00, self->goods_0x264[i].item_0x04);
            lb_equip_cell_draw_layout5(&row, self->goods_0x264[i].price_0x08, &pos, enabled, picked, active, 2, 1);
        }
    }
    font_set_size(0x10, 0x10);
    spr_data_copy(&label, get_lsp_data(0x1929, NULL));
    spr_data_copy(&value, get_lsp_data(0x192B, NULL));
    label.pos.x = base.x + value.pos.x - 0x30;
    label.pos.y = base.y + value.pos.y;
    lb_font_print(label.pos.x, label.pos.y, (s8*)LbStr(0, 0x10F), label.color, 0);
    spr_data_copy(&label, get_lsp_data(0x192A, NULL));
    spr_data_copy(&value, get_lsp_data(0x192C, NULL));
    label.pos.x = base.x + value.pos.x - 0x30;
    label.pos.y = base.y + value.pos.y;
    lb_font_print(label.pos.x, label.pos.y, (s8*)LbStr(0, 0x110), label.color, 0);
    draw_font_idx(0x192B, lb_sort_filter_name_get(self->sort_0x01C.pick_a_0x81), 4, &base);
    draw_font_idx(0x192C, lb_sort_order_name_get(self->sort_0x01C.pick_b_0x80), 4, &base);
}

#pragma optimization_level 4
/* 0x803642B8 (0x30C): draws the make page: the stats page, the goods window or the sort chooser, the panes and the
 * button prompts. */
void lb_smith_make_draw(LbSmithWork* self)
{
    s32 mode;
    u8 kind;
    u8 tab;
    _mh_ivec2_ pos;

    if ((u8)(self->step_0x001 + 0xFE) <= 1) {
        return;
    }
    lb_smith_make_preview_set(self);
    if ((self->step_0x001 != 6 && self->step_0x001 != 7 && self->count_0x018 != 0) ||
        self->scroll_0x154.kind_0x00 == 3) {
        if (self->held_0x005 != 0) {
            mode = 0;
        } else {
            mode = self->step_0x001 == 0;
        }
        if (self->count_0x018 == 0 && (kind = self->scroll_0x154.kind_0x00) == 3) {
            self->scroll_0x154.kind_0x00 = 9;
            lb_equip_page_draw_c1(&self->scroll_0x154, mode);
            self->scroll_0x154.kind_0x00 = kind;
        } else {
            lb_equip_page_draw_c1(&self->scroll_0x154, mode);
        }
    }
    switch (self->step_0x001) {
    case 0:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            lb_smith_make_goods_draw(self, 1);
        }
        if (self->held_0x005 != 0) {
            tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
            if (tab != 4 && tab != 10) {
                lb_eqshop_scroll_frame_draw(0);
            }
            lb_title_frame_draw(&self->tab_list_0x134, self->tab_kinds_0x12C);
            return;
        }
        switch (self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]) {
        case 4:
        case 10:
            if (self->count_0x018 != 0) {
                if (get_option_cfg(7) == 0) {
                    draw_lsp_sprite_variant(0x694, 0);
                } else {
                    draw_lsp_sprite_variant(0x694, 0xD);
                }
            }
            break;
        case 3:
        case 9:
            if (self->count_0x018 != 0) {
                if (get_option_cfg(7) == 0) {
                    draw_lsp_sprite_variant(0x1999, 0);
                } else {
                    draw_lsp_sprite_variant(0x1999, 0xD);
                }
            }
            break;
        default:
            if (get_option_cfg(7) == 0) {
                if (self->count_0x018 != 0) {
                    draw_lsp_sprite_variant(0x1999, 0);
                }
                draw_lsp_sprite_variant(0x1947, 1);
            } else {
                if (self->count_0x018 != 0) {
                    draw_lsp_sprite_variant(0x1999, 0xD);
                }
                draw_lsp_sprite_variant(0x1947, 0xE);
            }
            break;
        }
        break;
    case 5:
        lb_smith_make_goods_draw(self, 0);
        lb_eqshop_scroll_frame_draw(0);
        lb_eqshop_sort_draw((struct LbShopSortWork*)&self->sort_0x01C, 0);
        break;
    case 1:
    case 3:
    case 4:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            lb_smith_make_goods_draw(self, 0);
        }
        break;
    case 6:
        lb_equip_set_pane_draw((struct LbEquipSetPane*)&self->set_0x80C, 0x1A32, (struct LbEntryPane*)&self->set_box_0x5D8,
                               0x1965, 0);
        break;
    case 7:
        get_lsp_data(0x1965, &pos);
        lb_pane_draw((struct LbEntryPane*)&self->pane_0x3A4, pos.x, pos.y, NULL);
        break;
    }
}

#pragma optimization_level reset

/* 0x803645C4 (0x20): draws the gun-part list. */
void lb_smith_part_list_draw(LbSmithWork* self)
{
    static u16 frame[8] = {0x18F4, 0x18F5, 0x18F6, 0x18F7, 0x18F8, 0x18F9, 0x18FA, 0xFFFF};
    static u16 rows[3] = {0x18FB, 0x18FC, 0x18FD};

    lb_choice_draw(&self->part_list_0x100, 0x18F3, frame, rows, 0, 0);
}

/* 0x803645E4 (0x1AC): draws the decoration sort labels: the slot filter and the order. */
void lb_smith_sort_labels_draw(LbSmithWork* self, _mh_ivec2_* base)
{
    _SPR_DATA_ label;
    _SPR_DATA_ value;
    u16 filter;

    font_set_size(0x10, 0x10);
    spr_data_copy(&label, get_lsp_data(0x1929, NULL));
    spr_data_copy(&value, get_lsp_data(0x192B, NULL));
    label.pos.x = base->x + value.pos.x - 0x30;
    label.pos.y = base->y + value.pos.y;
    lb_font_print(label.pos.x, label.pos.y, (s8*)LbStr(0, 0x10F), label.color, 0);
    spr_data_copy(&label, get_lsp_data(0x192A, NULL));
    spr_data_copy(&value, get_lsp_data(0x192C, NULL));
    label.pos.x = base->x + value.pos.x - 0x30;
    label.pos.y = base->y + value.pos.y;
    lb_font_print(label.pos.x, label.pos.y, (s8*)LbStr(0, 0x111), label.color, 0);
    filter = 0;
    if (self->sort_0x01C.pick_a_0x81 != 0) {
        filter = self->sort_0x01C.pick_a_0x81 - 10;
    }
    draw_font_idx(0x192B, (s8*)LbStr(0, filter + 0xEA), 4, base);
    draw_font_idx(0x192C, (s8*)LbStr(0, (self->sort_0x01C.pick_b_0x80 == 0) + 0xE8), 4, base);
}

/* 0x80364790 (0x19C): draws the decoration recipes window. */
void lb_smith_deco_goods_draw(LbSmithWork* self)
{
    s32 active;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _mh_ivec2_ window;
    _SPR_DATA_* lsp;
    s32 i;
    s32 enabled;
    s32 picked;
    s32 mode;
    LbSmithGoods* goods;

    active = self->step_0x001 == 0;
    get_lsp_data(0x1915, &base);
    draw_sprite_ary(lb_eqshop_goods_frame, &base);
    draw_sprite_idx(0x1928, &base);
    lb_page_arrow_draw(0x1914, self->page_0x012, self->pages_0x014, self->page_moved_0x00A, active);
    if (self->count_0x018 == 0) {
        lb_text_center_print(0x1938, (s8*)LbStr(3, 0x75), 2, 0x12, &base, 0);
    } else {
        goods = self->goods_0x264;
        for (i = 0; i < self->count_0x018; i++, goods++) {
            enabled = self->goods_0x264[i].flags_0x02 == 0;
            if (i == self->cursor_0x00C) {
                mode = 2;
                picked = 1;
                get_lsp_data(0x18FE, &window);
                lb_item_window_draw(goods->item_0x04, &window);
            } else {
                mode = 2;
                picked = 0;
            }
            if (self->sort_0x01C.pick_b_0x80 == 0x12) {
                mode |= 8;
            }
            lsp = get_lsp_data(lb_eqshop_goods_row_lsp[i], NULL);
            pos.x = base.x + lsp->pos.x;
            pos.y = base.y + lsp->pos.y;
            lb_item_cell_draw_layout5(goods->item_0x04, goods->price_0x08, &pos, enabled, picked, active, mode);
        }
    }
    lb_smith_sort_labels_draw(self, &base);
}

/* 0x8036492C (0x2AC): draws the decoration info window of item `item`: its slot size, its two skills and its text. */
void lb_smith_deco_info_draw(_PLW* plw, u16 item)
{
    static u16 frame[16] = {0x1A6E, 0x1A6F, 0x1A70, 0x1A77, 0x1A78, 0x1A79, 0x1A7A, 0x1A7B,
                            0x1A7C, 0x1A6C, 0x1A6D, 0x1A62, 0x1A63, 0x1A6B, 0xFFFF, 0};
    static u16 skill_a_frame[4] = {0x1A71, 0x1A72, 0x1A73, 0xFFFF};
    static u16 skill_b_frame[4] = {0x1A74, 0x1A75, 0x1A76, 0xFFFF};
    ItemSpeciesRecord* species;
    _mh_ivec2_ base;
    PlaceRec rec;
    _SPR_DATA_ spr;
    _SPR_DATA_* lsp;

    species = get_item_species_data(GetItemData(item)->species);
    get_lsp_data(0x1A61, &base);
    draw_sprite_ary(frame, &base);
    draw_font_idx_num(0x1A64, species->slot_size, 1, &base);
    if (species->kind != 0) {
        draw_sprite_ary(skill_a_frame, &base);
        spr_data_copy(&spr, get_lsp_data(0x1A65, NULL));
        if (pl_skill_valid_ck(plw, species->kind) == 0) {
            spr.color = 0x878787FF;
        }
        draw_font(spr, (s8*)item_kind_name_get(species->kind), 0, &base);
        spr_data_copy(&spr, get_lsp_data(0x1A66, NULL));
        if (pl_skill_valid_ck(plw, species->kind) == 0) {
            spr.color = 0x878787FF;
        }
        lb_sprite_num_draw(&spr, (s8)species->kind_points, 2, &base);
        menu_row_rec_init(&rec, species->kind);
        place_rec_alloc_idx(&rec, 0x1A72, &base, -4, 9);
    }
    if (species->kind2 != 0) {
        draw_sprite_ary(skill_b_frame, &base);
        spr_data_copy(&spr, get_lsp_data(0x1A67, NULL));
        if (pl_skill_valid_ck(plw, species->kind2) == 0) {
            spr.color = 0x878787FF;
        }
        draw_font(spr, (s8*)item_kind_name_get(species->kind2), 0, &base);
        spr_data_copy(&spr, get_lsp_data(0x1A68, NULL));
        if (pl_skill_valid_ck(plw, species->kind2) == 0) {
            spr.color = 0x878787FF;
        }
        lb_sprite_num_draw(&spr, (s8)species->kind2_points, 2, &base);
        menu_row_rec_init(&rec, species->kind2);
        place_rec_alloc_idx(&rec, 0x1A75, &base, -4, 9);
    }
    lsp = get_lsp_data(0x1A69, NULL);
    font_set_size(lsp->width, lsp->height);
    font_print_ex(base.x + lsp->pos.x, base.y + lsp->pos.y, 0, (s8*)"%s", ItemExp(item));
    ainpc_page_mark_a_draw(0x1A7D, &base);
}

/* 0x80364BD8 (0xE0): draws the decoration make page. */
void lb_smith_deco_draw(LbSmithWork* self)
{
    if (self->count_0x018 != 0) {
        lb_equip_page_draw_c1(&self->scroll_0x154,
                              self->held_0x005 != 0 ? 0 : self->step_0x001 == 0);
    }
    switch (self->step_0x001) {
    case 0:
        lb_smith_deco_goods_draw(self);
        if (get_option_cfg(7) == 0) {
            draw_lsp_sprite_variant(0x1947, 1);
        } else {
            draw_lsp_sprite_variant(0x1947, 0xE);
        }
        break;
    case 5:
        lb_smith_deco_goods_draw(self);
        lb_eqshop_scroll_frame_draw(0);
        lb_eqshop_sort_draw((struct LbShopSortWork*)&self->sort_0x01C, 1);
        break;
    case 1:
        lb_smith_deco_goods_draw(self);
        break;
    }
}

/* 0x80364CB8 (0x230): draws the held decorations window of the attach page. */
void lb_smith_attach_goods_draw(LbSmithWork* self, s32 active)
{
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _mh_ivec2_ window;
    _SPR_DATA_ spr;
    _SPR_DATA_* lsp;
    s16 lines;
    s32 mode;
    s32 enabled;
    s32 picked;
    LbSmithDeco* deco;
    s32 i;
    s8* text;

    get_lsp_data(0x1915, &base);
    draw_sprite_ary(lb_eqshop_goods_frame, &base);
    draw_sprite_idx(0x1A41, &base);
    lb_page_arrow_draw(0x1914, self->page_0x012, self->pages_0x014, self->page_moved_0x00A, active);
    if (self->count_0x018 != 0) {
        deco = self->decos_0x324;
        for (i = 0; i < self->count_0x018; i++, deco++) {
            enabled = deco->flags_0x0E == 0;
            mode = 2;
            if (i == self->cursor_0x00C) {
                picked = 1;
                get_lsp_data(0x18FE, &window);
                lb_item_window_draw(deco->item_0x08, &window);
            } else {
                picked = 0;
            }
            if (self->sort_0x01C.pick_b_0x80 == 0x12) {
                mode |= 8;
            }
            lsp = get_lsp_data(lb_eqshop_goods_row_lsp[i], NULL);
            pos.x = base.x + lsp->pos.x;
            pos.y = base.y + lsp->pos.y;
            lb_item_cell_draw_layout5(deco->item_0x08, deco->price_0x04, &pos, enabled, picked, active, mode);
        }
    } else {
        if (self->sort_0x01C.pick_a_0x81 == 0) {
            text = (s8*)LbStr(3, 0x6D);
        } else {
            text = (s8*)LbStr(3, 0x6E);
        }
        spr_data_copy(&spr, get_lsp_data(0x1938, NULL));
        pos.x = menu_text_block_center_x_c1((char*)text, base.x + spr.pos.x + (spr.width >> 1), 0x12, &lines);
        pos.y = base.y + spr.pos.y + (spr.height >> 1);
        pos.y -= (s16)(lines * 9);
        font_set_size(0x12, 0x12);
        font_print_ex(pos.x, pos.y, 2, text);
    }
    lb_smith_sort_labels_draw(self, &base);
}

/* 0x80364EE8 (0x2AC): draws the attach page: the stats page with the decoration set in, the box pane or the
 * decorations window, and the prompts. */
void lb_smith_attach_draw(LbSmithWork* self)
{
    LbSmithDeco* deco;
    s32 mode;
    u8 tab;
    s32 lsp;
    u8 frame;
    u32 prompt;
    _mh_ivec2_ pos;

    if ((u32)(self->step_0x001 - 1) <= 2 && self->count_0x018 != 0) {
        equip_record_copy(&self->scroll_0x154.cmp_0x38[0], &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E]);
        equip_record_copy(&self->scroll_0x154.base_0x5C[0], &self->scroll_0x154.cmp_0x38[0]);
        deco = &self->decos_0x324[self->cursor_0x00C];
        if ((deco->flags_0x0E & 0x20) == 0) {
            equip_deco_attach(&self->scroll_0x154.base_0x5C[0], deco->species_id_0x0A);
        }
        if (self->held_0x005 != 0) {
            mode = 0;
        } else {
            mode = self->step_0x001 - 1 == 0;
        }
        lb_equip_page_draw_c1(&self->scroll_0x154, mode);
    }
    switch (self->step_0x001) {
    case 0:
        get_lsp_data(0x1965, &pos);
        lb_pane_draw((struct LbEntryPane*)&self->pane_0x3A4, pos.x, pos.y, NULL);
        break;
    case 1:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            lb_smith_attach_goods_draw(self, 1);
        }
        if (self->count_0x018 == 0 && self->sort_0x01C.pick_a_0x81 == 0) {
            break;
        }
        lsp = 0;
        frame = 0;
        prompt = 1;
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        switch (tab) {
        case 8:
            lsp = 0x199A;
            break;
        case 2:
            lsp = 0x1999;
            break;
        case 4:
            frame = 1;
            prompt = 0;
            lsp = 0x694;
            break;
        }
        if (self->held_0x005 != 0) {
            if (tab != 4 && tab != 10) {
                lb_eqshop_scroll_frame_draw(frame);
            }
            lb_title_frame_draw(&self->tab_list_0x134, self->tab_kinds_0x12C);
            break;
        }
        if (get_option_cfg(7) == 0) {
            if (self->count_0x018 != 0 && lsp != 0) {
                draw_lsp_sprite_variant(lsp, 0);
            }
            if (prompt == 1) {
                draw_lsp_sprite_variant(0x1947, 1);
            }
        } else {
            if (self->count_0x018 != 0 && lsp != 0) {
                draw_lsp_sprite_variant(lsp, 0xD);
            }
            if (prompt == 1) {
                draw_lsp_sprite_variant(0x1947, 0xE);
            }
        }
        break;
    case 3:
        lb_smith_attach_goods_draw(self, 1);
        lb_eqshop_scroll_frame_draw(0);
        lb_eqshop_sort_draw((struct LbShopSortWork*)&self->sort_0x01C, 1);
        break;
    case 2:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            lb_smith_attach_goods_draw(self, 0);
        }
        break;
    }
}

/* 0x80365194 (0x20C): draws the set decorations window of the detach page and the selected decoration's detail. */
void lb_smith_detach_goods_draw(LbSmithWork* self, bool active)
{
    static u16 frame[16] = {0x1A07, 0x1A08, 0x1A09, 0x1A0B, 0x1A0C, 0x1A0A, 0x1A05, 0x1A06,
                            0x19FE, 0x19FF, 0x1A00, 0x1A01, 0x1A02, 0x1A03, 0x1A04, 0xFFFF};
    static u16 frame_b[14] = {0x1A52, 0x1A4E, 0x1A4F, 0x1A50, 0x1A51, 0x1A48, 0x1A49, 0x1A4A,
                              0x1A4B, 0x1A4C, 0x1A4D, 0x1A42, 0xFFFF, 0};
    static u16 rows[3] = {0x1A53, 0x1A54, 0x1A55};
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _mh_ivec2_ window;
    _SPR_DATA_ spr;
    s8 text[16];
    bool enabled;
    s32 picked;
    u32 rare;
    ItemDataRecord* data;
    LbSmithDeco* deco;
    u16* row_lsp;
    s32 i;

    get_lsp_data(0x19FD, &base);
    draw_sprite_anim_ary(frame, 1, &base);
    get_lsp_data(0x1A40, &base);
    draw_sprite_ary(frame_b, &base);
    i = 0;
    deco = self->decos_0x324;
    row_lsp = rows;
    for (; i < self->count_0x018; i++) {
        enabled = deco->flags_0x0E == 0;
        if (i == self->cursor_0x00C) {
            picked = 1;
            draw_font_idx(0x1A47, (s8*)ItemName(deco->item_0x08), 1, &base);
            spr_data_copy(&spr, get_lsp_data(0x1A44, NULL));
            spr.color = GetMenuIconColor(enabled, 1, active, 0);
            draw_itemicon_item_id(spr, deco->item_0x08, &base);
            data = GetItemData(deco->item_0x08);
            rare = get_rare_color(data->level_0x01);
            spr_data_copy(&spr, get_lsp_data(0x1A43, NULL));
            spr.color = rare;
            draw_sprite(spr, &base);
            spr_data_copy(&spr, get_lsp_data(0x1A45, NULL));
            spr.color = rare;
            sprintf((char*)text, "%d", data->level_0x01 + 1);
            draw_font(spr, text, 0, &base);
            sprite_frame_apply(&spr, 0x19FC, 1, &window);
            lb_item_window_draw(deco->item_0x08, &window);
        } else {
            picked = 0;
        }
        get_lsp_data(*row_lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_item_cell_draw_layout1(deco->item_0x08, deco->price_0x04, &pos, enabled, picked, active, 2);
        deco++;
        row_lsp++;
    }
}

/* 0x803653A0 (0x230): draws the detach page: the stats page with the decoration taken out, the box pane or the
 * decorations window, the prompts and the stored item. */
void lb_smith_detach_draw(LbSmithWork* self)
{
    LbSmithDeco* deco;
    s32 mode;
    u8 tab;
    s32 lsp;
    _mh_ivec2_ pos;

    if ((u32)(self->step_0x001 - 1) <= 1 && self->count_0x018 != 0) {
        equip_record_copy(&self->scroll_0x154.cmp_0x38[0], &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E]);
        equip_record_copy(&self->scroll_0x154.base_0x5C[0], &self->scroll_0x154.cmp_0x38[0]);
        if (equip_deco_count(&self->scroll_0x154.base_0x5C[0]) != 0) {
            deco = &self->decos_0x324[self->cursor_0x00C];
            if ((deco->flags_0x0E & 0x20) == 0) {
                equip_deco_detach(&self->scroll_0x154.base_0x5C[0], deco->slot_0x0C);
            }
        }
        if (self->held_0x005 != 0) {
            mode = 0;
        } else {
            mode = self->step_0x001 - 1 == 0;
        }
        lb_equip_page_draw_c1(&self->scroll_0x154, mode);
    }
    switch (self->step_0x001) {
    case 0:
        get_lsp_data(0x1965, &pos);
        lb_pane_draw((struct LbEntryPane*)&self->pane_0x3A4, pos.x, pos.y, NULL);
        break;
    case 1:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            lb_smith_detach_goods_draw(self, true);
        }
        if (self->held_0x005 != 0) {
            lb_eqshop_scroll_frame_draw(1);
            lb_title_frame_draw(&self->tab_list_0x134, self->tab_kinds_0x12C);
            break;
        }
        lsp = 0;
        switch (self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00]) {
        case 8:
            lsp = 0x199A;
            break;
        case 2:
            lsp = 0x1999;
            break;
        case 4:
            lsp = 0x694;
            break;
        }
        if (lsp != 0) {
            if (get_option_cfg(7) == 0) {
                draw_lsp_sprite_variant(lsp, 0);
            } else {
                draw_lsp_sprite_variant(lsp, 0xD);
            }
        }
        break;
    case 2:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            lb_smith_detach_goods_draw(self, false);
        }
        break;
    case 3:
        get_lsp_data(0x1965, &pos);
        lb_page_walk_head(&pos, self->stored_0x1FE);
        break;
    }
}

/* 0x803655D0 (0x13C): draws the weapon upgrade paths. */
void lb_smith_kakou_rows_draw(LbSmithWork* self, s32 active)
{
    static u16 rows[4] = {0x1A0F, 0x1A10, 0x1A11, 0x1A12};
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _EQUIP box;
    s32 i;
    s32 picked;

    get_lsp_data(0x19FD, &base);
    lb_anim_mark_draw(0, 0, &base);
    draw_sprite_idx(0x1A0E, &base);
    equip_record_copy(&box, &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E]);
    for (i = 0; i < 4; i++) {
        if (i == self->kakou_cursor_0x128) {
            picked = 1;
            lb_equip_cell_draw_c1(0x19FC, &self->kakou_rows_0x20C[i], 1);
        } else {
            picked = 0;
        }
        if (self->kakou_rows_0x20C[i].item_id != 0) {
            get_lsp_data(rows[i], &pos);
            pos.x += base.x;
            pos.y += base.y;
            lb_equip_cell_draw_layout5(&self->kakou_rows_0x20C[i], self->kakou_price_0x23C[i], &pos,
                                       self->kakou_flags_0x24C[i] == 0, picked, active, 2, 1);
        }
    }
}

/* 0x8036570C (0x2DC): draws the gun-part or armour upgrade window: the piece, its level, the price and the stage. */
void lb_smith_kakou_info_draw(LbSmithWork* self, bool active, u8 gun)
{
    static u16 frame[6] = {0x1A16, 0x1A20, 0x1A21, 0x1A22, 0x1A23, 0xFFFF};
    static u16 cursor[6] = {0x1A30, 0x1A2D, 0x1A2E, 0x1A2F, 0xFFFF, 0};
    static u16 level_frame[4] = {0x1A24, 0x1A25, 0x1A26, 0xFFFF};
    static u16 stage_a_frame[4] = {0x1A27, 0x1A28, 0x1A29, 0xFFFF};
    static u16 stage_b_frame[4] = {0x1A2A, 0x1A2B, 0x1A2C, 0xFFFF};
    _mh_ivec2_ base;
    _EQUIP box;
    s8 text[32];
    _SPR_DATA_ spr;
    s32 color;

    get_lsp_data(0x19FD, &base);
    lb_anim_mark_draw(0, 0, &base);
    if (gun != 0) {
        draw_sprite_idx(0x1A15, &base);
    } else {
        draw_sprite_idx(0x1A14, &base);
    }
    equip_record_copy(&box, &SMITH_USER->box_0x0E00[self->pane_0x3A4.box_0x01E]);
    lb_equip_cell_draw_c1(0x19FC, self->kakou_rows_0x20C, 1);
    color = GetMenuFontColor(self->kakou_flags_0x24C[0] == 0, 1, active, 0);
    get_lsp_data(0x1A13, &base);
    draw_sprite_ary(frame, &base);
    if (active == 1) {
        put_menu_cursor(cursor, 0, &base);
    }
    spr_data_copy(&spr, get_lsp_data(0x1A17, NULL));
    spr.color = color;
    draw_font(spr, (s8*)LbStr(0, 0xA9), 4, &base);
    sprintf((char*)text, "%d", self->kakou_rows_0x20C[0].deco_count + 1);
    if (self->kakou_rows_0x20C[0].deco_count + 1 < 10) {
        spr_data_copy(&spr, get_lsp_data(0x1A19, NULL));
    } else {
        spr_data_copy(&spr, get_lsp_data(0x1A1B, NULL));
    }
    spr.color = color;
    draw_font(spr, text, 4, &base);
    sprintf((char*)text, "%dz", self->kakou_price_0x23C[0]);
    spr_data_copy(&spr, get_lsp_data(0x1A1C, NULL));
    spr.color = color;
    draw_font(spr, text, 6, &base);
    if (gun != 0) {
        draw_sprite_ary(level_frame, &base);
        draw_font_idx(0x1A1D, (s8*)LbStr(0, 0xAA), 1, &base);
        return;
    }
    draw_sprite_ary(level_frame, &base);
    draw_font_idx(0x1A1D, (s8*)LbStr(0, 0xAB), 1, &base);
    if (equip_level_stage_get(self->kakou_rows_0x20C, self->kakou_rows_0x20C[0].deco_count) == 5) {
        draw_sprite_ary(stage_a_frame, &base);
        draw_font_idx(0x1A1E, (s8*)LbStr(0, 0xAC), 1, &base);
        draw_sprite_ary(stage_b_frame, &base);
        draw_font_idx(0x1A1F, (s8*)LbStr(0, 0xAD), 1, &base);
    }
}

/* 0x803659E8 (0x29C): draws the upgrade page: the stats page, the box pane or the upgrade window, the prompts and
 * the set pane. */
void lb_smith_kakou_draw(LbSmithWork* self)
{
    s32 mode;
    u8 tab;
    s32 lsp;
    _mh_ivec2_ pos;

    if (self->step_0x001 != 0 && self->step_0x001 != 5 && self->kakou_count_0x12A != 0) {
        lb_smith_kakou_preview_set(self);
        if (self->held_0x005 != 0) {
            mode = 0;
        } else {
            mode = self->step_0x001 - 1 == 0;
        }
        lb_equip_page_draw_c1(&self->scroll_0x154, mode);
    }
    switch (self->step_0x001) {
    case 0:
        get_lsp_data(0x1965, &pos);
        lb_pane_draw((struct LbEntryPane*)&self->pane_0x3A4, pos.x, pos.y, NULL);
        break;
    case 1:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            if (self->top_0x0A0.cursor_0x00 == 0) {
                if (lb_smith_gun_part_ck(self->kind_0x007) == 1) {
                    lb_smith_kakou_info_draw(self, true, 1);
                } else {
                    lb_smith_kakou_rows_draw(self, 1);
                }
            } else {
                lb_smith_kakou_info_draw(self, true, 0);
            }
            lsp = 0x1999;
        } else {
            lsp = 0x694;
        }
        if (self->held_0x005 != 0) {
            lb_eqshop_scroll_frame_draw(1);
            lb_title_frame_draw(&self->tab_list_0x134, self->tab_kinds_0x12C);
            break;
        }
        if (get_option_cfg(7) == 0) {
            draw_lsp_sprite_variant(lsp, 0);
        } else {
            draw_lsp_sprite_variant(lsp, 0xD);
        }
        break;
    case 2:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            if (self->top_0x0A0.cursor_0x00 == 0) {
                if (lb_smith_gun_part_ck(self->kind_0x007) == 1) {
                    lb_smith_kakou_info_draw(self, false, 1);
                } else {
                    lb_smith_kakou_rows_draw(self, 0);
                }
            } else {
                lb_smith_kakou_info_draw(self, false, 0);
            }
        }
        break;
    case 3:
        tab = self->tab_kinds_0x12C[self->tab_list_0x134.cursor_0x00];
        if (tab != 4 && tab != 10) {
            if (self->top_0x0A0.cursor_0x00 == 0) {
                if (lb_smith_gun_part_ck(self->kind_0x007) == 1) {
                    lb_smith_kakou_info_draw(self, false, 1);
                } else {
                    lb_smith_kakou_rows_draw(self, 0);
                }
            } else {
                lb_smith_kakou_info_draw(self, false, 0);
            }
        }
        break;
    case 5:
        lb_equip_set_pane_draw((struct LbEquipSetPane*)&self->set_0x80C, 0x1A32, (struct LbEntryPane*)&self->set_box_0x5D8,
                               0x1965, 0);
        break;
    }
}

} /* extern "C" */
