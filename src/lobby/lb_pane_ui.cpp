/*
 * lobby/lb_pane_ui.cpp - the lobby's item and equipment box menu: the digit-entry and item-list panes, the box combine
 *   step, the equipment entry/set/sort panes, the saved equipment and gunner sets, the model, colour and dye panes, the
 *   box menu's frame step and draw pass, and the lobby scene setup.
 * RANGE. .text 0x801EC9F8-0x801FBF78 (129 functions); .data 0x805B85D8-0x805B8D44, .sdata 0x80791C58-0x80791D50,
 *   .sdata2 0x80799870-0x80799880, extab, extabindex.  Three bands whose `.sdata2`/`.data` pools run on without a
 *   break, so one TU: the panes 0x801EC9F8-0x801F3294, the page/panel group 0x801F3294-0x801F9CD4, the colour screen
 *   0x801F9CD4-0x801FBF78.  The panes and the page band keep their declarations in a namespace each (`s_801EC9F8`,
 *   `s_801F3294`): they view `lobby_world_block` (`LbItemDb` / `LbWorldBlock`), the box menu work (`LbListPane`,
 *   `LbEquipBoxWork`, `LbPageOwner`, `LbBoxMenu`) and some callees (`lb_cmd_pressed_ck`, `sprite_frame_apply`)
 *   differently; the colour band's `lb_chg` scope spells `lobby_world_block` as the pointer it is.  Uniting the views
 *   is open work.  `lb_box_menu_draw` and `lb_dye_pane_draw` sit in the colour band's address range but read the page
 *   band's types, so they are defined in a reopened `s_801F3294`.
 * FLAGS. `cflags_lobby`; `#pragma peephole off` throughout and `#pragma dont_inline on` over the panes band
 *   (0x801EC9F8-0x801F3294) only (retail keeps `fn_801ED3E8`/`fn_801ED464` out of line), lifted around the two saved-set
 *   copies so their record copies inline; measured in docs/lobby.md.
 * NAMES. `lb_pane_ui` is a GUESS from what the three bands draw; no `__FILE__` string or dump name covers the range.
 *   GUESS (from each body and its callers): every `lb_*` row of the range (`lb_box_*`, `lb_entry_pane_*`, `lb_equip_*`,
 *   `lb_my_set_*`, `lb_gun_set_*`, `lb_color_*`, `lb_dye_*`, `lb_model_pane_*`, `lb_set_pane_draw`, `lb_count_box_draw`,
 *   `lb_cursor_box_step`, `lb_progress_rewards_check`, `lobby_scene_setup_step`, `lb_digit_pane_value`,
 *   `lb_list_pane_init`, `lb_list_pane_pos_set`, `lb_page_walk_head`, `lb_pane_draw`) and every named `.data`/`.sdata`
 *   table.  Foreign callees named through the registry (lane lobby-pane-ui) are GUESS names too.
 * RESIDUALS. All 129 rows are written; 40 short of 100, by cause:
 *  - shared tails: retail's step machines branch into a sibling case's tail (`fn_801F3294`, `lb_color_pane_step`'s
 *    "record done" tails); ours repeats the tail (MWCC has no tail-merge pass, idea 213), and `lb_color_pane_step`'s
 *    `do { } while (0)` exits return `result` where retail branches to one `mr r3,r29`;
 *  - `lobby_enter_seq_step`/`lobby_scene_setup_step`: retail keeps one "system file done" block (`field_0x865 = 0;
 *    state = 3`) inside case 2, entered from case 0, case 1 and backwards from `case 0x64`, and one
 *    `game_system_file_create_start` test inside case 1 that case 0 enters; ours has the done block once after the
 *    switch (idea 183) and the create test twice (a third exit kind needs a flag).  Retail's flow graph is the
 *    source's own (idea 213: no pass merges the copies); a scratch `goto` spelling measures 98.92 / 744 B;
 *  - an `s16` index retail re-extends at every use where ours extends once at the definition (`lb_pane_draw`'s and
 *    `lb_box_grid_draw`'s grid walks, `fn_801EEDC4`'s slot index), and register colouring / scheduling:
 *    `lb_box_grid_draw` (keeps `reason` on the stack), `lb_dye_inner_draw`, `lb_color_pane_draw`, `lb_dye_pane_draw`,
 *    `lb_box_lists_draw`, `lb_box_menu_draw`, `lb_entry_pane_menu_step` (the sort loop), `lb_entry_pane_sort_apply`,
 *    `lb_entry_pane_list_reset` (the `-(owner == NULL)` delta), `lb_model_pane_draw` (the row text id from the
 *    unmasked counter), `lb_my_set_detail_draw`/`lb_gun_set_detail_draw` (the page row count re-masked at the use),
 *    `lb_box_top_draw`, `lb_my_set_list_draw`, `lb_equip_set_pane_step`, `lb_equip_set_pane_init` (parameter homes),
 *    `lb_equip_box_wear_step`, `lb_entry_pane_scroll_refresh` (the `kind == 10` test is `cmplwi`), `fn_801EE1E4`,
 *    `fn_801EDC4C`, `fn_801ED184`, `fn_801ED200`, `fn_801ED464`, `fn_801ED688`, `fn_801EDB34`;
 *  - `lb_equip_icon_cell_draw`: the `draw_frame_rect_z` arguments load after the float; `fn_801EDA34`: retail's
 *    `bge`/`b` pair around the minimum; `fn_801EDA9C`: the box capacity's `clrlwi` scheduled late; `fn_801ECF74`,
 *    `fn_801F5534` (`(u16)count <= 1` needs a bare `cmplwi r5,1`), `fn_801F86FC` (retail keeps the `flags` pointer in
 *    r30), `fn_801FB364` (frame 0x30 against 0x20 with the same local offsets), `lb_count_box_draw` (one `mr` source).
 *   Data: the `.sdata` tables the code reads are defined here in address order (0x80791C58-0x80791C68 in the panes band,
 *   0x80791C70-0x80791D40 in the page band); `.sdata2` is the compiler's pool (0.0f and the signed int-to-float
 *   constant, 16 bytes like the target; the map names them `lb_icon_frame_z` and `lbl_80799878`); the `.data` tables
 *   (0x805B85D8-0x805B8BC0, 49 tables, no relocations) are still `extern` (declared in `lobby/lb_pane_ui.h`, the panes
 *   band and `unsplit/lobby.h`, several as `const`), the five jump tables are the compiler's, and `lbl_805B8CE0` is the
 *   "mot_aicom.brres" literal of `fn_801FB364`; the string literals "1"/"2"/"3"/"" at 0x80791D40-0x80791D4C come from
 *   the compiler.
 *   flipcheck: `.data` claimed, not emitted; extab/extabindex short of the claim.
 * SHAPES. The step machines are `switch`es laid out in retail's case order (MWCC places case bodies in source order);
 *   an `s16` local for `lb_digit_pane_value`'s result before indexing `lobby_world_block` keeps retail's load order;
 *   one-case byte tests are `switch`es with the other path as `default:`; `(u32)(x - 1) > 4` keeps retail's range test
 *   where the source branches on it first; in the page/panel band `__declspec(noinline)` on `fn_801F353C`,
 *   `fn_801F3588`, `fn_801F6034`; `while (*table != -1)` (retail tests below the body); `fn_801F37C8`, `fn_801F3998`
 *   and `fn_801F4444` with the labels in the order -1, -3, -2, `fn_801F3EDC`, and `fn_801F3DEC` with `default` first;
 *   a `u32`/`u16` parameter narrowed at the use (`fn_801F8ABC`, `fn_801F60D4`) but a `u8` one for `fn_801F3DEC`;
 *   the never-called inline `lb_set_parts_copy` ahead of `lb_my_set_copy` makes the `LbSetOpts`/`LbSetName`
 *   assignments inline (without it the first copy calls an emitted `__as__`); MWCC colours registers by declaration
 *   order, so several functions' locals are in the order that reproduces retail's allocation; the font colour and
 *   arrow flags handed to `GetMenuFontColor`/`LbPutAnaPageArrow` are `bool` locals; a `switch` on a callee's result
 *   (`game_save_wait`, `fn_801EDA9C`/`fn_801EDB34`, `menu_cursor_page_move`) where retail range-tests it; values a
 *   call consumes are computed before the call when retail does (`lb_pane_draw`'s and `lb_box_grid_draw`'s uv row,
 *   `lb_my_set_list_draw`'s mark mode, `lb_equip_set_pane_step`'s row count); a set's three ids are walked in a loop
 *   (`lb_equip_record_swap`); the sort masks' kind bits are set one statement each; `lb_color_pane_step` wraps its
 *   switch in `do { } while (0)` (idea 183: `break` reaches the "left with the old colour" tail, `continue` the
 *   "new colour" one), and so do `lobby_enter_seq_step`/`lobby_scene_setup_step` (the "system file done" block after
 *   the switch, `done = 1` before the error-code call); the page band calls `lb_choice_draw`/`lb_help_num_put` through `_c1` views on the owner's
 *   signatures (`unsplit/lobby.h` declares other ones) and sees `lb_entry_pane_scroll_kind_get` returning a word (its
 *   definition returns `u8`); `Screen_w`'s size is converted signed (`(f32)(s16)`) where retail loads it with `lha`.
 */
#include "types.h"
#include "id_value.h"
#include "menu/menu_message.h"
#include "nw4r/math.h"
#include "sound/fn_800D7F54.h" /* sysSE_req (rule 2) */

#include "Runtime.PPCEABI.H/memset.h"
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "fn_80047398/userdata_gunner_ck.h" /* userdata_gunner_ck (rule 2) */
#include "lobby/lobby_w.h" /* `lobby_w`, owned by lobby/lb_menu_pos_tbl.cpp (rule 2) */
#include "menu/fn_8031A6C0.h" /* MenuSelCursor */
#include "menu/menu_sel_cursor_first_set.h"
#include "menu/menu_sel_cursor_blend_tick.h"
#include "Pl/plw.h" /* _PLW, _EQUIP */
#include "Pl/fn_8027D684.h" /* Gunner_opt_ok_ck, EnableChangeColor, the equipment colour getters */
#include "Pl/equip_color_set.h"
#include "userdata_item/equip_worn_color_set.h"
#include "color_rgba_copy.h"
#include "camera/camera_lobby_menu_enter.h"
#include "lobby/lb_box_menu_close.h"
#include "my_set_any_ck.h"
#include "gun_set_any_ck.h"
struct LbChgColorWork;
#include "Pl/pl_item_pouch_ok_ck.h"
#include "font/flfnt.h"
#include "sound/set_zmode__FbUcb.h"
#include "hud/draw_lsp_sprite_variant.h"
#include "hud/put_button_guide_c_idx.h"
#include "lobby/lb_frame_draw_at.h"
#include "hud/put_button_guide_b_idx.h"
#include "hud/draw_frame_rect_z.h"
#include "hud/draw_number_anim.h"
#include "hud/draw_equipicon.h"
#include "menu/menu_frame_entries_build.h"
#include "menu/menu_item_cell_draw.h"
#include "menu/menu_sel_cursor_result_draw.h"
#include "menu/menu_sel_cursor_confirm_draw.h"
#include "menu/get_menu_lsp_tbl.h"
#include "userdata_item/item_slots_count_sum.h"
#include "menu/put_menu_cursor.h"
#include "MSL_C/string.h"
#include "mh3_pad/Screen_w.h"
#include "RVLGX/GXMisc.h"
#include "menu/Put_equip_dtl_syo_item1.h"
#include "npc_model_res_ids.h"
#include "menu/Put_status_equip_status_block.h"
#include "menu/equip_list_draw.h"
#include "menu/column_skills_page_draw.h"
#include "menu/gun_stats_columns_draw.h"
#include "lobby/LbPutAnaPageArrow.h"
#include "lobby/lb_equip_cell_draw_layout3.h"
#include "lobby/lb_ana_page_arrow_put.h"
#include "menu/put_equip_row_variant.h"
#include "menu/put_equip_row_variant_ex.h"
#include "menu/put_equip_row_variant2.h"
#include "menu/put_equip_panel_row.h"
#include "menu/put_lsp_sprite_offset.h"
#include "hud/draw_weaponicon_idx.h"
#include "hud/get_rare_color.h"
#include "Pl/Get_equip_rare.h"
#include "lobby/lb_equip_page_draw.h"
#include "lobby/lb_anim_mark_draw.h"
#include "uv_pair_copy.h"
#include "lobby/lb_item_info_draw.h"
#include "lobby/lb_count_window_draw.h"
#include "hud/color_mult.h"
#include "lobby/lb_title_frame_draw.h"
#include "lobby/lb_equip_cell_draw.h"
#include "lobby/lb_list_draw_ex.h"
#include "lobby/lb_kakou_icon_build.h"
#include "lobby/lb_help_text_put.h"
#include "lobby/lb_sort_menu_frame.h"
#include "Pl/equip_deco_count.h"
#include "hud/spr_anim_frame_set.h"
#include "my_set_record_used_ck.h"
#include "gun_set_record_used_ck.h"
#include "userdata_progress_flag_ck.h"
#include "userdata_flag_ck.h"
#include "lobby/lb_list.h" /* LbList */
#include "lobby/lb_scroll_kind_set.h"
#include "lobby/lb_cmd_held_ck.h"
#include "lobby/equip_sell_price_get.h"
#include "equip_wearable_ck.h"
#include "my_set_equip_id_swap.h"
#include "my_set_gun_id_swap.h"
#include "equip_record_wear.h"
#include "userdata_item/equip_record_discard.h"
#include "userdata_item/equip_worn_records_refresh.h"
#include "my_set_store.h"
#include "my_set_clear.h"
#include "my_set_used_ck.h"
#include "gun_set_store.h"
#include "gun_set_clear.h"
#include "gun_set_used_ck.h"

struct LbPage;
struct _PLW;

#pragma peephole off
#pragma dont_inline on

/* ------------------------------------------------------------------------------------------------
 * The two work records this range drives.  Both are this range's own objects (no other registered unit
 * reads them), so their layouts are stated here rather than in a shared header.
 */

/* The digit-entry pane `fn_801ECD50` initialises and `fn_801ED048`/`fn_801ECDD4` step: three decimal
 * digits (hundreds/tens/ones), two blink timers and the pick/cursor pair.  Layout from the object's
 * own stores (`fn_801ECD50`), its field widths (`stb`/`sth`/`stw` at 0x1C/0x0A/0x04) and the accessors
 * `fn_801ED160`/`fn_801ECEF0`. */
typedef struct LbDigitPane {
    /* +0x00 */ u8 phase_0x00;          /* the step switch `fn_801EDC4C`/`fn_801EEDC4` dispatch on */
    /* +0x01 */ u8 sub_0x01;
    /* +0x02 */ u8 digit_hundreds_0x02;
    /* +0x03 */ u8 digit_hundreds_max_0x03;
    /* +0x04 */ s32 stepper_0x04;       /* the 4-byte stepper state `toggle_word_step` walks */
    /* +0x08 */ u16 flags_0x08;         /* bit 0x100 / 0x200 arm the two blink timers */
    /* +0x0A */ s16 digit_ones_0x0A;
    /* +0x0C */ s16 digit_tens_0x0C;
    /* +0x0E */ s16 anim_0x0E;          /* the value `fn_801EC804` animates */
    /* +0x10 */ s16 timer_0x10;
    /* +0x12 */ s16 blink_a_0x12;       /* -1 when idle, ticks 0..6 */
    /* +0x14 */ s16 blink_b_0x14;
    /* +0x16 */ s16 pick_max_0x16;      /* how many of the item can be taken */
    /* +0x18 */ s16 pick_count_0x18;    /* how many are picked, 1..pick_max_0x16 */
    /* +0x1A */ s16 cursor_0x1A;
    /* +0x1C */ u8 active_0x1C;         /* the "a pane is open" latch `fn_801ED048` waits on */
    /* +0x1D */ u8 done_0x1D;
} LbDigitPane; /* size: 0x1E (the extent this range reads) */

/* The 0x194-byte item list `fn_801ECF74` builds at `LbListPane::items_0x3B8`: 0x65 id/value pairs, the last
 * of which the rebuild loop clears before every re-insert. */
typedef struct LbItemList {
    /* +0x000 */ IdValue entries_0x000[0x65];
} LbItemList; /* size: 0x194 */

/* The item-list pane `fn_801ED688`/`fn_801EE1E4` walk and `fn_801ECF74` fills: the cursor pair, the
 * two stepper states the pad moves, the selected item id, and the 0x194-byte entry list at +0x3B8. */
typedef struct LbListPane {
    /* +0x000 */ u8 unused_0x000;
    /* +0x001 */ u8 phase_0x001;        /* the step switch `fn_801ED688`/`fn_801EE1E4` dispatch on */
    /* +0x002 */ u8 unused_0x002[3];
    /* +0x005 */ u8 side_0x005;         /* which of the two panes (0/1) the item comes from */
    /* +0x006 */ u8 unused_0x006[0xA];
    /* +0x010 */ void* data_0x010;      /* the equipment work block `fn_801ED284` backs up */
    /* +0x014 */ s16 row_0x014;         /* the cursor row */
    /* +0x016 */ u8 row_sub_0x016;      /* the 8-step row pair `menu_cursor_step_fixed_tail` walks */
    /* +0x017 */ u8 row_sub_max_0x017;
    /* +0x018 */ s16 column_0x018;      /* the cursor column */
    /* +0x01A */ u8 column_sub_0x01A;   /* the 8-step column pair `menu_cursor_step` walks */
    /* +0x01B */ u8 column_sub_max_0x01B;
    /* +0x01C */ u16 move_0x01C;        /* the pad-driven 4-step state at +0x1C (menu_cursor_step_fixed_tail's base) */
    /* +0x01E */ s16 move_max_0x01E;
    /* +0x020 */ s16 entry_0x020;       /* the 4-step state at +0x20 (menu_cursor_page_move's cursor base) */
    /* +0x022 */ s16 entry_max_0x022;
    /* +0x024 */ u16 item_id_0x024;
    /* +0x026 */ u8 active_0x026;       /* the "confirm is open" latch `fn_801ED56C` waits on */
    /* +0x027 */ u8 unused_0x027[5];
    /* +0x02C */ s32 stepper_0x02C;     /* the 4-byte stepper state `toggle_word_step` walks */
    /* +0x030 */ u8 unused_0x030[0x388];
    /* +0x3B8 */ LbItemList items_0x3B8; /* the 0x65-entry list `fn_801ECF74` fills */
} LbListPane; /* size: 0x54C (the extent this range reads) */

/* The equipment work block `LbListPane::data_0x010` points at: the working eight-item id/kind run and
 * the backup run `fn_801ED284` copies it into. */
typedef struct LbConfigWork {
    /* +0x0000 */ u8 unused_0x0000[0x5F2];
    /* +0x05F2 */ u16 ids_0x5F2[8];
    /* +0x0602 */ u8 kinds_0x602[8];
    /* +0x060A */ u8 unused_0x060A[0x10];
    /* +0x061A */ u16 backup_ids_0x61A[8];
    /* +0x062A */ u8 backup_kinds_0x62A[8];
} LbConfigWork; /* size: 0x632+ (the extent this range reads) */

/* This band's view of the 0x80-byte scroll-list work `lobby/fn_80212810.cpp` drives (`lb_scroll_init` clears it,
 * `lb_scroll_tables_set` fills the four record tables): the kind byte, the moved bits the release step writes, the
 * scroll row and its count, and the flag byte at +0x10. */
typedef struct LbPaneScroll {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01[5];
    /* +0x06 */ u16 moved_0x06;        /* `menu_cursor_step_fixed_tail`'s moved bits */
    /* +0x08 */ s16 row_0x08;          /* the scroll row */
    /* +0x0A */ s16 rows_0x0A;         /* the row count */
    /* +0x0C */ u8 unused_0x0C[4];
    /* +0x10 */ u8 flags_0x10;
    /* +0x11 */ u8 unused_0x11[0x17];
    /* +0x28 */ u16 kakou_0x28;        /* the crafting state `lb_kakou_icon_build` returned */
    /* +0x2A */ u8 unused_0x2A[2];
    /* +0x2C */ u8* kakou_icon_0x2C;   /* the crafting icon record it built */
    /* +0x30 */ u8 unused_0x30[0x50];
} LbPaneScroll; /* size: 0x80 */

/* The set pane `fn_801EF1D0`/`lb_equip_set_pane_init`/`lb_equip_set_pane_step` drive: the slot the cursor is on, the set's
 * three record ids, the confirm stepper and the scroll list of the set. */
typedef struct LbEquipPane {
    /* +0x00 */ u8 phase_0x00;
    /* +0x01 */ u8 side_0x01;
    /* +0x02 */ s16 index_0x02;         /* the id slot the cursor is on */
    /* +0x04 */ u16 ids_0x04[3];        /* the three equipped record ids (0xFFFF = empty) */
    /* +0x0A */ u8 unused_0x0A[2];
    /* +0x0C */ s32 stepper_0x0C;       /* the 4-byte stepper state `toggle_word_step` walks */
    /* +0x10 */ LbPaneScroll scroll_0x10;
    /* +0x90 */ struct LbPage* page_0x90; /* the page record the scroll list is handed */
} LbEquipPane; /* size: 0x94 */

/* The lobby menu work object `lobby_w.menu_0xAC` points at, with the six fields `fn_801ECB40` drives; `LbMenuWork` in
 * `unsplit/lobby.h` is the full record and spells +0x02/+0x06/+0x0A as padding (two views to merge). */
typedef struct LbMenuPanel {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 mode_0x01;
    /* +0x02 */ u8 list_kind_0x02;     /* 0 while the panel is still on its first page */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 unused_0x05;
    /* +0x06 */ s16 row_0x06;          /* the selection row, an index into `lbl_805B85D8` */
    /* +0x08 */ s16 cursor_0x08;
    /* +0x0A */ s16 page_0x0A;         /* 0/1 page the up/down keys flip */
    /* +0x0C */ s16 row_max_0x0C;
    /* +0x0E */ s16 stage_0x0E;        /* the stage id the row selects */
} LbMenuPanel; /* size: 0x10 (the extent this range reads) */

/* One 0xC-byte equipment record (the `equip_record_copy`/`fn_8004BD58` copy unit and `Gunner_opt_ok_ck`'s
 * argument).  Only the kind byte is read here. */
typedef struct LbEquipRec {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u8 unused_0x04[8];
} LbEquipRec; /* size: 0xC */

/* One saved set's name, copied as one block.  size: 0x17 */
typedef struct LbSetName {
    /* +0x00 */ char text_0x00[0x17];
} LbSetName;

/* A saved set's two weapon option ids, copied as one block.  size: 0x4 */
typedef struct LbSetOpts {
    /* +0x00 */ u16 ids[2];
} LbSetOpts;

/* One saved equipment set (`my_set_store`): the weapon, its two option ids, the six armour ids and the name.
 * size: 0x2A */
typedef struct LbMySet {
    /* +0x00 */ u16 weapon_0x00;
    /* +0x02 */ LbSetOpts opts_0x02;
    /* +0x06 */ u16 armor_0x06[6];
    /* +0x12 */ LbSetName name_0x12;
    /* +0x29 */ u8 unused_0x29;
} LbMySet;

/* One saved gunner set (`gun_set_store`): the bowgun, its two option ids and the name.  size: 0x1E */
typedef struct LbGunSet {
    /* +0x00 */ u16 weapon_0x00;
    /* +0x02 */ LbSetOpts opts_0x02;
    /* +0x06 */ LbSetName name_0x06;
    /* +0x1D */ u8 unused_0x1D;
} LbGunSet;

/* The lobby item database `lobby_world_block` (a 4-byte pointer in `.sbss`) points at: the item slot table
 * at +0x180 (`userdata_equip_item_id_get`/`userdata_pouch_get`/`userdata_equip_item_slots_get` index it), the 0xC-byte equipment records at
 * +0xE00, and the current-equipment kind at +0x8C. */
typedef struct LbItemDb {
    /* +0x0000 */ u8 unused_0x0000[0x18];
    /* +0x0018 */ u8 data_0x18[0x74];  /* the block `score_add_clamped` is handed as a work area */
    /* +0x008C */ u16 kind_0x8C;       /* the equipped item's kind (0xB is a bowgun) */
    /* +0x008E */ s16 sub_0x8E;
    /* +0x0090 */ s16 sub_0x90;
    /* +0x0092 */ u16 worn_a_0x92;     /* the worn armour record ids, in `my_set_store`'s order */
    /* +0x0094 */ u16 worn_b_0x94;
    /* +0x0096 */ u16 worn_c_0x96;
    /* +0x0098 */ u16 worn_d_0x98;
    /* +0x009A */ u16 worn_e_0x9A;
    /* +0x009C */ u16 worn_f_0x9C;
    /* +0x009E */ u8 unused_0x009E[0xE2];
    /* +0x0180 */ IdValue slots_0x180[0x320]; /* the slot table the item calls index */
    /* +0x0E00 */ LbEquipRec equip_0x0E00[0x100]; /* the 0xC-byte equipment records */
    /* +0x1A00 */ u8 unused_0x1A00[0x1980];
    /* +0x3380 */ LbMySet my_sets_0x3380[20];      /* the saved equipment sets */
    /* +0x36C8 */ LbGunSet gun_sets_0x36C8[20];    /* the saved gunner sets */
    /* +0x3920 */ u8 unused_0x3920[0xFF8];
    /* +0x4918 */ u16 gallery_0x4918;              /* set once the gallery reward of unlock condition 5 is earned */
} LbItemDb; /* size: 0x491A+ (only the runs this range indexes are named) */

/* The equipment-entry pane: the digit entry (the same leading fields as `LbDigitPane`) picking an equipment record,
 * the slot kind it is filling, the three ids of the set it shows, the sort menu's step bytes, the six lobby lists
 * (the scroll list and the five sort menus) and the 100 sorted record ids.  Embedded at +0x74 of
 * `LbEquipBoxWork`, whose +0x2A8 follows it directly. */
typedef struct LbEntryPane {
    /* +0x000 */ u8 phase_0x000;
    /* +0x001 */ u8 sub_0x001;
    /* +0x002 */ u8 digit_hundreds_0x002;
    /* +0x003 */ u8 digit_hundreds_max_0x003;
    /* +0x004 */ s32 stepper_0x004;     /* the 4-byte stepper state `toggle_word_step` walks */
    /* +0x008 */ u16 flags_0x008;       /* bit 0x100 / 0x200 arm the two blink timers */
    /* +0x00A */ s16 digit_ones_0x00A;
    /* +0x00C */ s16 digit_tens_0x00C;
    /* +0x00E */ s16 anim_0x00E;
    /* +0x010 */ s16 timer_0x010;
    /* +0x012 */ s16 blink_a_0x012;
    /* +0x014 */ s16 blink_b_0x014;
    /* +0x016 */ s16 slot_kind_0x016;   /* 0 = any record, 1..3 = the set's three slots */
    /* +0x018 */ s16 set_ids_0x018[3];  /* the set's three record ids (-1 = empty) */
    /* +0x01E */ s16 mark_0x01E;      /* the record a move or sell step holds */
    /* +0x020 */ u8 held_0x020;         /* set while the list button is held */
    /* +0x021 */ u8 list_mode_0x021;    /* 1 = the four-row list, 2 = the six-row one */
    /* +0x022 */ u8 unused_0x022;
    /* +0x023 */ u8 sort_step_0x023;    /* the sort menu's depth (0xFF = the confirm step) */
    /* +0x024 */ u8 sort_prev_0x024;
    /* +0x025 */ u8 sort_sub_0x025;
    /* +0x026 */ u8 sort_kind_0x026;    /* the sort the list was last built with */
    /* +0x027 */ u8 unused_0x027;
    /* +0x028 */ struct LbPage* page_0x028; /* the page record the scroll list and the wear step use */
    /* +0x02C */ LbList list_0x02C;
    /* +0x04C */ LbPaneScroll scroll_0x04C;
    /* +0x0CC */ LbList sort_lists_0x0CC[5];
    /* +0x16C */ s16 sorted_ids_0x16C[0x64];
} LbEntryPane; /* size: 0x234 */

/* The equipment-box screen work (`lobby_w.menu_0xAC` while the box is open): the done flag, the page record
 * `userdata_pl_look_apply` refreshes, the entry pane and the set pane. */
typedef struct LbEquipBoxWork {
    /* +0x000 */ u8 unused_0x000[4];
    /* +0x004 */ u8 changed_0x004;     /* set once the worn equipment changed */
    /* +0x005 */ u8 unused_0x005[0xB];
    /* +0x010 */ struct LbPage* page_0x010;
    /* +0x014 */ u8 unused_0x014[0x60];
    /* +0x074 */ LbEntryPane entry_0x074;
    /* +0x2A8 */ LbEquipPane set_0x2A8;
} LbEquipBoxWork; /* size: 0x33C (approximate: the set pane is the last field this band reaches) */

/* The my-set list pane: the step, the list mode and kind, the set under the cursor and the set count, the set a
 * move holds, the detail row, the two moved-bit words, the blink timer, the confirm stepper and the page record. */
typedef struct LbMySetPane {
    /* +0x00 */ u8 phase_0x00;
    /* +0x01 */ u8 sub_0x01;
    /* +0x02 */ u8 mode_0x02;          /* 0 = equipment sets, otherwise gunner sets */
    /* +0x03 */ u8 kind_0x03;
    /* +0x04 */ u8 index_0x04;         /* the set under the cursor */
    /* +0x05 */ u8 count_0x05;         /* the set count (20) */
    /* +0x06 */ u8 mark_0x06;          /* the set a move holds */
    /* +0x07 */ u8 row_0x07;           /* the detail row */
    /* +0x08 */ u16 moved_0x08;        /* the set cursor's moved bits */
    /* +0x0A */ u16 row_moved_0x0A;    /* the detail row's moved bits */
    /* +0x0C */ s16 blink_0x0C;        /* the move step's blink timer (0..19) */
    /* +0x0E */ u8 unused_0x0E[2];
    /* +0x10 */ s32 stepper_0x10;      /* the confirm stepper `toggle_word_step`/`lb_yes_no_step` walk */
    /* +0x14 */ struct LbPage* page_0x14; /* the page record `userdata_pl_look_apply` refreshes */
} LbMySetPane; /* size: 0x18 */

/* The model pane `lb_model_pane_init` seeds: the step, the mode, the part kind it starts on and the player work whose
 * equipment models it shows. */
typedef struct LbModelPane {
    /* +0x00 */ u8 phase_0x00;
    /* +0x01 */ u8 sub_0x01;
    /* +0x02 */ u8 mode_0x02;          /* 1 = the gunner model */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ s16 cursor_0x04;
    /* +0x06 */ s16 part_0x06;         /* the part kind the pane starts on (4 or 11) */
    /* +0x08 */ s16 value_0x08;
    /* +0x0A */ u8 unused_0x0A[2];
    /* +0x0C */ struct _PLW* plw_0x0C;
} LbModelPane; /* size: 0x10 */
namespace s_801EC9F8 {

/* ------------------------------------------------------------------------------------------------
 * The symbols this range reads, as plain prototypes (their owners' headers do not declare them).  `lobby_w` and
 * `lobby_world_block` carry this range's own view: `unsplit/lobby.h` cannot be included (its `lb_cmd_pressed_ck(s32, s16)`
 * and `menu_cursor_step_fixed_tail` do not match the calls here).
 */
extern "C" {
/* This range's own functions, called by earlier bodies. */
s32 lb_cursor_box_step(s16* x, s16* y, s16 x_min, s16 x_max, s16 y_min, s16 y_max, u16 buttons);
void lb_color_cursor_default(u32 color, s16* x, s16* y);
s32 fn_801ECB40(void);
void fn_801ECD50(LbDigitPane* self);
void fn_801ECDD4(LbDigitPane* self);
void fn_801ECEF0(LbDigitPane* self);
void fn_801ECF74(LbListPane* self);
s32 fn_801ED048(LbDigitPane* self, LbListPane* list);
s16 fn_801ED160(LbDigitPane* self);
s32 fn_801ED184(s32 index);
s32 fn_801ED200(s32 index, u8 side);
void fn_801ED284(LbListPane* self);
void fn_801ED3E8(LbListPane* self);
void fn_801ED464(LbListPane* self);
s16 fn_801ED53C(LbListPane* self);
s16 fn_801ED554(LbListPane* self);
s32 fn_801ED56C(LbListPane* self, u8 arg);
s32 fn_801ED688(LbListPane* self, u8 arg);
s32 fn_801ED9E4(s32 index);
s32 fn_801EDA34(s32 index, u8 side);
u8 fn_801EDA9C(LbListPane* self, u16 index, u16 item_id);
u8 fn_801EDB34(LbListPane* self, u16 index, u16 item_id);
s32 fn_801EDC4C(LbDigitPane* self, LbListPane* list);
s32 fn_801EE1E4(LbListPane* self);
u32 fn_801EE900(u16 item_id);
s32 fn_801EE940(u16 index, u16 item_id);
s32 lb_box_combine_step(LbDigitPane* self, MenuSelCursor* cur, LbListPane* list);
s32 fn_801EEDC4(LbDigitPane* self, LbListPane* list);
s32 fn_801EEFA0(LbDigitPane* self, LbListPane* list);
s32 fn_801EF1D0(LbEquipPane* self);
void lb_equip_set_pane_init(LbEquipPane* self, s16 index, u16 a, u16 b, u16 c, u8 side, struct LbPage* page);
s32 lb_equip_set_pane_step(LbEquipPane* self, LbEntryPane* entry, LbEquipBoxWork* owner);
s16 lb_digit_pane_value(LbEntryPane* self);
void lb_entry_pane_scroll_refresh(LbEntryPane* self);
void lb_entry_pane_kind_set(LbEntryPane* self, s16 slot_kind, u8 keep);
void lb_list_pane_init(LbEntryPane* self, s16 slot_kind, struct LbPage* page);
void lb_list_pane_pos_set(LbEntryPane* self, s16 id_a, s16 id_b, s16 id_c);
u8* lb_entry_pane_kind_tbl_get(LbEntryPane* self);
u8 lb_entry_pane_scroll_kind_get(LbEntryPane* self);
void lb_entry_pane_input(LbEntryPane* self);
void lb_entry_pane_sort_masks_get(LbEntryPane* self, s32* kinds, s32* picked, u8* weapon);
u8 lb_entry_pane_sort_class_get(LbEntryPane* self, s32* kinds, s32* picked, u8* weapon);
void lb_entry_pane_sort_apply(LbEntryPane* self, u8 kind);
void lb_entry_pane_list_reset(LbEntryPane* self, LbEquipBoxWork* owner);
s32 lb_entry_pane_busy_ck(LbEntryPane* self);
s32 lb_entry_pane_menu_step(LbEntryPane* self, LbEquipBoxWork* owner, LbEquipPane* set);
void lb_entry_pane_blink_tick(LbEntryPane* self);
void lb_entry_pane_frame_begin(LbEntryPane* self);
void lb_equip_record_swap(u16 id_a, u16 id_b, LbEquipPane* set);
s32 lb_entry_pane_select_step(LbEntryPane* self, LbEquipBoxWork* owner, LbEquipPane* set);
s32 lb_equip_box_wear_step(LbEquipBoxWork* self);
s32 lb_entry_pane_move_step(LbEntryPane* self, LbEquipBoxWork* owner);
s32 lb_entry_pane_sell_step(LbEntryPane* self, LbEquipBoxWork* owner);
void lb_my_set_pane_init(LbMySetPane* self, u8 mode, u8 kind, struct LbPage* page);
s32 lb_my_set_pane_rows_get(LbMySetPane* self);
void lb_my_set_pane_row_clamp(LbMySetPane* self);
s32 lb_my_set_load_step(LbEquipBoxWork* owner, LbMySetPane* self);
s32 lb_my_set_save_step(LbMySetPane* self);
s32 lb_my_set_move_step(LbMySetPane* self);
void lb_my_set_copy(LbMySet* dst, LbMySet* src);
s32 lb_my_set_delete_step(LbMySetPane* self);
s32 lb_gun_set_load_step(LbEquipBoxWork* owner, LbMySetPane* self);
s32 lb_gun_set_save_step(LbMySetPane* self);
s32 lb_gun_set_move_step(LbMySetPane* self);
void lb_gun_set_copy(LbGunSet* dst, LbGunSet* src);
s32 lb_gun_set_delete_step(LbMySetPane* self);
void lb_model_pane_init(LbModelPane* self, struct _PLW* plw, u8 mode);

/* The entry pane's scroll-kind tables, one byte per list row (`lb_entry_pane_kind_tbl_get`). */
extern u8 lb_entry_scroll_kinds_four[4];
extern u8 lb_entry_scroll_kinds_four_empty[4];
extern u8 lb_entry_scroll_kinds_six[8];
extern u8 lb_entry_scroll_kinds_six_empty[8];
}

/* Foreign data. */
extern "C" { extern LbItemDb* lobby_world_block; }
extern "C" { extern s16 lbl_805B85D8[0x30]; }      /* the row -> stage table `fn_801ECB40` indexes */
extern "C" {
extern u16 lb_sort_blade_classes[8];      /* .data 0x805B8618, the blade sort menu's row -> weapon class table */
extern u16 lb_sort_blade_classes_late[6]; /* .data 0x805B8638, the same once the late weapon classes are unlocked */
}
extern "C" { extern u8 jumptable_805B8B98[0x2C]; }

extern "C" {
/* The lobby UI API.  Signatures are the caller-side ones the call sites imply (the callee is
 * `NonMatching` in every case, so the width a call site uses is what its codegen needs). */
s32 item_pair_copy(IdValue* dst, const IdValue* src);
s32 equip_record_copy(LbEquipRec* dst, const LbEquipRec* src);
u16 userdata_box_capacity(LbItemDb* db);
s16 userdata_equip_box_capacity(LbItemDb* db);
s16 userdata_pouch_size(u8 side);
s32 userdata_item_slot_count(LbItemDb* db);
IdValue* userdata_pouch_get(LbItemDb* db, u8 side);
IdValue* userdata_equip_item_slots_get(LbItemDb* db);
u16 userdata_equip_item_id_get(LbItemDb* db, s32 index);
s16 userdata_equip_item_value_get(LbItemDb* db, s16 index);
s32 userdata_pouch_room_get(LbItemDb* db, u8 side);
s32 userdata_item_total_count(LbItemDb* db, u16 id);
s32 item_slots_room_get(u16 id, void* slots, s32 count);
s32 item_pair_index_find(u16 id, IdValue* base, s32 count);
s32 item_take(u16 id, s16 a, void* p, s32 b, s32 c, s32 d);
void userdata_pouch_item_give(LbItemDb* db, u16 id, s16 a, u8 side, s32 b);
s32 item_pair_clear(void* p);
s32 item_slots_store(void* dst, u16 count, u16 id, s16 v, void* tmp);
s32 item_box_slot_take(IdValue* slots, u16 index, s16 delta);
s32 item_box_store(u16 id, s32 a, u16* out);
s32 item_slots_merge(void* dst, u16 count);
u16 item_slots_free_count(void* slots, u16 count);
void userdata_pl_look_apply(struct LbPage* page, LbItemDb* db);
u32 userdata_equip_box_worn_ck(LbItemDb* db, u16 id);
s32 score_add_clamped(s32 a, void* p);
s32 userdata_progress_flag_ck(s32 a);
s32 userdata_flag_ck(s32 a);
void sysSE_stop(s32 id);
s32 fn_801EC7AC(LbMenuPanel* self);
void fn_801EC804(s16* value);
s32 fn_801EC828(LbMenuPanel* self);
s32 lb_box_cam_input_step(LbMenuPanel* self, u8 flag);
void lb_box_model_turn_step(LbMenuPanel* self);
u32 lb_page_record_empty_ck(struct LbPage* page);
s32 lb_cmd_pressed_ck(u32 mask);
s32 lb_cmd_repeat_ck(u32 mask);
u16 lb_cmd_repeat_get(void);
u16 lobby_cmd_trig_get(void);
u16 lobby_cmd_release_get(void);
s32 lobby_cmd_release_ck(u32 mask);
s32 lb_yes_no_step(void* p);
s32 lb_equip_row_next(u8 a, s16 b, void* c, u16 d, s32 e, s32 f, u8 g);
void lb_scroll_init(struct LbPage* page, LbPaneScroll* scroll, u8 kind, u8 mode);
void lb_scroll_flag_set(LbPaneScroll* scroll, u8 kind);
void lb_scroll_tables_set(LbPaneScroll* scroll, LbEquipRec* a, LbEquipRec* b, LbEquipRec* c, LbEquipRec* d);
void lb_equip_compare_set(LbPaneScroll* scroll, LbEquipRec* rec);
void lb_equip_base_set(LbPaneScroll* scroll, LbEquipRec* rec);
s32 lb_equip_entry_apply(s32 a);
void pl_equip_model_set(struct _PLW* plw, u8* rec);
void lb_list_init(LbList* list, s16 count, u8 kind, u16 str_base, s16 help_str, u16 off_mask, s32 se_ok, u32 flags,
                  u16 blink_mask);
s32 lb_choice_step(void* list);
s32 camera_lobby_manual_set(s32 a);
void menu_sel_cursor_init(MenuSelCursor* cur, s8 kind, MenuSel* owner);
s32 menu_sel_cursor_second_set(MenuSelCursor* cur, u16 slot, u16 item_id);
u32 menu_sel_cursor_status_update(MenuSelCursor* cur);
s32 menu_sel_cursor_confirm_step(MenuSelCursor* cur, s32* flag, u16 a, u16 b, u16 c);
void menu_sel_cursor_combine_start(MenuSelCursor* cur, u8 which);
u32 menu_sel_cursor_combine_step(MenuSelCursor* cur, u16 a, u16 b);
void menu_sel_cursor_apply_start(MenuSelCursor* cur);
s32 menu_sel_cursor_apply_step(MenuSelCursor* cur, u16 a, u16 b);
s32 lb_combo_item_known_ck(u16 item_id);
s32 lb_combo_recipe_find(u16 a, u16 b, s32 c);
s32 eft052_item_value_get(u16 id);
s32 hud_item_msg_push(s32 a, s32 b, s32 c);
u32 lobby_net_busy_ck(void);
}
} /* namespace s_801EC9F8 */


/* C++-linkage callees: their map names are manglings, so the real declaration is what makes the
 * front-end emit them (docs/plan.md 6.5 rule 9). */
u8* GetItemData(u16 id);

s32 chk_pointer(void);
namespace s_801EC9F8 {


/* ------------------------------------------------------------------------------------------------ */

/* Clamps the cursor pair to the given box after applying the D-pad bits of `buttons`; returns whether
 * the pad owned the frame. */
extern "C" s32 lb_cursor_box_step(s16* x, s16* y, s16 x_min, s16 x_max, s16 y_min, s16 y_max, u16 buttons)
{
    s16 vx;
    s16 vy;

    if (lobby_net_busy_ck() == 1U) {
        return 0;
    }
    if ((buttons & 0x3C00) != 0) {
        if ((buttons & 0x800) != 0) {
            *x -= 1;
        }
        if ((buttons & 0x400) != 0) {
            *x += 1;
        }
        if ((buttons & 0x2000) != 0) {
            *y -= 1;
        }
        if ((buttons & 0x1000) != 0) {
            *y += 1;
        }
        vx = *x;
        if (vx < x_min) {
            *x = x_min;
        } else if (vx > x_max) {
            *x = x_max;
        }
        vy = *y;
        if (vy < y_min) {
            *y = y_min;
        } else if (vy > y_max) {
            *y = y_max;
        }
        return 1;
    }
    return 0;
}

/* The cursor's default position (100, 45). */
extern "C" void lb_color_cursor_default(u32 color, s16* x, s16* y)
{
    *x = 100;
    *y = 45;
}

/* The lobby menu panel's frame step: leaves on the cancel button, otherwise advances the two pages. */
extern "C" s32 fn_801ECB40(void)
{
    LbMenuPanel* panel;
    s32 result;
    s16 row;

    panel = lobby_w.menu_0x0AC;
    result = 0;
    if (lb_cmd_pressed_ck(0x80) == 1U && panel->list_kind_0x02 == 0) {
        fn_801EC828(panel);
    } else {
        switch (panel->page_0x0A) {
        case 0:
            if (lb_cmd_pressed_ck(0x10) == 1U) {
                sysSE_req(0);
                panel->page_0x0A += 1;
                row = panel->row_0x06;
                if (row == 4) {
                    panel->stage_0x0E = lbl_805B85D8[row];
                    if (userdata_flag_ck(0x7D4) == 0 && userdata_flag_ck(0x36B1) == 0) {
                        panel->stage_0x0E -= 2;
                    } else if (userdata_progress_flag_ck(0x10) == 0) {
                        panel->stage_0x0E -= 1;
                    }
                } else {
                    panel->stage_0x0E = lbl_805B85D8[row];
                }
                panel->cursor_0x08 = 0;
            } else if (lb_cmd_pressed_ck(0x20) == 1U) {
                result = 2;
                sysSE_req(1);
            } else if (lb_cmd_repeat_ck(3) != 0) {
                panel->row_0x06 = menu_cursor_step(panel->row_0x06, panel->row_max_0x0C, lb_cmd_repeat_get(), 1, 2);
            }
            break;
        case 1:
            if (lb_cmd_pressed_ck(0x10) == 1U) {
                result = 1;
            } else if (lb_cmd_pressed_ck(0x20) == 1U) {
                panel->page_0x0A -= 1;
                sysSE_req(1);
            } else if (lb_cmd_repeat_ck(3) != 0) {
                panel->cursor_0x08 = menu_cursor_step(panel->cursor_0x08, panel->stage_0x0E, lb_cmd_repeat_get(), 1, 2);
            }
            break;
        }
    }
    lb_box_model_turn_step(panel);
    return result;
}

/* Clears the digit-entry pane and seeds its digit limit from the item count. */
extern "C" void fn_801ECD50(LbDigitPane* self)
{
    self->phase_0x00 = 0;
    self->sub_0x01 = 0;
    self->digit_hundreds_0x02 = 0;
    self->digit_hundreds_max_0x03 = menu_page_count(userdata_box_capacity(lobby_world_block), 100);
    self->stepper_0x04 = 0;
    self->anim_0x0E = 0;
    self->timer_0x10 = 0;
    self->blink_a_0x12 = -1;
    self->blink_b_0x14 = -1;
    self->digit_ones_0x0A = 0;
    self->digit_tens_0x0C = 0;
    self->active_0x1C = 0;
    self->done_0x1D = 0;
}

/* The digit-entry pad's frame step: the right stick walks the ones/tens digits, the D-pad the hundreds. */
extern "C" void fn_801ECDD4(LbDigitPane* self)
{
    if (lb_cmd_repeat_ck(0xF) != 0) {
        if (lb_cmd_repeat_ck(0xC) != 0) {
            self->digit_ones_0x0A = menu_cursor_step(self->digit_ones_0x0A, 10, lb_cmd_repeat_get(), 4, 8);
        }
        if (lb_cmd_repeat_ck(3) != 0) {
            self->digit_tens_0x0C = menu_cursor_step(self->digit_tens_0x0C, 10, lb_cmd_repeat_get(), 1, 2);
        }
    } else if (lb_cmd_pressed_ck(0x300) != 0) {
        self->digit_hundreds_0x02 = menu_cursor_step_forward(self->digit_hundreds_0x02, self->digit_hundreds_max_0x03,
                                                             lobby_cmd_trig_get(), 0x100, 0x200, 6, &self->flags_0x08);
        if ((self->flags_0x08 & 0x100) != 0) {
            self->blink_a_0x12 = 0;
        }
        if ((self->flags_0x08 & 0x200) != 0) {
            self->blink_b_0x14 = 0;
        }
    }
}

/* Ticks the two blink timers and the pane's frame counter. */
extern "C" void fn_801ECEF0(LbDigitPane* self)
{
    if (self->blink_a_0x12 >= 0) {
        self->blink_a_0x12++;
        if (self->blink_a_0x12 > 6) {
            self->blink_a_0x12 = -1;
            self->timer_0x10 = 0;
        }
    }
    if (self->blink_b_0x14 >= 0) {
        self->blink_b_0x14++;
        if (self->blink_b_0x14 > 6) {
            self->blink_b_0x14 = -1;
            self->timer_0x10 = 0;
        }
    }
    self->timer_0x10++;
    if (self->timer_0x10 > 0x1E) {
        self->timer_0x10 = 0;
    }
}

/* Rebuilds the pane's 101-entry item list from the database's slot table. */
extern "C" void fn_801ECF74(LbListPane* self)
{
    IdValue* slot;
    LbItemList* list;
    LbEquipRec tmp;
    u16 count;
    u16 i;

    slot = &lobby_world_block->slots_0x180[0];
    list = &self->items_0x3B8;
    memset(list, 0, 404);
    i = 0;
    do {
        item_slots_store(list, 101, slot->id, slot->value, &tmp);
        i++;
        slot++;
    } while (i < 101U);
    item_pairs_compact_sort(list->entries_0x000, 101);
    count = userdata_box_capacity(lobby_world_block);
    for (; i < count; i++, slot++) {
        list->entries_0x000[0x64].id = 0;
        list->entries_0x000[0x64].value = 0;
        item_slots_store(list, 101, slot->id, slot->value, &tmp);
        item_pairs_compact_sort(list->entries_0x000, 101);
    }
}

/* Confirms the digit entry: the button opens the pane's item list, the pad steps it. */
extern "C" s32 fn_801ED048(LbDigitPane* self, LbListPane* list)
{
    s32 result;
    u16 count;

    if (self->active_0x1C == 0) {
        if (lb_cmd_pressed_ck(0x40) == 1U) {
            self->active_0x1C = 1;
            self->done_0x1D = 0;
            self->stepper_0x04 = 0;
            fn_801ECF74(list);
            sysSE_req(0xD);
            return 1;
        }
    } else {
        result = toggle_word_step(&self->stepper_0x04, lobby_cmd_trig_get(), 4, 8, 0xFFFF);
        switch (result) {
        case 1:
            count = userdata_box_capacity(lobby_world_block);
            item_slots_merge(&lobby_world_block->slots_0x180[0], count);
            item_pairs_compact_sort(&lobby_world_block->slots_0x180[0], count);
            self->active_0x1C = 0;
            self->digit_ones_0x0A = 0;
            self->digit_tens_0x0C = 0;
            self->digit_hundreds_0x02 = 0;
            sysSE_req(0);
            break;
        case 2:
            self->active_0x1C = 0;
            break;
        }
        return 1;
    }
    return 0;
}

/* The three decimal digits as a number. */
extern "C" s16 fn_801ED160(LbDigitPane* self)
{
    return (s16)((self->digit_ones_0x0A + self->digit_hundreds_0x02 * 100) + self->digit_tens_0x0C * 10);
}

/* How many of the slot's item the player can afford. */
extern "C" s32 fn_801ED184(s32 index)
{
    LbItemDb* db;
    s32 count;

    if (index >= userdata_item_slot_count(lobby_world_block)) {
        return 0;
    }
    db = lobby_world_block;
    count = userdata_box_capacity(db);
    return item_slots_room_get(userdata_equip_item_id_get(db, index), &db->slots_0x180[0], (u16)count);
}

/* Whether the slot holds an item at all (and which side it came from). */
extern "C" s32 fn_801ED200(s32 index, u8 side)
{
    if (index >= userdata_item_slot_count(lobby_world_block)) {
        return 0;
    }
    if (userdata_equip_item_id_get(lobby_world_block, index) == 0) {
        return 0;
    }
    return userdata_pouch_room_get(lobby_world_block, side == 0);
}

/* Copies the working equipment set into the backup run and re-arms the pane's two counters. */
extern "C" void fn_801ED284(LbListPane* self)
{
    LbConfigWork* cfg;
    s32 i;

    cfg = (LbConfigWork*)self->data_0x010;
    self->row_0x014 = 0;
    self->column_0x018 = 0;
    self->row_sub_0x016 = 0;
    self->column_sub_0x01A = 0;
    for (i = 0; i < 8; i++) {
        cfg->backup_ids_0x61A[i] = cfg->ids_0x5F2[i];
        cfg->backup_kinds_0x62A[i] = cfg->kinds_0x602[i];
    }
    if (userdata_gunner_ck(lobby_world_block) == 1U) {
        self->side_0x005 = 1;
        self->row_sub_max_0x017 = menu_page_count(userdata_pouch_size(1U), 8);
        self->column_sub_max_0x01B = menu_page_count(userdata_pouch_size(0U), 8);
    } else {
        self->side_0x005 = 0;
        self->row_sub_max_0x017 = menu_page_count(userdata_pouch_size(0U), 8);
        self->column_sub_max_0x01B = menu_page_count(userdata_pouch_size(1U), 8);
    }
    self->move_0x01C = 0;
    self->active_0x026 = 0;
}

/* Slides the pane's item out of the slot by one column. */
extern "C" void fn_801ED3E8(LbListPane* self)
{
    u16 out;

    item_box_store(userdata_equip_item_id_get(lobby_world_block, self->entry_max_0x022), self->entry_0x020, &out);
    userdata_pouch_item_give(lobby_world_block, userdata_equip_item_id_get(lobby_world_block, self->entry_max_0x022), -self->entry_0x020,
                self->side_0x005, 1);
    self->entry_max_0x022 = out;
}

/* Moves the pane's whole item stack to the other side. */
extern "C" void fn_801ED464(LbListPane* self)
{
    u8 side;
    u16 id;
    s16 limit;
    s16 v;

    side = self->side_0x005 == 0;
    id = userdata_equip_item_id_get(lobby_world_block, self->entry_max_0x022);
    userdata_pouch_item_give(lobby_world_block, id, self->entry_0x020, side, 1);
    userdata_pouch_item_give(lobby_world_block, id, -self->entry_0x020, self->side_0x005, 1);
    limit = userdata_pouch_size(side);
    v = item_pair_index_find(id, userdata_pouch_get(lobby_world_block, side), limit);
    self->entry_max_0x022 = v;
    self->column_sub_0x01A = v / 8;
}

/* The pane's cursor row as an absolute row. */
extern "C" s16 fn_801ED53C(LbListPane* self)
{
    return (s16)(self->row_0x014 + self->row_sub_0x016 * 8);
}

/* The pane's cursor column as an absolute column. */
extern "C" s16 fn_801ED554(LbListPane* self)
{
    return (s16)(self->column_0x018 + self->column_sub_0x01A * 8);
}


/* Confirms the pane's selection: the button opens the confirm step, the pad steps it. */
extern "C" s32 fn_801ED56C(LbListPane* self, u8 arg)
{
    s32 result;
    u8 side;
    IdValue* pool;
    IdValue* off;

    if (self->active_0x026 == 0) {
        if (lb_cmd_pressed_ck(0x40) == 1U) {
            self->active_0x026 = 1;
            self->stepper_0x02C = 0;
            sysSE_req(0xD);
            return 1;
        }
    } else {
        result = toggle_word_step(&self->stepper_0x02C, lobby_cmd_trig_get(), 4, 8, 0xFFFF);
        switch (result) {
        case 1:
            if (arg == 0) {
                side = self->side_0x005;
            } else {
                side = self->side_0x005 == 0;
            }
            pool = userdata_pouch_get(lobby_world_block, side);
            if (side == 1) {
                off = &pool[0x18];
            } else {
                off = 0;
            }
            item_pages_sort(pool, off);
            self->active_0x026 = 0;
            sysSE_req(0);
            break;
        case 2:
            self->active_0x026 = 0;
            break;
        }
        return 1;
    }
    return 0;
}

/* The pane's item-selection step machine (pick / drag / confirm). */
extern "C" s32 fn_801ED688(LbListPane* self, u8 arg)
{
    s16 idx;
    s32 result;
    s32 r;

    result = 0;
    self->move_0x01C = 0;
    switch (self->phase_0x001) {
    case 0:
        if (fn_801ED56C(self, 0) == 1U) {
            return 0;
        }
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            idx = fn_801ED53C(self);
            self->entry_max_0x022 = idx;
            if (arg == 0) {
                self->move_max_0x01E = fn_801ED184(idx);
            } else {
                self->move_max_0x01E = fn_801ED200(idx, self->side_0x005);
            }
            if (self->move_max_0x01E > 0) {
                self->phase_0x001 = 2;
                if (userdata_equip_item_value_get(lobby_world_block, self->entry_max_0x022) < self->move_max_0x01E) {
                    self->move_max_0x01E = userdata_equip_item_value_get(lobby_world_block, self->entry_max_0x022);
                }
                self->entry_0x020 = self->move_max_0x01E;
                if (arg == 0) {
                    fn_801ED3E8(self);
                } else {
                    fn_801ED464(self);
                }
                sysSE_stop(5);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x8000) == 1U) {
            idx = fn_801ED53C(self);
            self->entry_max_0x022 = idx;
            if (arg == 0) {
                self->move_max_0x01E = fn_801ED184(idx);
            } else {
                self->move_max_0x01E = fn_801ED200(idx, self->side_0x005);
            }
            if (self->move_max_0x01E > 0) {
                self->phase_0x001 += 1;
                if (userdata_equip_item_value_get(lobby_world_block, self->entry_max_0x022) < self->move_max_0x01E) {
                    self->move_max_0x01E = userdata_equip_item_value_get(lobby_world_block, self->entry_max_0x022);
                }
                self->entry_0x020 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->row_0x014 = menu_cursor_step(self->row_0x014, 8, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->row_sub_0x016 = menu_cursor_step_fixed_tail(self->row_sub_0x016, self->row_sub_max_0x017,
                                                              lb_cmd_repeat_get(), 4, 8, &self->move_0x01C);
        }
        break;
    case 1:
        r = menu_cursor_page_move(&self->entry_0x020, self->move_max_0x01E, lobby_w.count_0x084,
                                  lb_cmd_repeat_get(), &self->move_0x01C);
        switch (r) {
        case 1:
            self->phase_0x001 += 1;
            if (arg == 0) {
                fn_801ED3E8(self);
            } else {
                fn_801ED464(self);
            }
            sysSE_stop(5);
            break;
        case 2:
            self->phase_0x001 -= 1;
            sysSE_req(1);
            break;
        }
        break;
    case 2:
        if (lb_cmd_pressed_ck(0x30) == 1U) {
            self->phase_0x001 = 0;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* The affordable count of the slot's item. */
extern "C" s32 fn_801ED9E4(s32 index)
{
    IdValue* slot;
    s32 limit;
    s32 value;

    slot = &lobby_world_block->slots_0x180[index];
    limit = userdata_item_total_count(lobby_world_block, slot->id);
    value = slot->value;
    if (limit < value) {
        value = limit;
    }
    return value;
}

/* The affordable count of the `index`-th entry of the side's own slot table. */
extern "C" s32 fn_801EDA34(s32 index, u8 side)
{
    IdValue* slot;
    s32 limit;
    s32 value;

    slot = &userdata_pouch_get(lobby_world_block, side == 0)[index];
    limit = userdata_item_total_count(lobby_world_block, slot->id);
    value = slot->value;
    if (limit >= value) {
        limit = value;
    }
    return limit;
}

/* Whether the entry can be picked: 1 unknown item, 2 an occupied slot, 0 free. */
extern "C" u8 fn_801EDA9C(LbListPane* self, u16 index, u16 item_id)
{
    if ((u8)*GetItemData(item_id) != 1 && index >= 0x18U) {
        return 1;
    }
    if (item_slots_free_count(&lobby_world_block->slots_0x180[0], userdata_box_capacity(lobby_world_block)) == 0
        && userdata_equip_item_id_get(lobby_world_block, index) != 0) {
        return 2;
    }
    return 0;
}

/* Whether the pane's entry can be moved: 1 unknown, 2 too big, 3 no room, 0 ok. */
extern "C" u8 fn_801EDB34(LbListPane* self, u16 index, u16 item_id)
{
    u8 side;
    u8* data;
    u16 slot_id;
    s16 limit;

    side = self->side_0x005 == 0;
    data = GetItemData(item_id);
    if (index >= 0x18U && *data != 1) {
        return 1;
    }
    slot_id = userdata_equip_item_id_get(lobby_world_block, index);
    if (slot_id != 0) {
        data = GetItemData(slot_id);
        limit = userdata_pouch_size(side);
        if ((s16)item_pair_index_find(item_id, userdata_pouch_get(lobby_world_block, side), limit) >= 0x18 && *data != 1) {
            return 2;
        }
        limit = userdata_pouch_size(side);
        if ((s16)item_pair_index_find(slot_id, userdata_pouch_get(lobby_world_block, side), limit) >= 0) {
            return 3;
        }
    }
    return 0;
}

/* The item-selection step machine driven by the digit entry (pick / drag / confirm). */
extern "C" s32 fn_801EDC4C(LbDigitPane* self, LbListPane* list)
{
    IdValue tmp;
    LbItemDb* db;
    IdValue* entry;
    s32 result;
    u16 value;
    s16 index;
    s32 limit;
    s16 move;
    s32 free;
    s32 extra;
    u16 id;

    result = 0;
    db = lobby_world_block;
    self->flags_0x08 = 0;
    fn_801EC804(&self->anim_0x0E);
    fn_801ECEF0(self);
    switch (self->phase_0x00) {
    case 0:
        if (fn_801ED048(self, list) == 1U) {
            return 0;
        }
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            self->cursor_0x1A = fn_801ED160(self);
            index = fn_801ED9E4(self->cursor_0x1A);
            self->pick_max_0x16 = index;
            if (index > 0) {
                entry = &lobby_world_block->slots_0x180[self->cursor_0x1A];
                limit = userdata_item_slot_count(lobby_world_block);
                move = item_pair_index_find(entry->id, userdata_equip_item_slots_get(lobby_world_block), limit);
                list->entry_max_0x022 = move;
                list->item_id_0x024 = entry->id;
                if (move < 0) {
                    self->phase_0x00 += 1;
                    limit = userdata_item_slot_count(lobby_world_block);
                    free = item_pair_index_find(0, userdata_equip_item_slots_get(lobby_world_block), limit);
                    if (free >= 0) {
                        if (list->side_0x005 == 1 && *GetItemData(list->item_id_0x024) == 1) {
                            extra = item_pair_index_find(0, &userdata_equip_item_slots_get(lobby_world_block)[0x18], 8);
                            if (extra >= 0) {
                                free = extra + 0x18;
                            }
                        }
                        list->row_sub_0x016 = free / 8;
                        list->row_0x014 = (u8)(free % 8);
                    } else {
                        list->row_sub_0x016 = 0;
                        list->row_0x014 = 0;
                    }
                } else {
                    self->phase_0x00 = 2;
                    list->row_sub_0x016 = list->entry_max_0x022 / 8;
                    list->row_0x014 = (u8)(list->entry_max_0x022 % 8);
                }
                self->pick_count_0x18 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else {
            fn_801ECDD4(self);
        }
        break;
    case 1:
        if (fn_801ED56C(list, 0) == 1U) {
            return 0;
        }
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            id = fn_801ED53C(list);
            value = id;
            if (fn_801EDA9C(list, id, list->item_id_0x024) == 0) {
                self->phase_0x00 += 1;
                list->entry_max_0x022 = value;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            self->phase_0x00 -= 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            list->row_0x014 = menu_cursor_step(list->row_0x014, 8, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            list->row_sub_0x016 = menu_cursor_step_fixed_tail(list->row_sub_0x016, list->row_sub_max_0x017,
                                                              lb_cmd_repeat_get(), 4, 8, &list->move_0x01C);
        }
        break;
    case 2:
        switch (menu_cursor_page_move(&self->pick_count_0x18, self->pick_max_0x16, lobby_w.count_0x084,
                                      lb_cmd_repeat_get(), &self->flags_0x08)) {
        case 1:
            id = db->slots_0x180[self->cursor_0x1A].id;
            entry = &userdata_equip_item_slots_get(lobby_world_block)[list->entry_max_0x022];
            if (entry->id != 0 && id != entry->id) {
                item_pair_copy(&tmp, entry);
                item_pair_clear(&userdata_pouch_get(lobby_world_block, list->side_0x005)[(u16)list->entry_max_0x022]);
                item_take(id, self->pick_count_0x18,
                            &userdata_pouch_get(lobby_world_block, list->side_0x005)[(u16)list->entry_max_0x022], 1, 1, 0);
                item_box_slot_take(&lobby_world_block->slots_0x180[0], self->cursor_0x1A, -self->pick_count_0x18);
                item_box_store(tmp.id, tmp.value, &value);
            } else {
                item_take(id, self->pick_count_0x18,
                            &userdata_pouch_get(lobby_world_block, list->side_0x005)[list->entry_max_0x022], 1, 1, 0);
                item_box_slot_take(&lobby_world_block->slots_0x180[0], self->cursor_0x1A, -self->pick_count_0x18);
            }
            limit = userdata_item_slot_count(lobby_world_block);
            list->entry_max_0x022 = item_pair_index_find(id, userdata_equip_item_slots_get(lobby_world_block), limit);
            list->row_sub_0x016 = list->entry_max_0x022 / 8;
            self->phase_0x00 += 1;
            sysSE_stop(5);
            break;
        case 2:
            limit = userdata_item_slot_count(lobby_world_block);
            if (item_pair_index_find(lobby_world_block->slots_0x180[self->cursor_0x1A].id,
                            userdata_equip_item_slots_get(lobby_world_block), limit) < 0) {
                self->phase_0x00 -= 1;
            } else {
                self->phase_0x00 = 0;
            }
            sysSE_req(1);
            break;
        }
        break;
    case 3:
        if (lb_cmd_pressed_ck(0x30) == 1U) {
            self->phase_0x00 = 0;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* The pane's item-selection step machine (the second pane's variant). */
extern "C" s32 fn_801EE1E4(LbListPane* self)
{
    IdValue tmp2;
    IdValue* pouch;
    s16 value;
    s16 index;
    s32 limit;
    s16 move;
    s32 free;
    s32 extra;
    u16 id;
    s32 result;
    s32 r;
    u8 side;

    result = 0;
    self->move_0x01C = 0;
    switch (self->phase_0x001) {
    case 0:
        if (fn_801ED56C(self, 1) == 1U) {
            return 0;
        }
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            id = fn_801ED554(self);
            index = fn_801EDA34(id, self->side_0x005);
            self->move_max_0x01E = index;
            if (index > 0) {
                pouch = userdata_pouch_get(lobby_world_block, self->side_0x005 == 0);
                limit = userdata_item_slot_count(lobby_world_block);
                move = item_pair_index_find(pouch[id].id, userdata_equip_item_slots_get(lobby_world_block), limit);
                self->entry_max_0x022 = move;
                self->item_id_0x024 = pouch[id].id;
                if (move < 0) {
                    self->phase_0x001 += 1;
                    limit = userdata_item_slot_count(lobby_world_block);
                    free = item_pair_index_find(0, userdata_equip_item_slots_get(lobby_world_block), limit);
                    if (free >= 0) {
                        if (self->side_0x005 == 1 && *GetItemData(self->item_id_0x024) == 1) {
                            extra = item_pair_index_find(0, &userdata_equip_item_slots_get(lobby_world_block)[0x18], 8);
                            if (extra >= 0) {
                                free = extra + 0x18;
                            }
                        }
                        self->row_sub_0x016 = free / 8;
                        self->row_0x014 = (u8)(free % 8);
                    } else {
                        self->row_sub_0x016 = 0;
                        self->row_0x014 = 0;
                    }
                } else {
                    self->phase_0x001 = 2;
                    self->row_sub_0x016 = self->entry_max_0x022 / 8;
                    self->row_0x014 = (u8)(self->entry_max_0x022 % 8);
                }
                self->entry_0x020 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->column_0x018 = menu_cursor_step(self->column_0x018, 8, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->column_sub_0x01A = menu_cursor_step_fixed_tail(self->column_sub_0x01A, self->column_sub_max_0x01B,
                                                                 lb_cmd_repeat_get(), 4, 8, &self->move_0x01C);
        }
        break;
    case 1:
        if (fn_801ED56C(self, 0) == 1U) {
            return 0;
        }
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            id = fn_801ED53C(self);
            if (fn_801EDB34(self, id, self->item_id_0x024) == 0) {
                side = self->side_0x005 == 0;
                if (userdata_equip_item_id_get(lobby_world_block, id) != 0
                    && userdata_pouch_room_get(lobby_world_block, side) <= 0) {
                    self->phase_0x001 = 3;
                    item_pair_copy(&tmp2, &userdata_equip_item_slots_get(lobby_world_block)[id]);
                    item_pair_clear(&userdata_pouch_get(lobby_world_block, self->side_0x005)[id]);
                    item_take(self->item_id_0x024, self->move_max_0x01E,
                                &userdata_pouch_get(lobby_world_block, self->side_0x005)[id], 1, 1, 0);
                    userdata_pouch_item_give(lobby_world_block, self->item_id_0x024, -self->move_max_0x01E, side, 1);
                    userdata_pouch_item_give(lobby_world_block, tmp2.id, tmp2.value, side, 1);
                    limit = userdata_item_slot_count(lobby_world_block);
                    self->entry_max_0x022 = item_pair_index_find(self->item_id_0x024,
                                                        userdata_equip_item_slots_get(lobby_world_block), limit);
                    self->row_sub_0x016 = self->entry_max_0x022 / 8;
                    sysSE_stop(5);
                } else {
                    self->phase_0x001 += 1;
                    self->entry_max_0x022 = id;
                    sysSE_req(0);
                }
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            self->phase_0x001 -= 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->row_0x014 = menu_cursor_step(self->row_0x014, 8, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->row_sub_0x016 = menu_cursor_step_fixed_tail(self->row_sub_0x016, self->row_sub_max_0x017,
                                                              lb_cmd_repeat_get(), 4, 8, &self->move_0x01C);
        }
        break;
    case 2:
        r = menu_cursor_page_move(&self->entry_0x020, self->move_max_0x01E, lobby_w.count_0x084,
                                  lb_cmd_repeat_get(), &self->move_0x01C);
        switch (r) {
        case 1:
            pouch = &userdata_equip_item_slots_get(lobby_world_block)[self->entry_max_0x022];
            side = self->side_0x005 == 0;
            if (pouch->id != 0 && self->item_id_0x024 != pouch->id) {
                item_pair_copy(&tmp2, pouch);
                item_pair_clear(&userdata_pouch_get(lobby_world_block, self->side_0x005)[(u16)self->entry_max_0x022]);
                item_take(self->item_id_0x024, self->entry_0x020,
                            &userdata_pouch_get(lobby_world_block, self->side_0x005)[(u16)self->entry_max_0x022], 1, 1, 0);
                userdata_pouch_item_give(lobby_world_block, self->item_id_0x024, -self->entry_0x020, side, 1);
                userdata_pouch_item_give(lobby_world_block, tmp2.id, tmp2.value, side, 1);
            } else {
                item_take(self->item_id_0x024, self->entry_0x020,
                            &userdata_pouch_get(lobby_world_block, self->side_0x005)[self->entry_max_0x022], 1, 1, 0);
                userdata_pouch_item_give(lobby_world_block, self->item_id_0x024, -self->entry_0x020, side, 1);
            }
            limit = userdata_item_slot_count(lobby_world_block);
            self->entry_max_0x022 = item_pair_index_find(self->item_id_0x024, userdata_equip_item_slots_get(lobby_world_block), limit);
            self->row_sub_0x016 = self->entry_max_0x022 / 8;
            self->phase_0x001 += 1;
            sysSE_stop(5);
            break;
        case 2:
            limit = userdata_item_slot_count(lobby_world_block);
            if (item_pair_index_find(self->item_id_0x024, userdata_equip_item_slots_get(lobby_world_block), limit) < 0) {
                self->phase_0x001 -= 1;
            } else {
                self->phase_0x001 = 0;
            }
            sysSE_req(1);
            break;
        }
        break;
    case 3:
        if (lb_cmd_pressed_ck(0x30) == 1U) {
            self->phase_0x001 = 0;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* Whether the entry's item is one the shop can hand over. */
extern "C" u32 fn_801EE900(u16 item_id)
{
    if (item_id != 0 && lb_combo_item_known_ck(item_id) == 1U) {
        return 1;
    }
    return 0;
}

/* Whether the entry's item is one the shop can accept. */
extern "C" s32 fn_801EE940(u16 index, u16 item_id)
{
    if (item_id != 0 && lb_combo_recipe_find(index, item_id, 0) != 0) {
        return 1;
    }
    return 0;
}


/* The box combine step machine: the digit entry picks the first item, the cursor the second, then the
 * confirm and the result hand-over. */
extern "C" s32 lb_box_combine_step(LbDigitPane* self, MenuSelCursor* cur, LbListPane* list)
{
    IdValue* slot;
    s32 result;
    u16 out;
    s16 value;
    u16 hold;
    u16 repeat;
    u16 trig;
    s32 r;

    result = 0;
    if (chk_pointer() == 1U) {
        hold = 0;
        repeat = 0;
        trig = 0;
    } else {
        hold = lobby_w.cmd_mask_0x084[0][1];
        repeat = lb_cmd_repeat_get();
        trig = lobby_w.count_0x084;
    }
    self->flags_0x08 = 0;
    fn_801EC804(&self->anim_0x0E);
    fn_801ECEF0(self);
    menu_sel_cursor_blend_tick(cur);
    switch (self->phase_0x00) {
    case 0:
        if (chk_pointer() == 1U) {
            break;
        }
        if (self->sub_0x01 == 0) {
            self->sub_0x01++;
            menu_sel_cursor_init(cur, 2, 0);
        }
        if (fn_801ED048(self, list) == 1U) {
            return 0;
        }
    case 1:
        if (chk_pointer() == 1U) {
            break;
        }
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            value = fn_801ED160(self);
            slot = &lobby_world_block->slots_0x180[value];
            switch (self->phase_0x00) {
            case 0:
                if (fn_801EE900(slot->id) == 1U) {
                    self->phase_0x00++;
                    menu_sel_cursor_first_set(cur, value, slot->id);
                    self->cursor_0x1A = value;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
                break;
            case 1:
                if (slot->id != 0 && menu_sel_cursor_second_set(cur, value, slot->id) == 1U) {
                    self->phase_0x00++;
                    if (menu_sel_cursor_status_update(cur) == 1U) {
                        self->stepper_0x04 = 0;
                    } else {
                        self->stepper_0x04 = 1;
                    }
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            switch (self->phase_0x00) {
            case 0:
                sysSE_req(1);
                result = 2;
                break;
            case 1:
                self->phase_0x00--;
                self->sub_0x01 = 0;
                sysSE_req(1);
                break;
            }
        } else {
            fn_801ECDD4(self);
        }
        break;
    case 2:
        cur->mode_0x16 = 0;
        if (menu_sel_cursor_status_update(cur) == 0) {
            self->stepper_0x04 = 1;
        }
        r = menu_sel_cursor_confirm_step(cur, &self->stepper_0x04, hold, repeat, trig);
        switch (r) {
        case 1:
            self->phase_0x00++;
            menu_sel_cursor_combine_start(cur, 0);
            break;
        case 2:
            if ((u16)cur->slot_a_0x08 == 0xFFFF || (u16)cur->slot_b_0x0A == 0xFFFF) {
                self->phase_0x00 = 0;
                self->sub_0x01 = 0;
                menu_sel_cursor_init(cur, 2, 0);
            } else {
                menu_sel_cursor_second_set(cur, 0xFFFF, 0);
                self->phase_0x00 = 1;
            }
            break;
        }
        break;
    case 3:
        if (menu_sel_cursor_combine_step(cur, trig, hold) == 1U) {
            if (item_slots_room_get(cur->value_0x0C, &lobby_world_block->slots_0x180[0],
                                    userdata_box_capacity(lobby_world_block)) <= 0) {
                hud_item_msg_push(2, 0x11, (u16)cur->value_0x0C);
            } else if (item_box_store(cur->value_0x0C, cur->anim_0x15, &out) != 0) {
                hud_item_msg_push(2, 0x11, (u16)cur->value_0x0C);
            }
            menu_sel_cursor_apply_start(cur);
            self->phase_0x00++;
            if ((u16)cur->slot_a_0x08 == 0xFFFF || (u16)cur->slot_b_0x0A == 0xFFFF) {
                self->stepper_0x04 = 1;
            }
        }
        break;
    case 4:
        r = menu_sel_cursor_apply_step(cur, trig, hold);
        switch (r) {
        case 1:
            menu_sel_cursor_combine_start(cur, 1);
            self->phase_0x00 = 3;
            break;
        case 2:
            if (cur->status_0x10 >= 0) {
                self->stepper_0x04 = 1;
            }
            self->phase_0x00 = 2;
            break;
        }
        break;
    }
    return result;
}

/* The item-selection step machine driven by the digit entry's confirm (the swap variant). */
extern "C" s32 fn_801EEDC4(LbDigitPane* self, LbListPane* list)
{
    IdValue tmp;
    s32 result;
    s16 value;

    result = 0;
    self->flags_0x08 = 0;
    fn_801EC804(&self->anim_0x0E);
    fn_801ECEF0(self);
    switch (self->phase_0x00) {
    case 0:
        if (fn_801ED048(self, list) == 1U) {
            return 0;
        }
        /* falls through into the step handling, like the target's shared tail */
    case 1:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            value = fn_801ED160(self);
            switch (self->phase_0x00) {
            case 0:
                self->phase_0x00 += 1;
                self->cursor_0x1A = value;
                sysSE_req(0xE);
                break;
            case 1:
                if ((lobby_world_block->slots_0x180[value].id == 0
                     && lobby_world_block->slots_0x180[self->cursor_0x1A].id == 0)
                    || value == self->cursor_0x1A) {
                    sysSE_req(2);
                } else {
                    self->phase_0x00 -= 1;
                    item_pair_copy(&tmp, &lobby_world_block->slots_0x180[value]);
                    item_pair_copy(&lobby_world_block->slots_0x180[value],
                                   &lobby_world_block->slots_0x180[self->cursor_0x1A]);
                    item_pair_copy(&lobby_world_block->slots_0x180[self->cursor_0x1A], &tmp);
                    sysSE_req(0xF);
                }
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            switch (self->phase_0x00) {
            case 0:
                sysSE_req(1);
                result = 2;
                break;
            case 1:
                self->phase_0x00 -= 1;
                sysSE_req(1);
                break;
            }
        } else {
            fn_801ECDD4(self);
        }
        break;
    default:
        break;
    }
    return result;
}

/* The item-selection step machine driven by the digit entry (the stock variant). */
extern "C" s32 fn_801EEFA0(LbDigitPane* self, LbListPane* list)
{
    LbItemDb* db;
    s32 result;
    s16 value;
    s16 index;
    s32 r;

    result = 0;
    db = lobby_world_block;
    self->flags_0x08 = 0;
    fn_801EC804(&self->anim_0x0E);
    fn_801ECEF0(self);
    switch (self->phase_0x00) {
    case 0:
        if (fn_801ED048(self, list) == 1U) {
            return 0;
        }
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            value = fn_801ED160(self);
            self->cursor_0x1A = value;
            index = db->slots_0x180[value].value;
            self->pick_max_0x16 = index;
            if (index > 0) {
                self->phase_0x00 += 1;
                self->pick_count_0x18 = 1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else {
            fn_801ECDD4(self);
        }
        break;
    case 1:
        r = menu_cursor_page_move(&self->pick_count_0x18, self->pick_max_0x16, lobby_w.count_0x084,
                                  lb_cmd_repeat_get(), &self->flags_0x08);
        switch (r) {
        case 1:
            self->phase_0x00 += 1;
            self->stepper_0x04 = 0;
            sysSE_req(0);
            break;
        case 2:
            self->phase_0x00 -= 1;
            sysSE_req(1);
            break;
        }
        break;
    case 2:
        r = toggle_word_step(&self->stepper_0x04, lobby_cmd_trig_get(), 4, 8, 0xFFFF);
        switch (r) {
        case 1:
            self->phase_0x00 = 0;
            score_add_clamped(self->pick_count_0x18 * eft052_item_value_get(db->slots_0x180[self->cursor_0x1A].id),
                        &lobby_world_block->data_0x18[0]);
            sysSE_req(9);
            item_box_slot_take(&lobby_world_block->slots_0x180[0], self->cursor_0x1A, -self->pick_count_0x18);
            break;
        case 2:
            self->phase_0x00 -= 1;
            break;
        }
        break;
    }
    return result;
}

/* Whether the equipment pane holds a complete set (or whether its bowgun can be fired). */
extern "C" s32 fn_801EF1D0(LbEquipPane* self)
{
    u16 id;

    id = self->ids_0x04[0];
    if (id != 0xFFFFU && self->ids_0x04[1] != 0xFFFFU && self->ids_0x04[2] != 0xFFFFU) {
        return 1;
    }
    if (id != 0xFFFFU) {
        return Gunner_opt_ok_ck((_EQUIP*)&lobby_world_block->equip_0x0E00[id]) == 0;
    }
    return 0;
}

/* Seeds the equipment pane with the three ids and the icon run. */
extern "C" void lb_equip_set_pane_init(LbEquipPane* self, s16 index, u16 a, u16 b, u16 c, u8 side, struct LbPage* page)
{
    u16 id_a;
    LbEquipRec* rec;
    u16 id_b;
    u16 id_c;

    id_a = a;
    id_b = b;
    id_c = c;
    self->phase_0x00 = 0;
    self->index_0x02 = index;
    if (lobby_world_block->equip_0x0E00[lobby_world_block->kind_0x8C].kind_0x00 == 0xB) {
        if (a == 0xFFFFU) {
            if (Gunner_opt_ok_ck((_EQUIP*)&lobby_world_block->equip_0x0E00[lobby_world_block->kind_0x8C]) == 0) {
                id_a = 0xFFFF;
            } else {
                id_a = lobby_world_block->kind_0x8C;
            }
        }
        rec = id_a != 0xFFFFU ? &lobby_world_block->equip_0x0E00[id_a] : NULL;
        if (rec != NULL && Gunner_opt_ok_ck((_EQUIP*)rec) == 0) {
            id_b = 0xFFFF;
            id_c = id_b;
        } else {
            if (id_b == 0xFFFFU) {
                id_b = lobby_world_block->sub_0x8E;
            }
            if (id_c == 0xFFFFU) {
                id_c = lobby_world_block->sub_0x90;
            }
        }
    }
    self->ids_0x04[0] = id_a;
    self->ids_0x04[1] = id_b;
    self->ids_0x04[2] = id_c;
    self->side_0x01 = side;
    self->page_0x90 = page;
    if (side == 0) {
        lb_scroll_init(page, &self->scroll_0x10, 5, 7);
    } else {
        lb_scroll_init(page, &self->scroll_0x10, 5, 6);
    }
    self->scroll_0x10.row_0x08 = 4;
    self->scroll_0x10.flags_0x10 |= 1;
}

/* The set pane's step machine: picks the slot to fill, hands the entry pane the slot's kind, and confirms or
 * cancels the set. */
extern "C" s32 lb_equip_set_pane_step(LbEquipPane* self, LbEntryPane* entry, LbEquipBoxWork* owner)
{
    s32 result;
    s32 rows;
    s16 id_a;
    s16 id_b;
    s16 id_c;
    u16 id;

    result = 0;
    self->scroll_0x10.moved_0x06 = 0;
    switch (self->phase_0x00) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            id = self->ids_0x04[0];
            if (id != 0xFFFF) {
                id_a = id;
            } else {
                id_a = -1;
            }
            if (self->ids_0x04[1] != 0xFFFF) {
                id_b = self->ids_0x04[1];
            } else {
                id_b = -1;
            }
            if (self->ids_0x04[2] != 0xFFFF) {
                id_c = self->ids_0x04[2];
            } else {
                id_c = -1;
            }
            switch (self->index_0x02) {
            case 0:
                self->phase_0x00++;
                lb_entry_pane_kind_set(entry, 1, 1);
                lb_list_pane_pos_set(entry, id_a, id_b, id_c);
                sysSE_req(0);
                break;
            case 1:
                if (id != 0xFFFF && Gunner_opt_ok_ck((_EQUIP*)&lobby_world_block->equip_0x0E00[id]) == 0) {
                    sysSE_req(2);
                    break;
                }
                self->phase_0x00++;
                lb_entry_pane_kind_set(entry, 2, 1);
                lb_list_pane_pos_set(entry, id_a, id_b, id_c);
                sysSE_req(0);
                break;
            case 2:
                if (id != 0xFFFF && Gunner_opt_ok_ck((_EQUIP*)&lobby_world_block->equip_0x0E00[id]) == 0) {
                    sysSE_req(2);
                    break;
                }
                self->phase_0x00++;
                lb_entry_pane_kind_set(entry, 3, 1);
                lb_list_pane_pos_set(entry, id_a, id_b, id_c);
                sysSE_req(0);
                break;
            default:
                sysSE_stop(3);
                result = 1;
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            self->phase_0x00 = 2;
            self->stepper_0x0C = 0;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            rows = (fn_801EF1D0(self) == 1) + 3;
            self->index_0x02 = menu_cursor_step(self->index_0x02, (s16)rows, lb_cmd_repeat_get(), 1, 2);
        } else if (lobby_cmd_release_ck(0xC00) != 0) {
            self->scroll_0x10.row_0x08 = (u8)menu_cursor_step_fixed_tail(
                self->scroll_0x10.row_0x08, self->scroll_0x10.rows_0x0A, lobby_cmd_release_get(), 0x800, 0x400,
                &self->scroll_0x10.moved_0x06);
        }
        break;
    case 1:
        switch (lb_entry_pane_select_step(entry, owner, self)) {
        case 2:
            self->phase_0x00 = 0;
            break;
        case 1:
            self->phase_0x00 = 0;
            self->ids_0x04[self->index_0x02] = lb_digit_pane_value(entry);
            if (self->index_0x02 == 0
                && Gunner_opt_ok_ck((_EQUIP*)&lobby_world_block->equip_0x0E00[self->ids_0x04[0]]) == 0) {
                self->ids_0x04[1] = 0xFFFF;
                self->ids_0x04[2] = 0xFFFF;
            }
            break;
        }
        break;
    case 2:
        switch (toggle_word_step(&self->stepper_0x0C, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            sysSE_req(1);
            result = 2;
            break;
        case 2:
            self->phase_0x00 = 0;
            break;
        }
        break;
    }
    return result;
}

/* The digit entry's number, as the id lookup takes it. */
extern "C" s16 lb_digit_pane_value(LbEntryPane* self)
{
    return (s16)((self->digit_ones_0x00A + self->digit_hundreds_0x002 * 100)
                      + self->digit_tens_0x00C * 10);
}


/* The scroll-kind tables `lb_entry_pane_kind_tbl_get` picks from. */
u8 lb_entry_scroll_kinds_four[4] = {1, 2, 4, 7};
u8 lb_entry_scroll_kinds_four_empty[4] = {1, 2, 7, 0};
u8 lb_entry_scroll_kinds_six[8] = {1, 2, 5, 6, 10, 7, 0, 0};
u8 lb_entry_scroll_kinds_six_empty[8] = {1, 2, 5, 6, 7, 0, 0, 0};

extern "C" {
}
/* Rebuilds the scroll list's record tables for the record the digits name, and keeps the scroll row in range when
 * the row count changed. */
extern "C" void lb_entry_pane_scroll_refresh(LbEntryPane* self)
{
    LbEquipRec set[3];
    LbEquipRec show[3];
    LbItemDb* db;
    LbEquipRec* rec;
    u16 id;
    s16 rows;
    s16 row;
    u8 kind;

    db = lobby_world_block;
    rows = self->scroll_0x04C.rows_0x0A;
    row = self->scroll_0x04C.row_0x08;
    id = lb_digit_pane_value(self);
    rec = &db->equip_0x0E00[id];
    if (rec->kind_0x00 != 0) {
        if (self->slot_kind_0x016 >= 1 && self->slot_kind_0x016 <= 3) {
            memset(set, 0, sizeof(set));
            kind = self->scroll_0x04C.kind_0x00;
            if ((u32)(kind - 5) <= 1 || kind == 10) {
                if (self->set_ids_0x018[0] != -1) {
                    equip_record_copy(&set[0], &db->equip_0x0E00[self->set_ids_0x018[0]]);
                } else {
                    set[0].kind_0x00 = 0xB;
                    set[0].value_0x02 = 0;
                }
                if (Gunner_opt_ok_ck((_EQUIP*)&set[0]) == 1U) {
                    if (self->set_ids_0x018[1] != -1) {
                        equip_record_copy(&set[1], &db->equip_0x0E00[self->set_ids_0x018[1]]);
                    } else {
                        set[1].kind_0x00 = 0xC;
                        set[1].value_0x02 = 0;
                    }
                    if (self->set_ids_0x018[2] != -1) {
                        equip_record_copy(&set[2], &db->equip_0x0E00[self->set_ids_0x018[2]]);
                    } else {
                        set[2].kind_0x00 = 0xD;
                        set[2].value_0x02 = 0;
                    }
                } else {
                    set[1].kind_0x00 = 0xC;
                    set[1].value_0x02 = set[0].value_0x02;
                    set[2].kind_0x00 = 0xD;
                    set[2].value_0x02 = set[0].value_0x02;
                }
                equip_record_copy(&show[0], &set[0]);
                equip_record_copy(&show[1], &set[1]);
                equip_record_copy(&show[2], &set[2]);
                switch (self->slot_kind_0x016) {
                case 1:
                    if (rec->kind_0x00 == 0xB) {
                        equip_record_copy(&show[0], rec);
                    } else {
                        memset(&show[0], 0, sizeof(LbEquipRec));
                        show[0].kind_0x00 = 0xB;
                        show[0].value_0x02 = 0;
                    }
                    if (Gunner_opt_ok_ck((_EQUIP*)&show[0]) == 0) {
                        memset(&show[1], 0, sizeof(LbEquipRec) * 2);
                        show[1].kind_0x00 = 0xC;
                        show[1].value_0x02 = show[0].value_0x02;
                        show[2].kind_0x00 = 0xD;
                        show[2].value_0x02 = show[0].value_0x02;
                    } else if (Gunner_opt_ok_ck((_EQUIP*)&set[0]) == 0) {
                        memset(&show[1], 0, sizeof(LbEquipRec) * 2);
                        show[1].kind_0x00 = 0xC;
                        show[1].value_0x02 = 0;
                        show[2].kind_0x00 = 0xD;
                        show[2].value_0x02 = 0;
                    }
                    break;
                case 2:
                    if (rec->kind_0x00 == 0xC) {
                        equip_record_copy(&show[1], rec);
                    } else {
                        memset(&show[1], 0, sizeof(LbEquipRec));
                        show[1].kind_0x00 = 0xC;
                        show[1].value_0x02 = 0;
                    }
                    break;
                case 3:
                    if (rec->kind_0x00 == 0xD) {
                        equip_record_copy(&show[2], rec);
                    } else {
                        memset(&show[2], 0, sizeof(LbEquipRec));
                        show[2].kind_0x00 = 0xD;
                        show[2].value_0x02 = 0;
                    }
                    break;
                }
                lb_scroll_tables_set(&self->scroll_0x04C, &set[0], &set[1], &show[0], &show[1]);
            } else if ((u8)((u8)equip_kind_table_class(rec->kind_0x00) - 1) <= 1) {
                switch (self->slot_kind_0x016) {
                case 1:
                    if (self->set_ids_0x018[0] != -1) {
                        equip_record_copy(&set[0], &db->equip_0x0E00[self->set_ids_0x018[0]]);
                    } else {
                        set[0].kind_0x00 = 0xB;
                        set[0].value_0x02 = 0;
                    }
                    break;
                case 2:
                    if (self->set_ids_0x018[1] != -1) {
                        equip_record_copy(&set[0], &db->equip_0x0E00[self->set_ids_0x018[1]]);
                    } else {
                        set[0].kind_0x00 = 0xC;
                        set[0].value_0x02 = 0;
                    }
                    break;
                case 3:
                    if (self->set_ids_0x018[2] != -1) {
                        equip_record_copy(&set[0], &db->equip_0x0E00[self->set_ids_0x018[2]]);
                    } else {
                        set[0].kind_0x00 = 0xD;
                        set[0].value_0x02 = 0;
                    }
                    break;
                }
                lb_scroll_tables_set(&self->scroll_0x04C, &set[0], NULL, NULL, NULL);
                lb_equip_base_set(&self->scroll_0x04C, rec);
            } else {
                lb_equip_compare_set(&self->scroll_0x04C, rec);
                kind = self->scroll_0x04C.kind_0x00;
                if ((kind == 2 || kind == 6) && (u8)equip_kind_table_class(rec->kind_0x00) == 0
                    && userdata_equip_box_worn_ck(db, id) == 1U) {
                    memset(&set[0], 0, sizeof(LbEquipRec));
                    set[0].kind_0x00 = rec->kind_0x00;
                    lb_equip_base_set(&self->scroll_0x04C, &set[0]);
                } else {
                    lb_equip_base_set(&self->scroll_0x04C, rec);
                }
            }
            lb_scroll_flag_set(&self->scroll_0x04C, rec->kind_0x00);
        } else {
            lb_equip_compare_set(&self->scroll_0x04C, rec);
            kind = self->scroll_0x04C.kind_0x00;
            if ((kind == 2 || kind == 6) && (u8)equip_kind_table_class(rec->kind_0x00) == 0
                && userdata_equip_box_worn_ck(db, id) == 1U) {
                memset(&set[0], 0, sizeof(LbEquipRec));
                set[0].kind_0x00 = rec->kind_0x00;
                lb_equip_base_set(&self->scroll_0x04C, &set[0]);
            } else {
                lb_equip_base_set(&self->scroll_0x04C, rec);
            }
            lb_scroll_flag_set(&self->scroll_0x04C, rec->kind_0x00);
        }
    }
    switch (self->scroll_0x04C.kind_0x00) {
    case 1:
        if (self->scroll_0x04C.rows_0x0A > rows) {
            if (row == 0) {
                self->scroll_0x04C.row_0x08 = 0;
            } else {
                self->scroll_0x04C.row_0x08 = row + 1;
            }
        } else if (self->scroll_0x04C.rows_0x0A < rows) {
            if (row == 0) {
                self->scroll_0x04C.row_0x08 = 0;
            } else if (row <= 3) {
                self->scroll_0x04C.row_0x08 = row - 1;
            } else {
                self->scroll_0x04C.row_0x08 = 3;
            }
        }
        break;
    case 2:
        if (self->scroll_0x04C.rows_0x0A > rows) {
            if (row == 0) {
                self->scroll_0x04C.row_0x08 = 0;
            } else {
                self->scroll_0x04C.row_0x08 = row + 3;
            }
        } else if (self->scroll_0x04C.rows_0x0A < rows) {
            if (row == 0) {
                self->scroll_0x04C.row_0x08 = 0;
            } else if (row <= 3) {
                self->scroll_0x04C.row_0x08 = 0;
            } else {
                self->scroll_0x04C.row_0x08 = row - 3;
            }
        }
        break;
    }
    if (self->scroll_0x04C.row_0x08 >= self->scroll_0x04C.rows_0x0A) {
        self->scroll_0x04C.row_0x08 = 0;
    }
}

/* Sets the slot kind the pane fills and re-initialises its scroll list for it (unless `keep` and the kind is
 * unchanged), then rebuilds the record tables. */
extern "C" void lb_entry_pane_kind_set(LbEntryPane* self, s16 slot_kind, u8 keep)
{
    struct LbPage* page;

    page = self->page_0x028;
    if (keep == 0 || self->slot_kind_0x016 != slot_kind) {
        switch (slot_kind) {
        case 7:
        case 8:
            self->list_0x02C.cursor_0x00 = 0;
            lb_scroll_init(page, &self->scroll_0x04C, 1, 9);
            break;
        case 9:
        case 10:
            self->list_0x02C.cursor_0x00 = 0;
            lb_scroll_init(page, &self->scroll_0x04C, 1, 8);
            break;
        case 4:
        case 5:
            self->list_0x02C.cursor_0x00 = 0;
            lb_scroll_init(page, &self->scroll_0x04C, 1, 3);
            break;
        case 1:
        case 2:
        case 3:
            if (keep == 0 || self->slot_kind_0x016 < 1 || self->slot_kind_0x016 > 3) {
                self->list_0x02C.cursor_0x00 = 0;
                lb_scroll_init(page, &self->scroll_0x04C, 1, 3);
                self->scroll_0x04C.flags_0x10 |= 1;
            }
            break;
        case 0:
        case 6:
        default:
            self->list_0x02C.cursor_0x00 = 1;
            lb_scroll_init(page, &self->scroll_0x04C, 2, 3);
            break;
        }
    }
    self->slot_kind_0x016 = slot_kind;
    lb_entry_pane_scroll_refresh(self);
}

/* Clears the entry pane, seeds its digit limit from the record count, and builds its list for the slot kind. */
extern "C" void lb_list_pane_init(LbEntryPane* self, s16 slot_kind, struct LbPage* page)
{
    self->phase_0x000 = 0;
    self->sub_0x001 = 0;
    self->digit_ones_0x00A = 0;
    self->digit_tens_0x00C = 0;
    self->digit_hundreds_0x002 = 0;
    self->digit_hundreds_max_0x003 = menu_page_count(userdata_equip_box_capacity(lobby_world_block), 100);
    self->stepper_0x004 = 0;
    self->flags_0x008 = 0;
    self->anim_0x00E = 0;
    self->timer_0x010 = 0;
    self->blink_a_0x012 = -1;
    self->blink_b_0x014 = -1;
    self->page_0x028 = page;
    lb_list_pane_pos_set(self, 0, -1, -1);
    self->list_mode_0x021 = 1;
    lb_list_init(&self->list_0x02C, 4, 0, 0, -1, 0, -1, 3, 0);
    lb_entry_pane_kind_set(self, slot_kind, 0);
    self->unused_0x022 = 0;
    self->sort_step_0x023 = 0;
    self->sort_prev_0x024 = 0;
    self->sort_kind_0x026 = 0;
}

/* Stores the three record ids of the set the pane shows. */
extern "C" void lb_list_pane_pos_set(LbEntryPane* self, s16 id_a, s16 id_b, s16 id_c)
{
    self->set_ids_0x018[0] = id_a;
    self->set_ids_0x018[1] = id_b;
    self->set_ids_0x018[2] = id_c;
}

/* The scroll-kind table of the pane's list mode, the short one when the record slot is empty. */
extern "C" u8* lb_entry_pane_kind_tbl_get(LbEntryPane* self)
{
    LbEquipRec* rec;
    u8* table;
    s16 index;

    index = lb_digit_pane_value(self);
    rec = &lobby_world_block->equip_0x0E00[index];
    if (self->list_mode_0x021 == 2) {
        table = rec->kind_0x00 == 0 ? lb_entry_scroll_kinds_six_empty : lb_entry_scroll_kinds_six;
    } else {
        table = rec->kind_0x00 == 0 ? lb_entry_scroll_kinds_four_empty : lb_entry_scroll_kinds_four;
    }
    return table;
}

/* The scroll kind of the list row under the cursor. */
extern "C" u8 lb_entry_pane_scroll_kind_get(LbEntryPane* self)
{
    return lb_entry_pane_kind_tbl_get(self)[self->list_0x02C.cursor_0x00];
}

/* The entry pane's pad step: the digits on the stick and D-pad (unless the row is a fixed one), then the scroll
 * list on the release keys. */
extern "C" void lb_entry_pane_input(LbEntryPane* self)
{
    LbItemDb* db;

    db = lobby_world_block;
    if (lb_entry_pane_scroll_kind_get(self) != 4 && lb_entry_pane_scroll_kind_get(self) != 10) {
        if (lb_cmd_repeat_ck(0xF) != 0) {
            if (lb_cmd_repeat_ck(0xC) != 0) {
                self->digit_ones_0x00A = menu_cursor_step(self->digit_ones_0x00A, 10, lb_cmd_repeat_get(), 4, 8);
            }
            if (lb_cmd_repeat_ck(3) != 0) {
                self->digit_tens_0x00C = menu_cursor_step(self->digit_tens_0x00C, 10, lb_cmd_repeat_get(), 1, 2);
            }
        } else if (lb_cmd_pressed_ck(0x300) != 0) {
            self->digit_hundreds_0x002 = menu_cursor_step_forward(self->digit_hundreds_0x002,
                                                                  self->digit_hundreds_max_0x003, lobby_cmd_trig_get(),
                                                                  0x100, 0x200, 6, &self->flags_0x008);
            if ((self->flags_0x008 & 0x100) != 0) {
                self->blink_a_0x012 = 0;
            }
            if ((self->flags_0x008 & 0x200) != 0) {
                self->blink_b_0x014 = 0;
            }
        }
        lb_entry_pane_scroll_refresh(self);
        if (db->equip_0x0E00[(u16)lb_digit_pane_value(self)].kind_0x00 != 0 && lobby_cmd_release_ck(0xC00) != 0) {
            self->scroll_0x04C.row_0x08 = (u8)menu_cursor_step_fixed_tail(
                self->scroll_0x04C.row_0x08, self->scroll_0x04C.rows_0x0A, lobby_cmd_release_get(), 0x800, 0x400,
                &self->scroll_0x04C.moved_0x06);
        }
    } else if (db->equip_0x0E00[(u16)lb_digit_pane_value(self)].kind_0x00 != 0 && lobby_cmd_release_ck(0xC00) != 0) {
        self->scroll_0x04C.row_0x08 = (u8)menu_cursor_step_fixed_tail(
            self->scroll_0x04C.row_0x08, self->scroll_0x04C.rows_0x0A, lobby_cmd_release_get(), 0x800, 0x400,
            &self->scroll_0x04C.moved_0x06);
    }
}

/* The record-kind and sub-kind masks (and the weapon class) the sort menu's choices select. */
extern "C" void lb_entry_pane_sort_masks_get(LbEntryPane* self, s32* kinds, s32* picked, u8* weapon)
{
    u16* table;

    switch (self->sort_lists_0x0CC[0].cursor_0x00) {
    case 1:
        switch (self->sort_lists_0x0CC[1].cursor_0x00) {
        case 1:
            *kinds |= 0x80;
            *kinds |= 0x100;
            *kinds |= 0x200;
            *kinds |= 0x400;
            *kinds |= 0x4000;
            *kinds |= 0x8000;
            if (userdata_progress_flag_ck(0xF) == 0 && userdata_progress_flag_ck(0xE) == 1U) {
                table = lb_sort_blade_classes_late;
            } else {
                table = lb_sort_blade_classes;
            }
            switch (table[self->sort_lists_0x0CC[2].cursor_0x00]) {
            case 1:
                *picked |= 0x80;
                break;
            case 6:
                *picked |= 0x4000;
                break;
            case 2:
                *picked |= 0x100;
                break;
            case 4:
                *picked |= 0x400;
                break;
            case 3:
                *picked |= 0x200;
                break;
            case 5:
                *picked |= 0x8000;
                break;
            }
            break;
        case 2:
            *kinds |= 0x800;
            *kinds |= 0x1000;
            *kinds |= 0x2000;
            switch (self->sort_lists_0x0CC[2].cursor_0x00) {
            case 1:
                *picked |= 0x800;
                break;
            case 2:
                *picked |= 0x1000;
                break;
            case 3:
                *picked |= 0x2000;
                break;
            }
            break;
        }
        break;
    case 2:
        *kinds |= 0x20;
        *kinds |= 0x2;
        *kinds |= 0x4;
        *kinds |= 0x8;
        *kinds |= 0x10;
        switch (self->sort_lists_0x0CC[2].cursor_0x00) {
        case 1:
            *picked |= 0x20;
            break;
        case 2:
            *picked |= 0x2;
            break;
        case 3:
            *picked |= 0x4;
            break;
        case 4:
            *picked |= 0x8;
            break;
        case 5:
            *picked |= 0x10;
            break;
        }
        switch (self->sort_lists_0x0CC[1].cursor_0x00) {
        case 1:
            *weapon = 1;
            break;
        case 2:
            *weapon = 2;
            break;
        }
        break;
    case 3:
        *kinds |= 0x40;
        break;
    }
}

/* Fills the sort masks and returns the sort order class of the pane's sort kind. */
extern "C" u8 lb_entry_pane_sort_class_get(LbEntryPane* self, s32* kinds, s32* picked, u8* weapon)
{
    *weapon = 0;
    *kinds = 0;
    *picked = 0;
    lb_entry_pane_sort_masks_get(self, kinds, picked, weapon);
    switch (self->sort_kind_0x026) {
    case 0:
        return 0x10;
    case 1:
        return 0x10;
    case 2:
        return 0xC;
    case 3:
        return 7;
    case 4:
        return 8;
    case 5:
        return 9;
    case 6:
        return 0xA;
    case 7:
        return 0xB;
    case 8:
        return 0xE;
    case 9:
        return 0xF;
    case 10:
        return 0xD;
    case 11:
        return 6;
    case 12:
        return 1;
    case 13:
        return 2;
    case 14:
        return 3;
    case 15:
        return 4;
    case 16:
        return 5;
    case 17:
        return 0x11;
    default:
        return 0x10;
    }
}

/* Closes the sort menu with sort `kind` and rebuilds the 100 sorted record ids. */
extern "C" void lb_entry_pane_sort_apply(LbEntryPane* self, u8 kind)
{
    s32 kinds;
    s32 picked;
    u8 weapon;
    u16 count;
    u8 order;
    s16 id;
    u32 i;

    self->stepper_0x004 = 0;
    self->sort_prev_0x024 = self->sort_step_0x023;
    self->sort_step_0x023 = 0xFF;
    self->sort_kind_0x026 = kind;
    sysSE_req(0xD);
    count = userdata_box_capacity(lobby_world_block);
    order = lb_entry_pane_sort_class_get(self, &kinds, &picked, &weapon);
    id = -1;
    i = 0;
    do {
        id = lb_equip_row_next(order, id, &lobby_world_block->equip_0x0E00[0], count, kinds, picked, weapon);
        self->sorted_ids_0x16C[(u16)i] = (u16)id;
        i++;
    } while (i < 100);
}

/* Re-initialises the pane's list for its mode: four rows for any record, six for a set slot (one fewer each when
 * there is no owner screen, and one fewer again when the record slot is empty). */
extern "C" void lb_entry_pane_list_reset(LbEntryPane* self, LbEquipBoxWork* owner)
{
    s16 delta;
    s16 cursor;
    s16 index;

    delta = -(owner == NULL);
    index = lb_digit_pane_value(self);
    if (lobby_world_block->equip_0x0E00[index].kind_0x00 == 0) {
        delta--;
    }
    if ((u32)(self->slot_kind_0x016 - 1) > 2) {
        if (self->list_mode_0x021 == 2 && self->list_0x02C.cursor_0x00 >= 2) {
            self->list_0x02C.cursor_0x00 -= 2;
        }
        cursor = self->list_0x02C.cursor_0x00;
        lb_list_init(&self->list_0x02C, delta + 4, 0, 0, -1, 0, -1, 3, 0);
        self->list_mode_0x021 = 1;
    } else {
        if (self->list_mode_0x021 == 1 && self->list_0x02C.cursor_0x00 >= 2) {
            self->list_0x02C.cursor_0x00 += 2;
        }
        cursor = self->list_0x02C.cursor_0x00;
        lb_list_init(&self->list_0x02C, delta + 6, 0, 0, -1, 0, -1, 3, 0);
        self->list_mode_0x021 = 2;
    }
    self->list_0x02C.cursor_0x00 = cursor;
}

/* Whether the pane is busy: the list button held or the sort menu open. */
extern "C" s32 lb_entry_pane_busy_ck(LbEntryPane* self)
{
    if (self->held_0x020 != 0 || self->sort_step_0x023 != 0) {
        return 1;
    }
    return 0;
}


/* The entry pane's list and sort menu step: the held list button walks the list rows, the menu button opens the
 * sort menus, and the confirm step sorts the box.  Returns 1 while it owns the frame. */
extern "C" s32 lb_entry_pane_menu_step(LbEntryPane* self, LbEquipBoxWork* owner, LbEquipPane* set)
{
    s32 kinds;
    s32 picked;
    u8 weapon;
    u16 count;
    u16 i;
    u16 j;
    u8 order;
    s16 rows;
    s16 id_a;
    s16 id_b;
    s16 id_c;
    LbEquipRec* rec;

    switch (self->sort_step_0x023) {
    case 0:
        if (lb_cmd_held_ck(0x80) == 1U) {
            if (self->held_0x020 == 0) {
                camera_lobby_manual_set(0);
                self->held_0x020 = 1;
                sysSE_req(5);
                lb_digit_pane_value(self);
                lb_entry_pane_list_reset(self, owner);
            } else if (lb_cmd_repeat_ck(3) != 0) {
                self->list_0x02C.cursor_0x00 = menu_cursor_step(self->list_0x02C.cursor_0x00,
                                                                self->list_0x02C.count_0x06, lb_cmd_repeat_get(), 1, 2);
            }
            lb_scroll_kind_set((struct LbScrollWork*)&self->scroll_0x04C, lb_entry_pane_scroll_kind_get(self));
            return 1;
        }
        if (self->held_0x020 == 1) {
            self->held_0x020 = 0;
            sysSE_req(1);
            if (lb_entry_pane_scroll_kind_get(self) == 7) {
                camera_lobby_manual_set(1);
            }
            return 1;
        }
        if (self->held_0x020 == 0) {
            u8 kind = lb_entry_pane_scroll_kind_get(self);

            if (owner != NULL && kind == 7) {
                lb_box_cam_input_step((LbMenuPanel*)owner, 1);
                return 1;
            }
            if (kind != 4 && kind != 10 && lb_cmd_pressed_ck(0x40) == 1U) {
                self->sort_step_0x023++;
                self->sort_sub_0x025 = 0;
                lb_list_init(&self->sort_lists_0x0CC[0], 4, 2, 0x24A, -1, 0, -1, 3, 0);
                sysSE_req(5);
            }
        }
        break;
    case 1:
        switch (lb_choice_step(&self->sort_lists_0x0CC[0])) {
        case 1:
            switch (self->sort_lists_0x0CC[0].cursor_0x00) {
            case 1:
                self->sort_step_0x023++;
                self->sort_sub_0x025 = 0;
                lb_list_init(&self->sort_lists_0x0CC[1], 3, 2, 0xFFFF, -1, 0, -1, 3, 0);
                sysSE_req(0);
                break;
            case 2:
                self->sort_step_0x023++;
                self->sort_sub_0x025 = 0;
                lb_list_init(&self->sort_lists_0x0CC[1], 3, 2, 0xFFFF, -1, 0, -1, 3, 0);
                sysSE_req(0);
                break;
            case 3:
                self->sort_step_0x023++;
                self->sort_sub_0x025 = 0;
                lb_list_init(&self->sort_lists_0x0CC[1], 2, 2, 0xFFFF, -1, 0, -1, 3, 0);
                sysSE_req(0);
                break;
            case 0:
                lb_entry_pane_sort_apply(self, 0);
                break;
            }
            break;
        case 2:
            self->sort_step_0x023--;
            break;
        }
        return 1;
    case 2:
        switch (self->sort_lists_0x0CC[0].cursor_0x00) {
        case 1:
            switch (self->sort_sub_0x025) {
            case 0:
                switch (lb_choice_step(&self->sort_lists_0x0CC[1])) {
                case 1:
                    switch (self->sort_lists_0x0CC[1].cursor_0x00) {
                    case 1:
                        self->sort_sub_0x025++;
                        rows = 7;
                        if (userdata_progress_flag_ck(0xE) == 0) {
                            rows = 6;
                        }
                        if (userdata_progress_flag_ck(0xF) == 0) {
                            rows--;
                        }
                        lb_list_init(&self->sort_lists_0x0CC[2], rows, 6, 0xFFFF, -1, 0, -1, 3, 0);
                        sysSE_req(5);
                        break;
                    case 2:
                        self->sort_sub_0x025++;
                        lb_list_init(&self->sort_lists_0x0CC[2], 4, 6, 0xFFFF, -1, 0, -1, 3, 0);
                        sysSE_req(5);
                        break;
                    default:
                        self->sort_step_0x023++;
                        self->sort_sub_0x025 = 0;
                        lb_list_init(&self->sort_lists_0x0CC[3], 4, 2, 0xFFFF, -1, 0, -1, 3, 0);
                        sysSE_req(0);
                        break;
                    }
                    break;
                case 2:
                    self->sort_step_0x023--;
                    break;
                }
                break;
            case 1:
                switch (lb_choice_step(&self->sort_lists_0x0CC[2])) {
                case 1:
                    self->sort_step_0x023++;
                    self->sort_sub_0x025 = 0;
                    if (self->sort_lists_0x0CC[1].cursor_0x00 != 2) {
                        lb_list_init(&self->sort_lists_0x0CC[3], 4, 2, 0xFFFF, -1, 0, -1, 3, 0);
                    } else {
                        lb_list_init(&self->sort_lists_0x0CC[3], 2, 2, 0xFFFF, -1, 0, -1, 3, 0);
                    }
                    sysSE_req(0);
                    break;
                case 2:
                    self->sort_sub_0x025--;
                    break;
                }
                break;
            }
            break;
        case 2:
            switch (self->sort_sub_0x025) {
            case 0:
                switch (lb_choice_step(&self->sort_lists_0x0CC[1])) {
                case 1:
                    self->sort_sub_0x025++;
                    lb_list_init(&self->sort_lists_0x0CC[2], 6, 6, 0xFFFF, -1, 0, -1, 3, 0);
                    sysSE_req(5);
                    break;
                case 2:
                    self->sort_step_0x023--;
                    break;
                }
                break;
            case 1:
                switch (lb_choice_step(&self->sort_lists_0x0CC[2])) {
                case 1:
                    self->sort_step_0x023++;
                    self->sort_sub_0x025 = 0;
                    lb_list_init(&self->sort_lists_0x0CC[3], 3, 2, 0xFFFF, -1, 0, -1, 3, 0);
                    sysSE_req(0);
                    break;
                case 2:
                    self->sort_sub_0x025--;
                    break;
                }
                break;
            }
            break;
        case 3:
            switch (lb_choice_step(&self->sort_lists_0x0CC[1])) {
            case 1:
                switch (self->sort_lists_0x0CC[1].cursor_0x00) {
                case 0:
                    lb_entry_pane_sort_apply(self, 1);
                    break;
                case 1:
                    lb_entry_pane_sort_apply(self, 0x11);
                    break;
                }
                break;
            case 2:
                self->sort_step_0x023--;
                break;
            }
            break;
        }
        return 1;
    case 3:
        switch (self->sort_lists_0x0CC[0].cursor_0x00) {
        case 1:
            switch (self->sort_sub_0x025) {
            case 0:
                switch (lb_choice_step(&self->sort_lists_0x0CC[3])) {
                case 1:
                    switch (self->sort_lists_0x0CC[3].cursor_0x00) {
                    case 0:
                        lb_entry_pane_sort_apply(self, 0);
                        break;
                    case 1:
                        lb_entry_pane_sort_apply(self, 2);
                        break;
                    case 2:
                        self->sort_sub_0x025++;
                        lb_list_init(&self->sort_lists_0x0CC[4], 5, 6, 0xFFFF, -1, 0, -1, 3, 0);
                        sysSE_req(5);
                        break;
                    case 3:
                        self->sort_sub_0x025++;
                        lb_list_init(&self->sort_lists_0x0CC[4], 3, 6, 0xFFFF, -1, 0, -1, 3, 0);
                        sysSE_req(5);
                        break;
                    }
                    break;
                case 2:
                    self->sort_step_0x023--;
                    break;
                }
                break;
            case 1:
                switch (lb_choice_step(&self->sort_lists_0x0CC[4])) {
                case 1:
                    switch (self->sort_lists_0x0CC[3].cursor_0x00) {
                    case 2:
                        switch (self->sort_lists_0x0CC[4].cursor_0x00) {
                        case 0:
                            lb_entry_pane_sort_apply(self, 3);
                            break;
                        case 1:
                            lb_entry_pane_sort_apply(self, 4);
                            break;
                        case 2:
                            lb_entry_pane_sort_apply(self, 5);
                            break;
                        case 3:
                            lb_entry_pane_sort_apply(self, 6);
                            break;
                        case 4:
                            lb_entry_pane_sort_apply(self, 7);
                            break;
                        }
                        break;
                    case 3:
                        switch (self->sort_lists_0x0CC[4].cursor_0x00) {
                        case 0:
                            lb_entry_pane_sort_apply(self, 0xA);
                            break;
                        case 1:
                            lb_entry_pane_sort_apply(self, 8);
                            break;
                        case 2:
                            lb_entry_pane_sort_apply(self, 9);
                            break;
                        }
                        break;
                    }
                    break;
                case 2:
                    self->sort_sub_0x025--;
                    break;
                }
                break;
            }
            break;
        case 2:
            switch (self->sort_sub_0x025) {
            case 0:
                switch (lb_choice_step(&self->sort_lists_0x0CC[3])) {
                case 1:
                    switch (self->sort_lists_0x0CC[3].cursor_0x00) {
                    case 0:
                        lb_entry_pane_sort_apply(self, 0);
                        break;
                    case 1:
                        lb_entry_pane_sort_apply(self, 0xB);
                        break;
                    case 2:
                        self->sort_sub_0x025++;
                        lb_list_init(&self->sort_lists_0x0CC[4], 5, 6, 0xFFFF, -1, 0, -1, 3, 0);
                        sysSE_req(5);
                        break;
                    }
                    break;
                case 2:
                    self->sort_step_0x023--;
                    break;
                }
                break;
            case 1:
                switch (lb_choice_step(&self->sort_lists_0x0CC[4])) {
                case 1:
                    switch (self->sort_lists_0x0CC[4].cursor_0x00) {
                    case 0:
                        lb_entry_pane_sort_apply(self, 0xC);
                        break;
                    case 1:
                        lb_entry_pane_sort_apply(self, 0xD);
                        break;
                    case 2:
                        lb_entry_pane_sort_apply(self, 0xE);
                        break;
                    case 3:
                        lb_entry_pane_sort_apply(self, 0xF);
                        break;
                    case 4:
                        lb_entry_pane_sort_apply(self, 0x10);
                        break;
                    }
                    break;
                case 2:
                    self->sort_sub_0x025--;
                    break;
                }
                break;
            }
            break;
        }
        return 1;
    case 0xFF:
        switch (toggle_word_step(&self->stepper_0x004, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            self->sort_step_0x023 = 0;
            sysSE_req(0);
            order = lb_entry_pane_sort_class_get(self, &kinds, &picked, &weapon);
            count = userdata_box_capacity(lobby_world_block);
            rec = &lobby_world_block->equip_0x0E00[0];
            for (i = 0; i < count; i++, rec++) {
                j = (u16)lb_equip_row_next(order, -1, rec, count - i, kinds, picked, weapon) + i;
                for (; j > i; j--) {
                    lb_equip_record_swap(j - 1, j, set);
                }
            }
            self->digit_ones_0x00A = 0;
            self->digit_tens_0x00C = 0;
            self->digit_hundreds_0x002 = 0;
            if (set != NULL) {
                id_a = -1;
                if (set->ids_0x04[0] != 0xFFFF) {
                    id_a = set->ids_0x04[0];
                }
                id_b = -1;
                if (set->ids_0x04[1] != 0xFFFF) {
                    id_b = set->ids_0x04[1];
                }
                id_c = -1;
                if (set->ids_0x04[2] != 0xFFFF) {
                    id_c = set->ids_0x04[2];
                }
                lb_list_pane_pos_set(self, id_a, id_b, id_c);
            } else {
                lb_list_pane_pos_set(self, 0, -1, -1);
            }
            break;
        case 2:
            self->sort_step_0x023 = self->sort_prev_0x024;
            break;
        }
        return 1;
    }
    return 0;
}

/* Ticks the two blink timers and the pane's frame counter. */
extern "C" void lb_entry_pane_blink_tick(LbEntryPane* self)
{
    if (self->blink_a_0x012 >= 0) {
        self->blink_a_0x012++;
        if (self->blink_a_0x012 > 6) {
            self->blink_a_0x012 = -1;
            self->timer_0x010 = 0;
        }
    }
    if (self->blink_b_0x014 >= 0) {
        self->blink_b_0x014++;
        if (self->blink_b_0x014 > 6) {
            self->blink_b_0x014 = -1;
            self->timer_0x010 = 0;
        }
    }
    self->timer_0x010++;
    if (self->timer_0x010 > 0x1E) {
        self->timer_0x010 = 0;
    }
}

/* Starts the pane's frame: the digit animation, the blink timers, and the pad flag words cleared. */
extern "C" void lb_entry_pane_frame_begin(LbEntryPane* self)
{
    fn_801EC804(&self->anim_0x00E);
    lb_entry_pane_blink_tick(self);
    self->flags_0x008 = 0;
    self->scroll_0x04C.moved_0x06 = 0;
}

/* Swaps two box records, keeping the worn slots, the saved sets and the set pane's ids pointing at the same
 * equipment. */
extern "C" void lb_equip_record_swap(u16 id_a, u16 id_b, LbEquipPane* set)
{
    LbEquipRec tmp;
    LbItemDb* db;
    s32 i;
    s8 class_change;
    u8 worn_a;
    u8 worn_b;

    db = lobby_world_block;
    class_change = 0;
    worn_a = userdata_equip_box_worn_ck(db, id_a) == 1;
    worn_b = userdata_equip_box_worn_ck(db, id_b) == 1;
    equip_record_copy(&tmp, &lobby_world_block->equip_0x0E00[id_a]);
    equip_record_copy(&lobby_world_block->equip_0x0E00[id_a], &lobby_world_block->equip_0x0E00[id_b]);
    equip_record_copy(&lobby_world_block->equip_0x0E00[id_b], &tmp);
    if (worn_a == 1) {
        equip_record_wear(lobby_world_block, id_b, &class_change);
    }
    if (worn_b == 1) {
        equip_record_wear(lobby_world_block, id_a, &class_change);
    }
    my_set_equip_id_swap(db, id_a, id_b);
    my_set_gun_id_swap(db, id_a, id_b);
    if (set != NULL) {
        for (i = 0; i < 3; i++) {
            if (id_a == set->ids_0x04[i]) {
                set->ids_0x04[i] = id_b;
            } else if (id_b == set->ids_0x04[i]) {
                set->ids_0x04[i] = id_a;
            }
        }
    }
}

/* The entry pane's pick step: 1 when the record under the digits fits the slot kind, 2 on cancel. */
extern "C" s32 lb_entry_pane_select_step(LbEntryPane* self, LbEquipBoxWork* owner, LbEquipPane* set)
{
    LbEquipRec* rec;
    s32 result;
    s16 index;

    result = 0;
    lb_entry_pane_frame_begin(self);
    if (lb_entry_pane_menu_step(self, owner, set) == 0) {
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            index = lb_digit_pane_value(self);
            rec = &lobby_world_block->equip_0x0E00[index];
            switch (self->slot_kind_0x016) {
            case 0:
                result = 1;
                break;
            case 1:
                if (rec->kind_0x00 == 0xB) {
                    result = 1;
                    sysSE_stop(0x2B);
                } else {
                    sysSE_req(2);
                }
                break;
            case 2:
                if (rec->kind_0x00 == 0xC) {
                    result = 1;
                    sysSE_stop(0x2B);
                } else {
                    sysSE_req(2);
                }
                break;
            case 3:
                if (rec->kind_0x00 == 0xD) {
                    result = 1;
                    sysSE_stop(0x2B);
                } else {
                    sysSE_req(2);
                }
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else {
            lb_entry_pane_input(self);
        }
    }
    return result;
}

/* The equipment box's wear step: the entry pane picks a record and it is worn (a weapon opens the set pane
 * first). */
extern "C" s32 lb_equip_box_wear_step(LbEquipBoxWork* self)
{
    LbEntryPane* entry;
    LbEquipRec* rec;
    s32 result;
    u16 id;
    s32 i;

    result = 0;
    entry = &self->entry_0x074;
    switch (entry->phase_0x000) {
    case 0:
        switch (lb_entry_pane_select_step(entry, self, NULL)) {
        case 2:
            result = 2;
            break;
        case 1:
            id = lb_digit_pane_value(entry);
            rec = &lobby_world_block->equip_0x0E00[id];
            switch (rec->kind_0x00) {
            case 11:
                entry->phase_0x000++;
                sysSE_stop(0x2B);
                lb_equip_set_pane_init(&self->set_0x2A8, 0, id, 0xFFFF, 0xFFFF, 0, entry->page_0x028);
                break;
            case 12:
                entry->phase_0x000++;
                sysSE_stop(0x2B);
                lb_equip_set_pane_init(&self->set_0x2A8, 1, 0xFFFF, id, 0xFFFF, 0, entry->page_0x028);
                break;
            case 13:
                entry->phase_0x000++;
                sysSE_stop(0x2B);
                lb_equip_set_pane_init(&self->set_0x2A8, 2, 0xFFFF, 0xFFFF, id, 0, entry->page_0x028);
                break;
            default:
                switch ((u8)equip_kind_table_class(rec->kind_0x00)) {
                case 1:
                    if (userdata_equip_box_worn_ck(lobby_world_block, id) == 0) {
                        entry->phase_0x000 = 2;
                        sysSE_stop(3);
                        lb_equip_entry_apply(id);
                        userdata_pl_look_apply(self->page_0x010, lobby_world_block);
                        fn_801EC7AC((LbMenuPanel*)self);
                        lb_entry_pane_list_reset(entry, self);
                        self->changed_0x004 = 1;
                    } else {
                        sysSE_req(2);
                    }
                    break;
                case 0:
                    if (equip_wearable_ck(lobby_world_block, (_EQUIP*)rec) == 1U) {
                        entry->phase_0x000 = 2;
                        sysSE_stop(3);
                        if (userdata_equip_box_worn_ck(lobby_world_block, id) == 1U) {
                            set_slot_none(lobby_world_block, rec->kind_0x00);
                        } else {
                            lb_equip_entry_apply(id);
                        }
                        userdata_pl_look_apply(self->page_0x010, lobby_world_block);
                        fn_801EC7AC((LbMenuPanel*)self);
                        lb_entry_pane_list_reset(entry, self);
                    } else {
                        sysSE_req(2);
                    }
                    break;
                default:
                    sysSE_req(2);
                    break;
                }
                break;
            case 0:
                sysSE_req(2);
                break;
            }
            break;
        }
        break;
    case 1:
        switch (lb_equip_set_pane_step(&self->set_0x2A8, entry, self)) {
        case 2:
            entry->phase_0x000 = 0;
            lb_entry_pane_kind_set(entry, 0, 1);
            break;
        case 1:
            entry->phase_0x000++;
            lb_entry_pane_kind_set(entry, 0, 1);
            for (i = 0; i < 3; i++) {
                if (self->set_0x2A8.ids_0x04[i] != 0xFFFF) {
                    lb_equip_entry_apply(self->set_0x2A8.ids_0x04[i]);
                }
            }
            userdata_pl_look_apply(self->page_0x010, lobby_world_block);
            fn_801EC7AC((LbMenuPanel*)self);
            lb_entry_pane_list_reset(entry, self);
            self->changed_0x004 = 1;
            break;
        }
        break;
    case 2:
        if (lb_page_record_empty_ck(entry->page_0x028) == 1U && lb_cmd_pressed_ck(0x30) == 1U) {
            entry->phase_0x000 = 0;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* The entry pane's move step: the first pick marks a record, the second swaps the two. */
extern "C" s32 lb_entry_pane_move_step(LbEntryPane* self, LbEquipBoxWork* owner)
{
    s32 result;
    s16 id;

    result = 0;
    lb_entry_pane_frame_begin(self);
    switch (self->phase_0x000) {
    case 0:
        if (lb_entry_pane_menu_step(self, owner, NULL) == 1U) {
            return 0;
        }
    case 1:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            if (lb_entry_pane_scroll_kind_get(self) != 4 && lb_entry_pane_scroll_kind_get(self) != 10) {
                id = lb_digit_pane_value(self);
                switch (self->phase_0x000) {
                case 0:
                    self->phase_0x000++;
                    self->mark_0x01E = id;
                    sysSE_req(0xE);
                    break;
                case 1:
                    if ((lobby_world_block->equip_0x0E00[id].kind_0x00 == 0
                         && lobby_world_block->equip_0x0E00[self->mark_0x01E].kind_0x00 == 0)
                        || id == self->mark_0x01E) {
                        sysSE_req(2);
                    } else {
                        self->phase_0x000--;
                        lb_equip_record_swap(id, self->mark_0x01E, NULL);
                        sysSE_req(0xF);
                    }
                    break;
                }
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            switch (self->phase_0x000) {
            case 0:
                sysSE_req(1);
                result = 2;
                break;
            case 1:
                self->phase_0x000--;
                sysSE_req(1);
                break;
            }
        } else {
            lb_entry_pane_input(self);
        }
    }
    return result;
}

/* The entry pane's sell step: picks an unworn record, confirms, then sells it for its price. */
extern "C" s32 lb_entry_pane_sell_step(LbEntryPane* self, LbEquipBoxWork* owner)
{
    LbItemDb* db;
    s32 result;

    result = 0;
    lb_entry_pane_frame_begin(self);
    switch (self->phase_0x000) {
    case 0:
        if (lb_entry_pane_menu_step(self, owner, NULL) == 0) {
            if (lb_cmd_pressed_ck(0x10) == 1U) {
                self->mark_0x01E = lb_digit_pane_value(self);
                if ((u8)equip_kind_table_class(lobby_world_block->equip_0x0E00[self->mark_0x01E].kind_0x00) != 0xFF
                    && userdata_equip_box_worn_ck(lobby_world_block, self->mark_0x01E) == 0) {
                    self->phase_0x000++;
                    self->stepper_0x004 = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
            } else if (lb_cmd_pressed_ck(0x20) == 1U) {
                sysSE_req(1);
                result = 2;
            } else {
                lb_entry_pane_input(self);
            }
        }
        break;
    case 1:
        switch (toggle_word_step(&self->stepper_0x004, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            self->phase_0x000 = 0;
            db = lobby_world_block;
            score_add_clamped(equip_sell_price_get((_EQUIP*)&db->equip_0x0E00[self->mark_0x01E]), &db->data_0x18[0]);
            sysSE_req(9);
            equip_record_discard(lobby_world_block, self->mark_0x01E);
            lb_entry_pane_kind_set(self, self->slot_kind_0x016, 0);
            break;
        case 2:
            self->phase_0x000--;
            break;
        }
        break;
    }
    return result;
}


/* Seeds the my-set pane for list mode `mode` and kind `kind` on page record `page`. */
extern "C" void lb_my_set_pane_init(LbMySetPane* self, u8 mode, u8 kind, struct LbPage* page)
{
    self->phase_0x00 = 0;
    self->sub_0x01 = 0;
    self->index_0x04 = 0;
    self->mode_0x02 = mode;
    self->count_0x05 = 20;
    self->kind_0x03 = kind;
    self->moved_0x08 = 0;
    self->blink_0x0C = 0;
    self->stepper_0x10 = 0;
    self->page_0x14 = page;
    self->row_0x07 = 0;
    self->row_moved_0x0A = 0;
}

/* The detail rows of the set under the cursor: 2 for a bowgun set, 1 otherwise, 3 for a gunner set. */
extern "C" s32 lb_my_set_pane_rows_get(LbMySetPane* self)
{
    if (self->mode_0x02 == 0) {
        return (lobby_world_block->equip_0x0E00[lobby_world_block->my_sets_0x3380[self->index_0x04].weapon_0x00]
                    .kind_0x00 == 0xB) + 1;
    }
    return 3;
}

/* Keeps the detail row inside the current set's rows. */
extern "C" void lb_my_set_pane_row_clamp(LbMySetPane* self)
{
    if (self->row_0x07 >= (u8)lb_my_set_pane_rows_get(self)) {
        self->row_0x07 = 0;
    }
}

/* The equipment-set load step: picks a stored set, confirms, then wears every record it names. */
extern "C" s32 lb_my_set_load_step(LbEquipBoxWork* owner, LbMySetPane* self)
{
    LbMySet* set;
    s32 result;
    s32 i;

    result = 0;
    self->moved_0x08 = 0;
    self->row_moved_0x0A = 0;
    switch (self->phase_0x00) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            if (lobby_world_block->my_sets_0x3380[self->index_0x04].weapon_0x00 != 0xFFFF) {
                self->phase_0x00++;
                self->stepper_0x10 = 0;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
            lb_my_set_pane_row_clamp(self);
        } else if (lobby_cmd_release_ck(0xC00) != 0) {
            u16 keys = lobby_cmd_release_get();

            self->row_0x07 = menu_cursor_step_fixed_tail(self->row_0x07, (u8)lb_my_set_pane_rows_get(self), keys, 0x800,
                                                         0x400, &self->row_moved_0x0A);
        }
        break;
    case 1:
        switch (toggle_word_step(&self->stepper_0x10, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            self->phase_0x00++;
            set = &lobby_world_block->my_sets_0x3380[self->index_0x04];
            lb_equip_entry_apply(set->weapon_0x00);
            for (i = 0; i < 2; i++) {
                if (set->opts_0x02.ids[i] != 0xFFFF) {
                    lb_equip_entry_apply(set->opts_0x02.ids[i]);
                }
            }
            if (set->armor_0x06[3] != 0xFFFF) {
                lb_equip_entry_apply(set->armor_0x06[3]);
            } else {
                set_slot_none(lobby_world_block, 4);
            }
            if (set->armor_0x06[4] != 0xFFFF) {
                lb_equip_entry_apply(set->armor_0x06[4]);
            } else {
                set_slot_none(lobby_world_block, 5);
            }
            if (set->armor_0x06[0] != 0xFFFF) {
                lb_equip_entry_apply(set->armor_0x06[0]);
            } else {
                set_slot_none(lobby_world_block, 1);
            }
            if (set->armor_0x06[1] != 0xFFFF) {
                lb_equip_entry_apply(set->armor_0x06[1]);
            } else {
                set_slot_none(lobby_world_block, 2);
            }
            if (set->armor_0x06[2] != 0xFFFF) {
                lb_equip_entry_apply(set->armor_0x06[2]);
            } else {
                set_slot_none(lobby_world_block, 3);
            }
            if (set->armor_0x06[5] != 0xFFFF) {
                lb_equip_entry_apply(set->armor_0x06[5]);
            } else {
                set_slot_none(lobby_world_block, 6);
            }
            equip_worn_records_refresh(self->page_0x14, lobby_world_block);
            fn_801EC7AC((LbMenuPanel*)owner);
            owner->changed_0x004 = 1;
            sysSE_stop(3);
            break;
        case 2:
            self->phase_0x00--;
            break;
        }
        break;
    case 2:
        if (lb_page_record_empty_ck(self->page_0x14) == 1U && lb_cmd_pressed_ck(0x30) == 1U) {
            result = 1;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* The equipment-set save step: picks a slot, confirms, then stores the worn equipment in it. */
extern "C" s32 lb_my_set_save_step(LbMySetPane* self)
{
    s32 result;

    result = 0;
    self->moved_0x08 = 0;
    self->row_moved_0x0A = 0;
    switch (self->phase_0x00) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            self->phase_0x00++;
            self->stepper_0x10 = 0;
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
            lb_my_set_pane_row_clamp(self);
        } else if (lobby_cmd_release_ck(0xC00) != 0) {
            u16 keys = lobby_cmd_release_get();

            self->row_0x07 = menu_cursor_step_fixed_tail(self->row_0x07, (u8)lb_my_set_pane_rows_get(self), keys, 0x800,
                                                         0x400, &self->row_moved_0x0A);
        }
        break;
    case 1:
        switch (toggle_word_step(&self->stepper_0x10, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            my_set_store(lobby_world_block, self->index_0x04);
            lb_my_set_pane_row_clamp(self);
            sysSE_req(0x17);
            result = 1;
            break;
        case 2:
            self->phase_0x00--;
            break;
        }
        break;
    }
    return result;
}

/* The equipment-set move step: the first pick marks a set, the second swaps the two. */
extern "C" s32 lb_my_set_move_step(LbMySetPane* self)
{
    LbMySet tmp;
    s32 result;

    result = 0;
    self->moved_0x08 = 0;
    self->row_moved_0x0A = 0;
    if (++self->blink_0x0C >= 20) {
        self->blink_0x0C = 0;
    }
    switch (self->phase_0x00) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            self->phase_0x00++;
            self->mark_0x06 = self->index_0x04;
            self->blink_0x0C = 0;
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            if (self->mark_0x06 != self->index_0x04) {
                self->phase_0x00--;
                lb_my_set_copy(&tmp, &lobby_world_block->my_sets_0x3380[self->mark_0x06]);
                lb_my_set_copy(&lobby_world_block->my_sets_0x3380[self->mark_0x06],
                               &lobby_world_block->my_sets_0x3380[self->index_0x04]);
                lb_my_set_copy(&lobby_world_block->my_sets_0x3380[self->index_0x04], &tmp);
                lb_my_set_pane_row_clamp(self);
                sysSE_req(0x17);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            self->phase_0x00--;
            self->index_0x04 = self->mark_0x06;
            lb_my_set_pane_row_clamp(self);
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
        }
        break;
    }
    return result;
}

#pragma push
#pragma dont_inline off
/* Copies a set's option ids and name; never called, it makes the compiler generate both blocks' assignments before
 * the set copies below use them, so they inline. */
inline void lb_set_parts_copy(LbSetOpts* dst_opts, const LbSetOpts* src_opts, LbSetName* dst_name, const LbSetName* src_name)
{
    *dst_opts = *src_opts;
    *dst_name = *src_name;
}
/* Copies one stored equipment set. */
extern "C" void lb_my_set_copy(LbMySet* dst, LbMySet* src)
{
    dst->weapon_0x00 = src->weapon_0x00;
    dst->opts_0x02 = src->opts_0x02;
    dst->armor_0x06[0] = src->armor_0x06[0];
    dst->armor_0x06[1] = src->armor_0x06[1];
    dst->armor_0x06[2] = src->armor_0x06[2];
    dst->armor_0x06[3] = src->armor_0x06[3];
    dst->armor_0x06[4] = src->armor_0x06[4];
    dst->armor_0x06[5] = src->armor_0x06[5];
    dst->name_0x12 = src->name_0x12;
}
#pragma pop

/* The equipment-set delete step: picks a stored set, asks, then clears it. */
extern "C" s32 lb_my_set_delete_step(LbMySetPane* self)
{
    s32 result;

    result = 0;
    self->moved_0x08 = 0;
    self->row_moved_0x0A = 0;
    switch (self->phase_0x00) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            if (my_set_used_ck(lobby_world_block, self->index_0x04) == 1U) {
                self->phase_0x00++;
                self->stepper_0x10 = 0;
                sysSE_req(0x14);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
            lb_my_set_pane_row_clamp(self);
        } else if (lobby_cmd_release_ck(0xC00) != 0) {
            u16 keys = lobby_cmd_release_get();

            self->row_0x07 = menu_cursor_step_fixed_tail(self->row_0x07, (u8)lb_my_set_pane_rows_get(self), keys, 0x800,
                                                         0x400, &self->row_moved_0x0A);
        }
        break;
    case 1:
        switch (lb_yes_no_step(&self->stepper_0x10)) {
        case 1:
            my_set_clear(lobby_world_block, self->index_0x04);
            lb_my_set_pane_row_clamp(self);
            result = 1;
            break;
        case 2:
            self->phase_0x00--;
            break;
        }
        break;
    }
    return result;
}

/* The gunner-set load step: picks a stored set, confirms, then wears the records it names. */
extern "C" s32 lb_gun_set_load_step(LbEquipBoxWork* owner, LbMySetPane* self)
{
    LbGunSet* set;
    s32 result;
    s32 i;

    result = 0;
    self->moved_0x08 = 0;
    self->row_moved_0x0A = 0;
    switch (self->phase_0x00) {
    case 0:
        set = &lobby_world_block->gun_sets_0x36C8[self->index_0x04];
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            if (set->weapon_0x00 != 0xFFFF) {
                self->phase_0x00++;
                self->stepper_0x10 = 0;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
        } else if (lobby_cmd_release_ck(0xC00) != 0 && set->weapon_0x00 != 0xFFFF) {
            u16 keys = lobby_cmd_release_get();

            self->row_0x07 = menu_cursor_step_fixed_tail(self->row_0x07, (u8)lb_my_set_pane_rows_get(self), keys, 0x800,
                                                         0x400, &self->row_moved_0x0A);
        }
        break;
    case 1:
        switch (toggle_word_step(&self->stepper_0x10, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            self->phase_0x00++;
            set = &lobby_world_block->gun_sets_0x36C8[self->index_0x04];
            lb_equip_entry_apply(set->weapon_0x00);
            for (i = 0; i < 2; i++) {
                if (set->opts_0x02.ids[i] != 0xFFFF) {
                    lb_equip_entry_apply(set->opts_0x02.ids[i]);
                }
            }
            userdata_pl_look_apply(self->page_0x14, lobby_world_block);
            fn_801EC7AC((LbMenuPanel*)owner);
            owner->changed_0x004 = 1;
            sysSE_stop(3);
            break;
        case 2:
            self->phase_0x00--;
            break;
        }
        break;
    case 2:
        if (lb_page_record_empty_ck(self->page_0x14) == 1U && lb_cmd_pressed_ck(0x30) == 1U) {
            result = 1;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* The gunner-set save step: picks a slot, confirms, then stores the worn bowgun set in it. */
extern "C" s32 lb_gun_set_save_step(LbMySetPane* self)
{
    LbGunSet* set;
    s32 result;

    result = 0;
    self->moved_0x08 = 0;
    self->row_moved_0x0A = 0;
    switch (self->phase_0x00) {
    case 0:
        set = &lobby_world_block->gun_sets_0x36C8[self->index_0x04];
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            self->phase_0x00++;
            self->stepper_0x10 = 0;
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
        } else if (lobby_cmd_release_ck(0xC00) != 0 && set->weapon_0x00 != 0xFFFF) {
            u16 keys = lobby_cmd_release_get();

            self->row_0x07 = menu_cursor_step_fixed_tail(self->row_0x07, (u8)lb_my_set_pane_rows_get(self), keys, 0x800,
                                                         0x400, &self->row_moved_0x0A);
        }
        break;
    case 1:
        switch (toggle_word_step(&self->stepper_0x10, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            gun_set_store(lobby_world_block, self->index_0x04);
            sysSE_req(0x17);
            result = 1;
            break;
        case 2:
            self->phase_0x00--;
            break;
        }
        break;
    }
    return result;
}

/* The gunner-set move step: the first pick marks a set, the second swaps the two. */
extern "C" s32 lb_gun_set_move_step(LbMySetPane* self)
{
    LbGunSet tmp;
    s32 result;

    result = 0;
    self->moved_0x08 = 0;
    self->row_moved_0x0A = 0;
    if (++self->blink_0x0C >= 20) {
        self->blink_0x0C = 0;
    }
    switch (self->phase_0x00) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            self->phase_0x00++;
            self->mark_0x06 = self->index_0x04;
            self->blink_0x0C = 0;
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            if (self->mark_0x06 != self->index_0x04) {
                self->phase_0x00--;
                lb_gun_set_copy(&tmp, &lobby_world_block->gun_sets_0x36C8[self->mark_0x06]);
                lb_gun_set_copy(&lobby_world_block->gun_sets_0x36C8[self->mark_0x06],
                                &lobby_world_block->gun_sets_0x36C8[self->index_0x04]);
                lb_gun_set_copy(&lobby_world_block->gun_sets_0x36C8[self->index_0x04], &tmp);
                sysSE_req(0x17);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            self->phase_0x00--;
            self->index_0x04 = self->mark_0x06;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
        }
        break;
    }
    return result;
}

#pragma push
#pragma dont_inline off
/* Copies one stored gunner set. */
extern "C" void lb_gun_set_copy(LbGunSet* dst, LbGunSet* src)
{
    dst->weapon_0x00 = src->weapon_0x00;
    dst->opts_0x02 = src->opts_0x02;
    dst->name_0x06 = src->name_0x06;
}
#pragma pop

/* The gunner-set delete step: picks a stored set, asks, then clears it. */
extern "C" s32 lb_gun_set_delete_step(LbMySetPane* self)
{
    LbGunSet* set;
    s32 result;

    result = 0;
    self->moved_0x08 = 0;
    self->row_moved_0x0A = 0;
    switch (self->phase_0x00) {
    case 0:
        set = &lobby_world_block->gun_sets_0x36C8[self->index_0x04];
        if (lb_cmd_pressed_ck(0x10) == 1U) {
            if (gun_set_used_ck(lobby_world_block, self->index_0x04) == 1U) {
                self->phase_0x00++;
                self->stepper_0x10 = 0;
                sysSE_req(0x14);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1U) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->index_0x04 = menu_cursor_step_fixed_tail(self->index_0x04, self->count_0x05, lb_cmd_repeat_get(), 4,
                                                           8, &self->moved_0x08);
        } else if (lobby_cmd_release_ck(0xC00) != 0 && set->weapon_0x00 != 0xFFFF) {
            u16 keys = lobby_cmd_release_get();

            self->row_0x07 = menu_cursor_step_fixed_tail(self->row_0x07, (u8)lb_my_set_pane_rows_get(self), keys, 0x800,
                                                         0x400, &self->row_moved_0x0A);
        }
        break;
    case 1:
        switch (lb_yes_no_step(&self->stepper_0x10)) {
        case 1:
            gun_set_clear(lobby_world_block, self->index_0x04);
            result = 1;
            break;
        case 2:
            self->phase_0x00--;
            break;
        }
        break;
    }
    return result;
}

/* Seeds the model pane and resets the shown player's equipment models (all five parts, or the gunner one). */
extern "C" void lb_model_pane_init(LbModelPane* self, struct _PLW* plw, u8 mode)
{
    LbEquipRec arg;

    self->phase_0x00 = 0;
    self->sub_0x01 = 0;
    self->mode_0x02 = mode;
    self->cursor_0x04 = 0;
    self->value_0x08 = 0;
    switch (mode) {
    default:
        self->part_0x06 = 4;
        memset(&arg, 0, sizeof(arg));
        arg.kind_0x00 = 1;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        arg.kind_0x00 = 2;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        arg.kind_0x00 = 3;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        arg.kind_0x00 = 4;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        arg.kind_0x00 = 5;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        break;
    case 1:
        self->part_0x06 = 11;
        memset(&arg, 0, sizeof(arg));
        arg.kind_0x00 = 5;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        break;
    }
    self->plw_0x0C = plw;
}

} /* namespace s_801EC9F8 */
#pragma dont_inline reset
#pragma peephole reset

/* The page/panel band (0x801F3294-0x801F9CD4). */
void subTransSet(u32 a, long b, u32* value);
/* The colour band's views of `lobby_world_block` (the pointer it is) and `lb_list_row_draw` (five arguments). */
namespace lb_chg {
extern "C" {
extern u8* lobby_world_block;
s32 lb_list_row_draw(s32 text, u8 flag, s16* pos, s32 color, u8 mode);
}
}  // namespace lb_chg
#include "unsplit/lobby.h"
#include "hud/spr_data.h"
/* C++-linkage callees of this section that `unsplit/lobby.h` does not declare: they stay at global scope (their manglings are made there). */
void PutPageArrow(u16* table, s16 a, s16 b, u16 c, const _mh_ivec2_* pos, u8 flags);
void set_blendmode(u8 a, u8 b, u8 c);
#include "types.h"

#include "Runtime.PPCEABI.H/memset.h"
namespace s_801F3294 {
#define LOBBY_VIEW_IN_NAMESPACE 1
#include "lobby/lb_pane_ui.h"

/* The owner's views of two `lobby/fn_80212810.cpp` entries that `unsplit/lobby.h`, which this band includes, declares
 * differently: `lb_choice_draw` takes six arguments (`lobby/lb_panel_msg_draw.h`), `lb_help_num_put` a word value
 * (`lobby/lb_list_init.h`). */
#define lb_choice_draw_c1 ((void (*)(u8*, u16, const u16*, const u16*, u16, u8))lb_choice_draw)
#define lb_help_num_put_c1 ((s32 (*)(s32, s32, s32))lb_help_num_put)

/* The colour pane `lb_color_pane_init` seeds and `lb_color_pane_step` drives: the step, the done code, the inner (0)
 * or hair (1) mode, the changed flag, the grid cursor and its extent, the free cursor, the three colour words (the
 * world block's, the picked one, the original) and the player work it shows. */
typedef struct LbColorPane {
    /* +0x00 */ u8 phase_0x00;
    /* +0x01 */ u8 done_0x01;           /* 1 = left unchanged, 2 = decided */
    /* +0x02 */ u8 mode_0x02;           /* 0 = the inner wear colour, 1 = the hair colour */
    /* +0x03 */ u8 changed_0x03;
    /* +0x04 */ s16 column_0x04;
    /* +0x06 */ s16 row_0x06;
    /* +0x08 */ s16 widest_0x08;        /* `fn_801F353C`'s out_max over the row widths */
    /* +0x0A */ s16 rows_0x0A;          /* its out_count */
    /* +0x0C */ s16 anim_0x0C;          /* the value `fn_801EC804` animates */
    /* +0x0E */ s16 cursor_x_0x0E;      /* the free colour cursor */
    /* +0x10 */ s16 cursor_y_0x10;
    /* +0x12 */ u8 unused_0x12[2];
    /* +0x14 */ u32 color_0x14;         /* the world block's colour word when the pane opened */
    /* +0x18 */ u32 picked_0x18;        /* the colour `fn_801F37C8` packs from the picked row */
    /* +0x1C */ u32 original_0x1C;
    /* +0x20 */ s32 stepper_0x20;       /* the confirm stepper */
    /* +0x24 */ ::_PLW* plw_0x24;       /* the player work the colour is shown on */
} LbColorPane; /* size: 0x28 */

/* The dye pane's record (`fn_801F3DEC` initialises it; the dye steps read it as an `LbPage`).  size: 0x2C */
typedef struct LbDyePane {
    /* +0x00 */ u8 record_0x00[0x2C];
} LbDyePane;

/* The box menu work (`lobby_w.menu_0xAC` while the item/equipment box is open): the menu state and the top list's
 * row and cursor, the player work the panes show, and every pane by value.  `LbListPane`, `LbEquipBoxWork` and
 * `LbPageOwner` are the other views of the same object. */
typedef struct LbBoxMenu {
    /* +0x000 */ u8 state_0x000;        /* 0 = opening, 1 = the top list, 2..25 = a pane */
    /* +0x001 */ u8 sub_0x001;
    /* +0x002 */ u8 list_kind_0x002;    /* 0 while the menu is on its first page */
    /* +0x003 */ u8 leave_0x003;        /* 1 asks the menu to close */
    /* +0x004 */ u8 changed_0x004;      /* set once the worn equipment changed */
    /* +0x005 */ u8 side_0x005;         /* which pouch (0/1) the item lists show first */
    /* +0x006 */ s16 menu_row_0x006;    /* the top list's row */
    /* +0x008 */ s16 menu_cursor_0x008; /* the top list's second-page cursor */
    /* +0x00A */ s16 menu_page_0x00A;   /* 0 = the category rows, 1 = the open category's rows */
    /* +0x00C */ s16 menu_rows_0x00C;   /* the category row count */
    /* +0x00E */ s16 menu_subrows_0x00E; /* the open category's row count */
    /* +0x010 */ LbPage* page_0x010;    /* the player work the panes show */
    /* +0x014 */ u8 unused_0x014[8];
    /* +0x01C */ u16 moved_0x01C;       /* the pad-driven moved bits, cleared every frame */
    /* +0x01E */ s16 list_0x01E;        /* the item lists' amount bound (`LbListPane::move_max_0x01E`) */
    /* +0x020 */ s16 list_0x020;        /* the amount picked */
    /* +0x022 */ s16 list_0x022;        /* the slot picked */
    /* +0x024 */ u16 item_id_0x024;     /* the item picked */
    /* +0x026 */ u8 unused_0x026[0xA];
    /* +0x030 */ MenuSelCursor cursor_0x030;
    /* +0x054 */ LbDigitPane digits_0x054;
    /* +0x074 */ LbEntryPane entry_0x074;
    /* +0x2A8 */ LbEquipPane set_0x2A8;
    /* +0x33C */ LbDyePane dye_0x33C;
    /* +0x368 */ LbColorPane color_0x368;
    /* +0x390 */ LbModelPane model_0x390;
    /* +0x3A0 */ LbMySetPane my_set_0x3A0;
    /* +0x3B8 */ LbItemList items_0x3B8;
} LbBoxMenu; /* size: 0x54C */

extern "C" {
/* The panes band's entry points the box menu runs (defined above, in `s_801EC9F8`). */
s32 lb_box_cam_input_step(::LbMenuPanel* self, u8 flag);
s32 fn_801ECB40(void);
void fn_801ECD50(::LbDigitPane* self);
void fn_801ED284(::LbListPane* self);
s32 fn_801ED688(::LbListPane* self, u8 arg);
s32 fn_801EDC4C(::LbDigitPane* self, ::LbListPane* list);
s32 fn_801EE1E4(::LbListPane* self);
s32 lb_box_combine_step(::LbDigitPane* self, ::MenuSelCursor* cur, ::LbListPane* list);
s32 fn_801EEDC4(::LbDigitPane* self, ::LbListPane* list);
s32 fn_801EEFA0(::LbDigitPane* self, ::LbListPane* list);
void lb_list_pane_init(::LbEntryPane* self, s16 slot_kind, ::LbPage* page);
s32 lb_equip_box_wear_step(::LbEquipBoxWork* self);
s32 lb_entry_pane_move_step(::LbEntryPane* self, ::LbEquipBoxWork* owner);
s32 lb_entry_pane_sell_step(::LbEntryPane* self, ::LbEquipBoxWork* owner);
void lb_my_set_pane_init(::LbMySetPane* self, u8 mode, u8 kind, ::LbPage* page);
s32 lb_my_set_load_step(::LbEquipBoxWork* owner, ::LbMySetPane* self);
s32 lb_my_set_save_step(::LbMySetPane* self);
s32 lb_my_set_move_step(::LbMySetPane* self);
s32 lb_my_set_delete_step(::LbMySetPane* self);
s32 lb_gun_set_load_step(::LbEquipBoxWork* owner, ::LbMySetPane* self);
s32 lb_gun_set_save_step(::LbMySetPane* self);
s32 lb_gun_set_move_step(::LbMySetPane* self);
s32 lb_gun_set_delete_step(::LbMySetPane* self);
s32 lb_my_set_pane_rows_get(::LbMySetPane* self);
void lb_model_pane_init(::LbModelPane* self, ::_PLW* plw, u8 mode);
/* The colour band's draw pass, handed to the transition. */
void lb_box_menu_draw(void);
s16 fn_801ED53C(::LbListPane* self);
s16 fn_801ED554(::LbListPane* self);
s32 fn_801ED184(s32 index);
s32 fn_801ED200(s32 index, u8 side);
u8 fn_801EDA9C(::LbListPane* self, u16 index, u16 item_id);
s32 fn_801EDA34(s32 index, u8 side);
u8 fn_801EDB34(::LbListPane* self, u16 index, u16 item_id);
u16 userdata_equip_item_id_get(::LbItemDb* db, s32 index);
s16 userdata_pouch_size(u8 side);
s32 item_pair_index_find(u16 id, ::IdValue* base, s32 count);
::IdValue* userdata_pouch_get(::LbItemDb* db, u8 side);
u16 userdata_box_capacity(::LbItemDb* db);
u32 lb_entry_pane_scroll_kind_get(::LbEntryPane* self);
u8* lb_entry_pane_kind_tbl_get(::LbEntryPane* self);
s16 lb_digit_pane_value(::LbEntryPane* self);
void lb_entry_pane_scroll_refresh(::LbEntryPane* self);
u32 userdata_equip_box_worn_ck(::LbItemDb* db, u16 id);
s16 fn_801ED160(::LbDigitPane* self);
s32 fn_801ED9E4(s32 index);
s32 userdata_item_slot_count(::LbItemDb* db);
::IdValue* userdata_equip_item_slots_get(::LbItemDb* db);
u32 fn_801EE900(u16 item_id);
s32 fn_801EE940(u16 index, u16 item_id);
s32 eft052_item_value_get(u16 id);
void lb_scroll_tables_set(::LbPaneScroll* scroll, ::LbEquipRec* a, ::LbEquipRec* b, ::LbEquipRec* c, ::LbEquipRec* d);
/* The box menu's draw helpers (defined below). */
void lb_box_guide_draw(u16 frame, u16 guide, u16 button, u8 mode, u8 changed);
void lb_box_top_draw(LbBoxMenu* self);
u8 lb_box_lists_draw(::LbListPane* self, u16 parts);
void lb_box_combine_draw(::LbDigitPane* digits, ::MenuSelCursor* cursor);
void lb_my_set_list_draw(::LbMySetPane* self);
void lb_gun_set_list_draw(::LbMySetPane* self);
void lb_model_pane_draw(::LbModelPane* self);
void lb_color_pane_draw(LbColorPane* self);
void lb_dye_pane_draw(LbPage* self);
void lb_dye_inner_draw(::LbChgColorWork* self);
}

extern "C" {
/* ---------------------------------------------------------------------------------------------------
 * The page band's small-data tables, in their `.sdata` order (0x80791C70-0x80791D40).
 * ------------------------------------------------------------------------------------------------- */
/* The entry pane's sort menu titles: the second- and third-level choices of each first-level sort. */
u16 lb_sort_title_kind[4] = {0x0253, 0x024E, 0x024F, 0};
u16 lb_sort_title_blade_class[4] = {0x0253, 0x00AE, 0x00AF, 0x00B0};
u16 lb_sort_title_order_a[4] = {0x0253, 0x00F7, 0x0251, 0x0252};
u16 lb_sort_title_order_b[4] = {0x00FF, 0x0100, 0x0101, 0};
u16 lb_sort_title_gun[4] = {0x0253, 0x0102, 0x0103, 0};
u16 lb_sort_title_gun_order[4] = {0x0253, 0x0105, 0x0106, 0};
u16 lb_sort_title_armor[2] = {0x0253, 0x0250};
/* The worn armour slot of each dye part (`lb_dye_pane_draw`). */
u8 lb_dye_part_equip_index[8] = {4, 4, 0, 1, 2, 3, 0, 0};
/* The item box grid's lock marks and page arrows (`lb_pane_draw`, `lb_box_grid_draw`). */
u16 lb_box_grid_lock[2] = {0x1AF0, 0xFFFF};
u16 lb_box_grid_arrow_l[4] = {0x1AE3, 0x1AE4, 0xFFFF, 0};
u16 lb_box_grid_arrow_r[4] = {0x1AE5, 0x1AE6, 0xFFFF, 0};
u16 lb_box_grid_lock_b[4] = {0x1AE2, 0xFFFF, 0, 0};
/* The model pane's two header frames (`lb_model_pane_draw`). */
u16 lb_model_pane_head_a[4] = {0x1CD8, 0x1CD9, 0x1CDA, 0xFFFF};
u16 lb_model_pane_head_b[4] = {0x1CD7, 0x1CDB, 0x1CDC, 0xFFFF};
/* The colour slot's sprite runs per mode (`lb_color_slot_draw`): the frame, the cursor and the mark. */
u16 lb_color_slot_frame_inner[2] = {0x1C56, 0xFFFF};
u16 lb_color_slot_mark_inner[2] = {0x1C55, 0xFFFF};
u16 lb_color_slot_frame_hair[4] = {0x1C93, 0xFFFF, 0, 0};
u16 lb_color_slot_cursor_hair[4] = {0x1C96, 0x1C94, 0x1C95, 0xFFFF};
u16 lb_color_slot_mark_hair[4] = {0x1C92, 0xFFFF, 0, 0};
const u16* lb_color_slot_frames[2] = {lb_color_slot_frame_inner, lb_color_slot_frame_hair};
u16* lb_color_slot_cursors[2] = {lb_color_slot_cursor_inner, lb_color_slot_cursor_hair};
const u16* lb_color_slot_marks[2] = {lb_color_slot_mark_inner, lb_color_slot_mark_hair};
u16 lb_color_slot_name_lsp[2] = {0x1C53, 0x1C91};
u16 lb_color_slot_swatch_lsp[2] = {0x1C54, 0x1C90};
/* The colour pane's header per mode (`lb_color_pane_draw`). */
u16 lb_color_pane_head_hair[4] = {0x1CFA, 0x1CFD, 0x1CFE, 0xFFFF};
u16 lb_color_pane_head_inner[4] = {0x1CF9, 0x1CFB, 0x1CFC, 0xFFFF};
/* The saved-set list's frame and the move step's mark (`lb_my_set_list_draw`, `lb_gun_set_list_draw`). */
u16 lb_my_set_list_frame[2] = {0x1BF7, 0xFFFF};
__declspec(section ".sdata") u16 lb_my_set_move_mark[6] = {0x1BD1, 0x1BD2, 0xFFFF, 0, 0, 0};
/* The gunner set's three cell layout sprites (`lb_gun_set_cells_draw`). */
u16 lb_gun_set_cell_lsp[4] = {0x1A3D, 0x1A3E, 0x1A3F, 0xFFFF};

}


extern "C" {
s32 lb_cursor_box_step(s16* x, s16* y, s16 x_min, s16 x_max, s16 y_min, s16 y_max, u16 buttons);
void lb_color_cursor_default(u32 color, s16* x, s16* y);
void fn_801EC804(s16* value);
}




extern "C" {

#pragma peephole off
/* 0x801F3294 - tick one lobby page request: a three-state machine that reads the pad mask, advances the
 * page cursor and pushes the new selection through `pl_equip_model_set`. */
u32 fn_801F3294(LbPageOwner* self, LbPage* req)
{
    u32 ret = 0;
    LbWorldBlock* world = lobby_world_block;
    LbPage* sub = self->page_0x010;

    switch (req->state_0x000) {
    case 0:
        if (req->done_0x001 == 1) {
            req->state_0x000 = 2;
            if (lb_page_record_empty_ck(sub) == 1) {
                ret = 2;
            }
            break;
        }
        if (lb_cmd_pressed_ck(0x80) == 1) {
            fn_801EC828(self);
            break;
        }
        if (lb_cmd_pressed_ck(0x10) == 1) {
            if (lb_page_record_empty_ck(sub) != 1) {
                break;
            }
            switch (req->mode_0x002) {
            case 0:
                if (world->count_0x009E != req->value_0x004) {
                    LbStepArg arg;

                    req->state_0x000++;
                    sysSE_stop(3);
                    userdata_hunter_equip_set(world, (u8)req->value_0x004);
                    sub->flag_0x01C = world->count_0x009E;
                    memset(&arg, 0, sizeof(arg));
                    arg.kind_0x00 = 1;
                    pl_equip_model_set((struct _PLW*)sub, (u8*)&arg);
                    arg.kind_0x00 = 2;
                    pl_equip_model_set((struct _PLW*)sub, (u8*)&arg);
                    arg.kind_0x00 = 3;
                    pl_equip_model_set((struct _PLW*)sub, (u8*)&arg);
                    arg.kind_0x00 = 4;
                    pl_equip_model_set((struct _PLW*)sub, (u8*)&arg);
                    arg.kind_0x00 = 5;
                    pl_equip_model_set((struct _PLW*)sub, (u8*)&arg);
                    fn_801EC7AC(self);
                } else {
                    sysSE_req(2);
                }
                break;
            case 1:
                if (world->count_0x0002 != req->value_0x004) {
                    req->state_0x000++;
                    sysSE_stop(0x28);
                    userdata_hunter_hair_set(world, (u8)req->value_0x004);
                    pl_parts_color_set((struct _PLW*)sub, 5, world->count_0x0002);
                    fn_801EC7AC(self);
                } else {
                    sysSE_req(2);
                }
                break;
            }
        } else {
            if (lb_cmd_pressed_ck(0x20) == 1) {
                userdata_pl_look_apply(sub, world);
                req->done_0x001 = 1;
                sysSE_req(1);
            } else if (lb_cmd_repeat_ck(3) != 0) {
                req->value_0x004 = menu_cursor_step(req->value_0x004, req->count_0x006, lb_cmd_repeat_get(), 1, 2);
            }
        }
        break;
    case 1:
        if (lb_page_record_empty_ck(sub) == 1 && lb_cmd_pressed_ck(0x30) == 1) {
            req->state_0x000 = 0;
            sysSE_req(0);
        }
        break;
    case 2:
        if (lb_page_record_empty_ck(sub) == 1) {
            ret = 2;
        }
        break;
    }
    return ret;
}

/* 0x801F353C - walk a `-1`-terminated `s16` table and report its largest value and its length.  The
 * target keeps the out-of-line call from `fn_801F3DEC`, so `-inline auto` may not fold it. */
__declspec(noinline) void fn_801F353C(const s16* table, s16* out_max, s16* out_count)
{
    s16 count = 0;
    s16 max = 0;

    while (*table != -1) {
        if (*table > max) {
            max = *table;
        }
        count++;
        table++;
    }
    if (out_max != NULL) {
        *out_max = max;
    }
    if (out_count != NULL) {
        *out_count = count;
    }
}

/* 0x801F3588 - sum a `-1`-terminated `s16` table (each entry as an unsigned 16-bit value).  The target
 * keeps the out-of-line call, so `-inline auto` must not fold it into its callers. */
__declspec(noinline) u32 fn_801F3588(const s16* table)
{
    u32 total = 0;

    while (*table != -1) {
        total += (u16)*table;
        table++;
    }
    return total;
}

/* 0x801F35B0 (0x11C): the colour grid's stick step: the row on up/down (the column kept inside the row's width),
 * the column on left/right; returns 1 when the stick moved the cursor. */
u32 lb_color_grid_step(const s16* widths, s16* column, s16* row, s16 rows)
{
    s16 width;

    if (lb_cmd_repeat_ck(0xF) != 0) {
        if (lb_cmd_repeat_ck(3) != 0) {
            *row = menu_cursor_step(*row, rows, lb_cmd_repeat_get(), 1, 2);
            width = widths[*row];
            if (width > 1 && *column >= width) {
                *column = width - 1;
            }
        }
        if (widths[*row] > 1 && lb_cmd_repeat_ck(0xC) != 0) {
            *column = menu_cursor_step(*column, widths[*row], lb_cmd_repeat_get(), 4, 8);
        }
        return 1;
    }
    return 0;
}

/* 0x801F36CC (0xFC): the colour grid cell index of `row`/`column`: the widths of the rows above, plus the column
 * on a row wider than one cell. */
s16 lb_color_grid_index(const s16* widths, s16 column, s16 row)
{
    s16 i;
    s16 index;

    index = 0;
    for (i = 0; i < row; i++) {
        index += widths[i];
    }
    if (widths[i] != 1) {
        index += column;
    }
    return index;
}

/* 0x801F37C8 - store the selection the given row id names: the packed table bytes for an ordinary row,
 * or the page's own id for the "current" row marker. */
void fn_801F37C8(LbColorPane* self, s16 sel)
{
    u32 value;

    switch (sel) {
    case -3:
        value = self->color_0x14;
        break;
    case -2:
        return;
    default: {
        const LbSelRec* rec = &lbl_805B8674[sel];

        value = ((u32)rec->bits_0x03 << 24) | ((u32)rec->bits_0x04 << 16) | ((u32)rec->bits_0x05 << 8) | 0xFF;
        break;
    }
    }
    self->picked_0x18 = value;
}

/* 0x801F3828 (0x170): seeds the colour pane for the inner (0) or hair (1) colour and resets the shown models. */
void lb_color_pane_init(LbColorPane* self, ::_PLW* plw, u8 mode)
{
    LbStepArg arg;

    self->phase_0x00 = 0;
    self->done_0x01 = 0;
    self->mode_0x02 = mode;
    self->changed_0x03 = 0;
    self->column_0x04 = 0;
    self->row_0x06 = 0;
    fn_801F353C(lb_color_grid_widths, &self->widest_0x08, &self->rows_0x0A);
    self->anim_0x0C = 0;
    self->cursor_x_0x0E = 1;
    self->cursor_y_0x10 = 1;
    switch (self->mode_0x02) {
    case 0:
        self->color_0x14 = lobby_world_block->inner_color_0x39BC;
        memset(&arg, 0, sizeof(arg));
        arg.kind_0x00 = 1;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        arg.kind_0x00 = 2;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        arg.kind_0x00 = 3;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        arg.kind_0x00 = 4;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        arg.kind_0x00 = 5;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        break;
    case 1:
        self->color_0x14 = lobby_world_block->hair_color_0x0014;
        memset(&arg, 0, sizeof(arg));
        arg.kind_0x00 = 5;
        pl_equip_model_set((struct _PLW*)plw, (u8*)&arg);
        break;
    default:
        self->color_0x14 = 0xFF;
        break;
    }
    lb_color_cursor_default(self->color_0x14, &self->cursor_x_0x0E, &self->cursor_y_0x10);
    self->original_0x1C = self->color_0x14;
    self->picked_0x18 = self->color_0x14;
    self->plw_0x24 = plw;
}

/* 0x801F3998 - write the three colour bytes of one of the page's two palettes from the top three bytes
 * of a packed value. */
void fn_801F3998(LbPage* self, u32 rgb, u8 which)
{
    s32 kind = which;

    switch (kind) {
    case 1:
        self->rgb_a_0x258[0] = (u8)(rgb >> 24);
        self->rgb_a_0x258[1] = (u8)((rgb >> 16) & 0xFF);
        self->rgb_a_0x258[2] = (u8)((rgb >> 8) & 0xFF);
        break;
    case 0:
        self->rgb_b_0x25C[0] = (u8)(rgb >> 24);
        self->rgb_b_0x25C[1] = (u8)((rgb >> 16) & 0xFF);
        self->rgb_b_0x25C[2] = (u8)((rgb >> 8) & 0xFF);
        break;
    }
}

/* 0x801F39E8 (0x404): the colour pane's step: the grid picks a preset or the free cursor a colour, the button stores
 * it in the world block; returns 2 when left with the old colour, 1 with a new one. */
u32 lb_color_pane_step(LbPageOwner* owner, LbColorPane* self)
{
    const s16* cells;
    PlayerPad* pad;
    u32 result;
    LbWorldBlock* world;
    u16 index;
    LbPage* page;
    _SPR_DATA_* box;
    const s16* widths;

    result = 0;
    pad = &Psw[0];
    world = lobby_world_block;
    page = (LbPage*)self->plw_0x24;
    fn_801EC804(&self->anim_0x0C);
    do {
        switch (self->phase_0x00) {
        case 0:
            if (self->done_0x01 == 1) {
                self->phase_0x00 = 2;
                self->done_0x01 = 0;
                break;
            }
            if (self->done_0x01 == 2) {
                self->phase_0x00 = 3;
                self->done_0x01 = 0;
                continue;
            }
            if (lb_cmd_pressed_ck(0x80) == 1) {
                fn_801EC828(owner);
                return 0;
            }
            widths = lb_color_grid_widths;
            cells = lbl_805B87D8;
            if (self->mode_0x02 == 0) {
                cells = lbl_805B87F4;
            }
            if (lb_cmd_pressed_ck(0x10) == 1) {
                index = lb_color_grid_index(widths, self->column_0x04, self->row_0x06);
                switch (cells[index]) {
                default:
                    fn_801F37C8(self, cells[index]);
                    fn_801F3998(page, self->picked_0x18, self->mode_0x02);
                    if (cells[index] >= 0) {
                        self->cursor_x_0x0E = lbl_805B8674[cells[index]].cursor_x_0x06;
                        self->cursor_y_0x10 = lbl_805B8674[cells[index]].cursor_y_0x08;
                    }
                    self->changed_0x03 = 1;
                    sysSE_stop(4);
                    return result;
                case -2:
                    switch (self->mode_0x02) {
                    case 1:
                        world->hair_color_0x0014 = self->picked_0x18;
                        break;
                    case 0:
                        world->inner_color_0x39BC = self->picked_0x18;
                        break;
                    }
                    userdata_pl_look_apply(page, world);
                    fn_801EC7AC(owner);
                    sysSE_req(0);
                    self->done_0x01 = 2;
                    break;
                case -3:
                    break;
                }
            } else if (lb_cmd_pressed_ck(0x20) == 1) {
                if (self->changed_0x03 == 0) {
                    switch (self->mode_0x02) {
                    case 1:
                        world->hair_color_0x0014 = self->original_0x1C;
                        break;
                    case 0:
                        world->inner_color_0x39BC = self->original_0x1C;
                        break;
                    }
                    userdata_pl_look_apply(page, world);
                    self->done_0x01 = 1;
                } else {
                    self->phase_0x00++;
                    self->stepper_0x20 = 0;
                }
                sysSE_req(1);
            } else if (lb_color_grid_step(widths, &self->column_0x04, &self->row_0x06, self->rows_0x0A) == 0) {
                index = lb_color_grid_index(widths, self->column_0x04, self->row_0x06);
                if (cells[index] == -3) {
                    box = (_SPR_DATA_*)get_lsp_data(0x1CA8, NULL);
                    if (lb_cursor_box_step(&self->cursor_x_0x0E, &self->cursor_y_0x10, 1, box->width - 1, 1,
                                           box->height - 1, pad->button_0x2C0.press_0x08) == 1U) {
                        self->changed_0x03 = 1;
                        fn_801F37C8(self, cells[index]);
                        fn_801F3998(page, self->picked_0x18, self->mode_0x02);
                    }
                }
            }
            break;
        case 1:
            if (self->done_0x01 == 1) {
                self->phase_0x00++;
                self->done_0x01 = 0;
                break;
            }
            switch (toggle_word_step(&self->stepper_0x20, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
            case 1:
                switch (self->mode_0x02) {
                case 1:
                    world->hair_color_0x0014 = self->original_0x1C;
                    break;
                case 0:
                    world->inner_color_0x39BC = self->original_0x1C;
                    break;
                }
                userdata_pl_look_apply(page, world);
                sysSE_req(0);
                self->done_0x01 = 1;
                break;
            case 2:
                self->phase_0x00--;
                break;
            }
            return result;
        case 2:
            break;
        case 3:
            continue;
        default:
            return result;
        }
        if (lb_page_record_empty_ck(page) == 1) {
            result = 2;
        }
        return result;
    } while (0);
    if (lb_page_record_empty_ck(page) == 1) {
        result = 1;
    }
    return result;
}

/* 0x801F3DEC - initialise one page record: clear its state, copy its kind from the world block and seed
 * the entry bounds from the kind's table. */
void fn_801F3DEC(LbPage* self, LbPage* next, u8 kind)
{
    self->state_0x000 = 0;
    self->done_0x001 = 0;
    self->kind_0x003 = kind;
    if (kind == 1 && userdata_flag_ck(19001) == 1) {
        self->kind_0x003 = 2;
    }
    self->mode_0x002 = lobby_world_block->field_0x39BA;
    self->value_0x004 = 0;
    self->count_0x006 = 0;
    self->field_0x008 = 0;
    self->field_0x00E = 0;
    self->field_0x010 = 1;
    self->field_0x012 = 1;
    self->sel_id_0x014 = 0;
    self->field_0x023 = 0;
    switch (self->kind_0x003) {
    default:
        self->max_0x00A = 1;
        self->num_0x00C = 5;
        break;
    case 1:
        fn_801F353C(lbl_805B8768, &self->max_0x00A, (s16*)&self->num_0x00C);
        break;
    case 2:
        fn_801F353C(lbl_805B8798, &self->max_0x00A, (s16*)&self->num_0x00C);
        break;
    }
    self->next_0x028 = next;
}

/* 0x801F3EDC - the page's entry count: either table's total for its two scrollable kinds, else the
 * count the page itself carries. */
u32 fn_801F3EDC(LbPage* self)
{
    s32 kind = self->kind_0x003;

    if (kind != 1) {
        if (kind == 2) {
            return fn_801F3588(lbl_805B8798);
        }
        return self->num_0x00C;
    }
    return fn_801F3588(lbl_805B8768);
}

/* 0x801F3F14 - whether the given index table entry is the one the page is currently showing. */
u32 fn_801F3F14(LbPage* self, u8 index)
{
    s16 entry = lbl_805B875C[index];
    u8 current;

    if (entry == -2) {
        return 0;
    }
    current = self->sub_0x262;
    if (current == 0) {
        return entry == -1;
    }
    return lbl_805B8674[entry].match_0x02 == current;
}

/* 0x801F3F78 - forward a byte to the child panel and refresh its seven slots from the index table. */
void fn_801F3F78(LbPage* self, u8 value)
{
    u32 i;

    self->next_0x028->sub_0x262 = value;
    for (i = 0; i < 7; i++) {
        u8 slot = fn_802738B8((u8)i);

        if (slot != 0xFF) {
            pl_equip_color_apply((::_PLW*)self->next_0x028, slot);
        }
    }
}

/* 0x801F3FDC (0x240): the dye pane's inner-wear step: picks the inner wear the player is shown in, keeping the world
 * block's choice until it is decided; returns 1 when decided, 2 when left. */
u32 lb_dye_inner_step(LbPageOwner* owner, LbPage* self)
{
    u32 result;
    s16 entry;

    result = 0;
    switch (self->state_0x000) {
    case 0:
        if (lb_cmd_pressed_ck(0x80) == 1) {
            fn_801EC828(owner);
            return 0;
        }
        if (lb_cmd_pressed_ck(0x10) == 1) {
            if (fn_801F3F14(self->next_0x028, (u8)self->field_0x008) == 0) {
                entry = lbl_805B875C[self->field_0x008];
                switch (entry) {
                case -1:
                    fn_801F3F78(self, 0);
                    sysSE_stop(4);
                    break;
                case -2:
                    lobby_world_block->field_0x39BA = self->next_0x028->sub_0x262;
                    fn_801EC7AC(owner);
                    result = 1;
                    sysSE_req(0);
                    break;
                default:
                    fn_801F3F78(self, lbl_805B8674[entry].match_0x02);
                    sysSE_stop(4);
                    break;
                }
                self->field_0x023 = 1;
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1) {
            if (self->field_0x023 == 0) {
                fn_801F3F78(self, lobby_world_block->field_0x39BA);
                result = 2;
            } else {
                self->state_0x000++;
                self->stepper_0x024 = 0;
            }
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->field_0x008 = menu_cursor_step(self->field_0x008, (s16)self->num_0x00C, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 1:
        switch (toggle_word_step(&self->stepper_0x024, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            fn_801F3F78(self, lobby_world_block->field_0x39BA);
            sysSE_req(0);
            result = 2;
            break;
        case 2:
            self->state_0x000--;
            break;
        }
        break;
    }
    return result;
}

/* 0x801F421C (0x114): whether the player's armour piece `part` (1..5; 0 = any of them) can be dyed. */
u32 lb_dye_part_ck(::_PLW* plw, u16 part)
{
    u8 i;

    switch (part) {
    case 0:
        for (i = 1; i < 6; i++) {
            if (lb_dye_part_ck(plw, i) == 1) {
                return 1;
            }
        }
        break;
    case 1:
        if (EnableChangeColor(&plw->equipA[4]) == 1) {
            return 1;
        }
        break;
    case 2:
        if (EnableChangeColor(&plw->equipA[0]) == 1) {
            return 1;
        }
        break;
    case 3:
        if (EnableChangeColor(&plw->equipA[1]) == 1) {
            return 1;
        }
        break;
    case 4:
        if (EnableChangeColor(&plw->equipA[2]) == 1) {
            return 1;
        }
        break;
    case 5:
        if (EnableChangeColor(&plw->equipA[3]) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x801F4330 (0x114): loads armour piece `part`'s dye colour into the pane (every dyeable piece for 0). */
void lb_dye_part_color_load(LbPage* self, u16 part)
{
    u16 i;

    self->sel_flag_0x022 = 0;
    switch (part) {
    case 0:
        for (i = 1; i < 6; i++) {
            if (lb_dye_part_ck((::_PLW*)self->next_0x028, i) == 1) {
                lb_dye_part_color_load(self, i);
            }
        }
        self->sel_0x020 = 0;
        self->sel_flag_0x022 = 1;
        break;
    case 1:
        self->sel_0x020 = self->part_colors_0x016[0] = ((::_PLW*)self->next_0x028)->equipA[4].color_rgb565;
        break;
    case 2:
        self->sel_0x020 = self->part_colors_0x016[1] = ((::_PLW*)self->next_0x028)->equipA[0].color_rgb565;
        break;
    case 3:
        self->sel_0x020 = self->part_colors_0x016[2] = ((::_PLW*)self->next_0x028)->equipA[1].color_rgb565;
        break;
    case 4:
        self->sel_0x020 = self->part_colors_0x016[3] = ((::_PLW*)self->next_0x028)->equipA[2].color_rgb565;
        break;
    case 5:
        self->sel_0x020 = self->part_colors_0x016[4] = ((::_PLW*)self->next_0x028)->equipA[3].color_rgb565;
        break;
    }
}

/* 0x801F4444 - decode the selected table row into the page's selection word; the two negative markers
 * only raise/lower the "open" flag. */
void fn_801F4444(LbPage* self, u16 value_0x004, s16 sel)
{
    u16 value;

    switch (sel) {
    case -1:
        self->sel_flag_0x022 = 1;
        return;
    case -3:
        value = self->sel_id_0x014;
        break;
    case -2:
        return;
    default: {
        const LbSelRec* rec = &lbl_805B8674[sel];

        value = (u16)(((s32)(rec->bits_0x03 & 0xF8) << 8) | ((s32)(rec->bits_0x04 & 0xFC) << 3) |
                      ((s32)(rec->bits_0x05 & 0xF8) >> 3));
        break;
    }
    }
    self->sel_flag_0x022 = 0;
    self->sel_0x020 = value;
}

/* 0x801F44CC (0x150): shows the pane's colour on armour piece `part` (every dyeable piece for 0); in the
 * all-pieces mode each piece takes its default dye. */
void lb_dye_part_color_apply(LbPage* self, u16 part)
{
    ::_EQUIP* equip;
    ::_PLW* plw;
    u16 i;
    u8 rgba[4];
    s32 word;
    u16 color;

    plw = (::_PLW*)self->next_0x028;
    if ((u32)(part - 1) > 4) {
        if (part == 0) {
            for (i = 1; i < 6; i++) {
                if (lb_dye_part_ck((::_PLW*)self->next_0x028, i) == 1) {
                    lb_dye_part_color_apply(self, i);
                }
            }
        }
    } else {
        switch (part) {
        case 1:
            equip = &plw->equipA[4];
            break;
        case 2:
            equip = &plw->equipA[0];
            break;
        case 3:
            equip = &plw->equipA[1];
            break;
        case 4:
            equip = &plw->equipA[2];
            break;
        case 5:
            equip = &plw->equipA[3];
            break;
        }
        if (EnableChangeColor(equip) == 1) {
            if (self->sel_flag_0x022 == 1) {
                word = equip_color_rgba_get(equip_color_index_get(equip));
                color_rgba_copy(rgba, (u8*)&word);
                color = ((rgba[0] & 0xF8) << 8) | ((rgba[1] & 0xFC) << 3) | ((rgba[2] & 0xF8) >> 3);
                self->sel_0x020 = color;
            } else {
                color = self->sel_0x020;
            }
            equip_color_set(equip, color);
        }
    }
}

/* 0x801F461C (0x100): stores the pane's colour in the worn armour piece `part` of the save block (every dyeable
 * piece for 0). */
void lb_dye_part_color_commit(LbPage* self, u16 part)
{
    LbWorldBlock* world;
    u16 i;

    world = lobby_world_block;
    switch (part) {
    case 0:
        for (i = 1; i < 6; i++) {
            if (lb_dye_part_ck((::_PLW*)self->next_0x028, i) == 1) {
                lb_dye_part_color_commit(self, i);
            }
        }
        break;
    case 1:
        equip_worn_color_set(world, 5, self->sel_0x020, self->sel_flag_0x022);
        break;
    case 2:
        equip_worn_color_set(world, 1, self->sel_0x020, self->sel_flag_0x022);
        break;
    case 3:
        equip_worn_color_set(world, 2, self->sel_0x020, self->sel_flag_0x022);
        break;
    case 4:
        equip_worn_color_set(world, 3, self->sel_0x020, self->sel_flag_0x022);
        break;
    case 5:
        equip_worn_color_set(world, 4, self->sel_0x020, self->sel_flag_0x022);
        break;
    }
}

/* 0x801F471C (0xE8): puts armour piece `part`'s loaded colour back on the player (every dyeable piece for 0). */
void lb_dye_part_color_restore(LbPage* self, u16 part)
{
    u16 i;

    switch (part) {
    case 0:
        for (i = 1; i < 6; i++) {
            if (lb_dye_part_ck((::_PLW*)self->next_0x028, i) == 1) {
                lb_dye_part_color_restore(self, i);
            }
        }
        break;
    case 1:
        ((::_PLW*)self->next_0x028)->equipA[4].color_rgb565 = self->part_colors_0x016[0];
        break;
    case 2:
        ((::_PLW*)self->next_0x028)->equipA[0].color_rgb565 = self->part_colors_0x016[1];
        break;
    case 3:
        ((::_PLW*)self->next_0x028)->equipA[1].color_rgb565 = self->part_colors_0x016[2];
        break;
    case 4:
        ((::_PLW*)self->next_0x028)->equipA[2].color_rgb565 = self->part_colors_0x016[3];
        break;
    case 5:
        ((::_PLW*)self->next_0x028)->equipA[3].color_rgb565 = self->part_colors_0x016[4];
        break;
    }
}

/* 0x801F4804 (0x410): the dye pane's step: picks the armour piece, then a preset or a free colour for it; the
 * button stores it, cancel puts the old colour back.  Returns 2 when left. */
u32 lb_dye_pane_step(LbPageOwner* owner, LbPage* self)
{
    s16 index;
    const s16* widths;
    _SPR_DATA_* box;
    u32 result;
    const s16* cells;
    PlayerPad* pad;

    result = 0;
    pad = &Psw[0];
    fn_801EC804(&self->anim_0x00E);
    switch (self->state_0x000) {
    case 0:
        if (lb_cmd_pressed_ck(0x80) == 1) {
            fn_801EC828(owner);
            return 0;
        }
        if (lb_cmd_pressed_ck(0x10) == 1) {
            if (lb_dye_part_ck((::_PLW*)self->next_0x028, self->part_0x004) == 1) {
                self->state_0x000++;
                self->count_0x006 = 0;
                self->field_0x008 = 0;
                self->field_0x023 = 0;
                lb_dye_part_color_load(self, self->part_0x004);
                lb_color_cursor_default(self->sel_0x020, &self->cursor_x_0x010, &self->cursor_y_0x012);
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->part_0x004 = menu_cursor_step(self->part_0x004, 6, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x80) == 1) {
            fn_801EC828(owner);
            return 0;
        }
        if (self->kind_0x003 == 1) {
            cells = lb_dye_grid_cells_a;
            widths = lbl_805B8768;
        } else {
            cells = lb_dye_grid_cells_b;
            widths = lbl_805B8798;
        }
        if (lb_cmd_pressed_ck(0x10) == 1) {
            index = lb_color_grid_index(widths, self->count_0x006, self->field_0x008);
            switch (cells[index]) {
            default:
                fn_801F4444(self, self->part_0x004, cells[index]);
                lb_dye_part_color_apply(self, self->part_0x004);
                fn_801F3F78(self, lobby_world_block->field_0x39BA);
                if (cells[index] >= 0) {
                    self->cursor_x_0x010 = lbl_805B8674[cells[index]].cursor_x_0x06;
                    self->cursor_y_0x012 = lbl_805B8674[cells[index]].cursor_y_0x08;
                }
                self->field_0x023 = 1;
                sysSE_stop(4);
                break;
            case -2:
                self->state_0x000--;
                lb_dye_part_color_commit(self, self->part_0x004);
                fn_801EC7AC(owner);
                sysSE_req(0);
                break;
            case -3:
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) == 1) {
            if (self->field_0x023 == 0) {
                self->state_0x000 = 0;
                lb_dye_part_color_restore(self, self->part_0x004);
                fn_801F3F78(self, lobby_world_block->field_0x39BA);
            } else {
                self->state_0x000++;
                self->stepper_0x024 = 0;
            }
            sysSE_req(1);
        } else if (lb_color_grid_step(widths, &self->count_0x006, &self->field_0x008, self->num_0x00C) == 0) {
            index = lb_color_grid_index(widths, self->count_0x006, self->field_0x008);
            if (cells[index] == -3) {
                box = (_SPR_DATA_*)get_lsp_data(0x1CA8, NULL);
                if (lb_cursor_box_step(&self->cursor_x_0x010, &self->cursor_y_0x012, 1, box->width - 1, 1,
                                       box->height - 1, pad->button_0x2C0.press_0x08) == 1U) {
                    self->field_0x023 = 1;
                    fn_801F4444(self, self->part_0x004, cells[index]);
                    lb_dye_part_color_apply(self, self->part_0x004);
                    fn_801F3F78(self, lobby_world_block->field_0x39BA);
                }
            }
        }
        break;
    case 2:
        switch (toggle_word_step_dpad(&self->stepper_0x024, lobby_cmd_trig_get(), 4, 8)) {
        case 1:
            self->state_0x000 = 0;
            lb_dye_part_color_restore(self, self->part_0x004);
            fn_801F3F78(self, lobby_world_block->field_0x39BA);
            break;
        case 2:
            self->state_0x000--;
            break;
        }
        break;
    }
    return result;
}

/* 0x801F4C14 (0x764): the box menu's frame step: opens it on the first frame, runs the top list (whose pick picks
 * and seeds a pane), then the active pane's step; a pane that ends returns the menu to the top list. */
void lb_box_menu_step(void)
{
    LbBoxMenu* self;
    s32 result;
    s32 refused;

    self = (LbBoxMenu*)lobby_w.menu_0xAC;
    refused = 0;
    self->moved_0x01C = 0;
    if (self->state_0x000 == 0) {
        self->state_0x000 = 1;
        if (self->list_kind_0x002 == 0) {
            camera_lobby_menu_enter();
        }
    } else {
        ainpc_page_hold_set();
        if (self->state_0x000 == 6 || (u32)chk_pointer() != 1) {
            if (self->leave_0x003 == 1) {
                lb_box_cam_input_step((::LbMenuPanel*)self, 0);
            } else {
                switch (self->state_0x000) {
                case 1:
                    switch (fn_801ECB40()) {
                    case 1:
                        self->sub_0x001 = 0;
                        switch (self->menu_row_0x006) {
                        case 0:
                            switch (self->menu_cursor_0x008) {
                            case 0:
                                self->state_0x000 = 2;
                                fn_801ED284((::LbListPane*)self);
                                break;
                            case 1:
                                self->state_0x000 = 3;
                                fn_801ED284((::LbListPane*)self);
                                break;
                            }
                            break;
                        case 1:
                            switch (self->menu_cursor_0x008) {
                            case 0:
                                self->state_0x000 = 4;
                                fn_801ED284((::LbListPane*)self);
                                fn_801ECD50(&self->digits_0x054);
                                break;
                            case 1:
                                self->state_0x000 = 5;
                                fn_801ED284((::LbListPane*)self);
                                break;
                            }
                            break;
                        case 2:
                            switch (self->menu_cursor_0x008) {
                            case 0:
                                self->state_0x000 = 6;
                                fn_801ECD50(&self->digits_0x054);
                                break;
                            case 1:
                                self->state_0x000 = 7;
                                fn_801ECD50(&self->digits_0x054);
                                break;
                            case 2:
                                self->state_0x000 = 8;
                                fn_801ECD50(&self->digits_0x054);
                                break;
                            }
                            break;
                        case 3:
                            switch (self->menu_cursor_0x008) {
                            case 0:
                                self->state_0x000 = 9;
                                lb_list_pane_init(&self->entry_0x074, 0, (::LbPage*)self->page_0x010);
                                break;
                            case 1:
                                self->state_0x000 = 10;
                                lb_list_pane_init(&self->entry_0x074, 5, (::LbPage*)self->page_0x010);
                                break;
                            case 2:
                                self->state_0x000 = 11;
                                lb_list_pane_init(&self->entry_0x074, 4, (::LbPage*)self->page_0x010);
                                break;
                            }
                            break;
                        case 5:
                            switch (self->menu_cursor_0x008) {
                            case 0:
                                if (my_set_any_ck(lobby_world_block) == 1) {
                                    self->state_0x000 = 12;
                                    lb_my_set_pane_init(&self->my_set_0x3A0, 0, 0, (::LbPage*)self->page_0x010);
                                } else {
                                    refused = 1;
                                }
                                break;
                            case 1:
                                self->state_0x000 = 13;
                                lb_my_set_pane_init(&self->my_set_0x3A0, 0, 1, (::LbPage*)self->page_0x010);
                                break;
                            case 2:
                                if (my_set_any_ck(lobby_world_block) == 1) {
                                    self->state_0x000 = 14;
                                    lb_my_set_pane_init(&self->my_set_0x3A0, 0, 2, (::LbPage*)self->page_0x010);
                                } else {
                                    refused = 1;
                                }
                                break;
                            case 3:
                                if (my_set_any_ck(lobby_world_block) == 1) {
                                    self->state_0x000 = 15;
                                    lb_my_set_pane_init(&self->my_set_0x3A0, 0, 3, (::LbPage*)self->page_0x010);
                                } else {
                                    refused = 1;
                                }
                                break;
                            }
                            break;
                        case 6:
                            switch (self->menu_cursor_0x008) {
                            case 0:
                                if (gun_set_any_ck(lobby_world_block) == 1) {
                                    self->state_0x000 = 16;
                                    lb_my_set_pane_init(&self->my_set_0x3A0, 1, 0, (::LbPage*)self->page_0x010);
                                } else {
                                    refused = 1;
                                }
                                break;
                            case 1:
                                if (userdata_gunner_ck(lobby_world_block) == 1) {
                                    self->state_0x000 = 17;
                                    lb_my_set_pane_init(&self->my_set_0x3A0, 1, 1, (::LbPage*)self->page_0x010);
                                } else {
                                    refused = 1;
                                }
                                break;
                            case 2:
                                if (gun_set_any_ck(lobby_world_block) == 1) {
                                    self->state_0x000 = 18;
                                    lb_my_set_pane_init(&self->my_set_0x3A0, 1, 2, (::LbPage*)self->page_0x010);
                                } else {
                                    refused = 1;
                                }
                                break;
                            case 3:
                                if (gun_set_any_ck(lobby_world_block) == 1) {
                                    self->state_0x000 = 19;
                                    lb_my_set_pane_init(&self->my_set_0x3A0, 1, 3, (::LbPage*)self->page_0x010);
                                } else {
                                    refused = 1;
                                }
                                break;
                            }
                            break;
                        case 4:
                            switch (self->menu_cursor_0x008) {
                            case 0:
                                self->state_0x000 = 20;
                                lb_model_pane_init(&self->model_0x390, (::_PLW*)self->page_0x010, 0);
                                break;
                            case 1:
                                self->state_0x000 = 21;
                                lb_color_pane_init(&self->color_0x368, (::_PLW*)self->page_0x010, 0);
                                break;
                            case 2:
                                self->state_0x000 = 22;
                                lb_model_pane_init(&self->model_0x390, (::_PLW*)self->page_0x010, 1);
                                break;
                            case 3:
                                self->state_0x000 = 23;
                                lb_color_pane_init(&self->color_0x368, (::_PLW*)self->page_0x010, 1);
                                break;
                            case 4:
                                self->state_0x000 = 24;
                                fn_801F3DEC((LbPage*)&self->dye_0x33C, self->page_0x010, 0);
                                break;
                            case 5:
                                if (lb_dye_part_ck((::_PLW*)self->page_0x010, 0) == 1) {
                                    self->state_0x000 = 25;
                                    fn_801F3DEC((LbPage*)&self->dye_0x33C, self->page_0x010, 1);
                                } else {
                                    refused = 1;
                                }
                                break;
                            }
                            break;
                        }
                        if (refused == 0) {
                            sysSE_req(5);
                        } else {
                            sysSE_req(2);
                        }
                        break;
                    case 2:
                        lb_box_menu_close((::LbMenuPanel*)self);
                        break;
                    }
                    break;
                case 2:
                    result = fn_801ED688((::LbListPane*)self, 0);
                    break;
                case 3:
                    result = fn_801ED688((::LbListPane*)self, 1);
                    break;
                case 4:
                    result = fn_801EDC4C(&self->digits_0x054, (::LbListPane*)self);
                    break;
                case 5:
                    result = fn_801EE1E4((::LbListPane*)self);
                    break;
                case 6:
                    result = lb_box_combine_step(&self->digits_0x054, &self->cursor_0x030, (::LbListPane*)self);
                    break;
                case 7:
                    result = fn_801EEDC4(&self->digits_0x054, (::LbListPane*)self);
                    break;
                case 8:
                    result = fn_801EEFA0(&self->digits_0x054, (::LbListPane*)self);
                    break;
                case 9:
                    result = lb_equip_box_wear_step((::LbEquipBoxWork*)self);
                    break;
                case 10:
                    result = lb_entry_pane_move_step(&self->entry_0x074, (::LbEquipBoxWork*)self);
                    break;
                case 11:
                    result = lb_entry_pane_sell_step(&self->entry_0x074, (::LbEquipBoxWork*)self);
                    break;
                case 12:
                    result = lb_my_set_load_step((::LbEquipBoxWork*)self, &self->my_set_0x3A0);
                    break;
                case 13:
                    result = lb_my_set_save_step(&self->my_set_0x3A0);
                    break;
                case 14:
                    result = lb_my_set_move_step(&self->my_set_0x3A0);
                    break;
                case 15:
                    result = lb_my_set_delete_step(&self->my_set_0x3A0);
                    break;
                case 16:
                    result = lb_gun_set_load_step((::LbEquipBoxWork*)self, &self->my_set_0x3A0);
                    break;
                case 17:
                    result = lb_gun_set_save_step(&self->my_set_0x3A0);
                    break;
                case 18:
                    result = lb_gun_set_move_step(&self->my_set_0x3A0);
                    break;
                case 19:
                    result = lb_gun_set_delete_step(&self->my_set_0x3A0);
                    break;
                case 20:
                case 22:
                    result = fn_801F3294((LbPageOwner*)self, (LbPage*)&self->model_0x390);
                    break;
                case 21:
                case 23:
                    result = lb_color_pane_step((LbPageOwner*)self, &self->color_0x368);
                    break;
                case 24:
                    result = lb_dye_inner_step((LbPageOwner*)self, (LbPage*)&self->dye_0x33C);
                    break;
                case 25:
                    result = lb_dye_pane_step((LbPageOwner*)self, (LbPage*)&self->dye_0x33C);
                    break;
                }
                if ((u32)(result - 1) <= 1) {
                    self->state_0x000 = 1;
                }
            }
        }
    }
    subTransSet((u32)lb_box_menu_draw, 0, NULL);
}

/* 0x801F5378 (0x14C): the box menu's frame and button guide: the pointer-less frame variant, the guide sprites
 * of the controller option, and the close/confirm prompt when `mode` is set. */
void lb_box_guide_draw(u16 frame, u16 guide, u16 button, u8 mode, u8 changed)
{
    _mh_ivec2_ pos;
    u32 variant_a;
    u32 variant_b;
    u32 prompt_a;
    u32 prompt_b;
    u16 sprite_a;
    u16 sprite_b;

    if ((u32)chk_pointer() == 1) {
        return;
    }
    if (get_option_cfg(7) == 0) {
        variant_a = 5;
        variant_b = 3;
        prompt_a = 0;
        prompt_b = 1;
        sprite_a = 0x1ABC;
        sprite_b = 0x1ABD;
    } else {
        variant_a = 15;
        variant_b = 17;
        prompt_a = 9;
        prompt_b = 10;
        sprite_a = 0x1ABE;
        sprite_b = 0x1ABF;
    }
    if (mode == 0) {
        draw_lsp_sprite_variant(frame, variant_a);
        get_lsp_data(guide, &pos);
        draw_sprite_anim_ary(lb_box_guide_frame, 0, &pos);
        return;
    }
    draw_lsp_sprite_variant(frame, variant_b);
    get_lsp_data(guide, &pos);
    draw_sprite_anim_ary(lb_box_guide_frame, 1, &pos);
    draw_sprite_anim_idx(sprite_a, 1, &pos);
    draw_sprite_anim_idx(sprite_b, 1, &pos);
    get_lsp_data(button, &pos);
    if (changed == 0) {
        put_button_guide_c_idx(button, prompt_a);
    } else {
        put_button_guide_c_idx(button, prompt_b);
    }
}

/* 0x801F54C4 - show the page-turn arrow for the current cursor, at the option's own row offset. */
void fn_801F54C4(void)
{
    _mh_ivec2_ pos;

    if ((u32)chk_pointer() == 1) {
        return;
    }
    get_lsp_data(6833, &pos);
    pos.y += 30;
    if (get_option_cfg(7) == 0) {
        put_button_guide(5, &pos);
    } else {
        put_button_guide(15, &pos);
    }
}

/* 0x801F5534 - the page's current row height, with the "extra" rows the two special pages add. */
s16 fn_801F5534(LbPage* self)
{
    s16 value = lbl_805B85F8[self->count_0x006];

    if (self->max_0x00A == 1) {
        s16 extra = self->field_0x008 + 1;

        value += extra;
        if ((u16)self->count_0x006 <= 1 && self->field_0x008 == 1 && userdata_gunner_ck(lobby_world_block) == 0) {
            value = (s16)(value + 1);
        }
    }
    return value;
}

/* 0x801F55B4 (0x404): the box menu's top list: the category rows, then the open category's sub rows greyed where the
 * pick would be refused (with the reason on the help line), then the guide. */
void lb_box_top_draw(LbBoxMenu* self)
{
    u32 color;
    const u16* sub_panels;
    s16 reason;
    _mh_ivec2_ base;
    u32 closing;
    s32 i;
    const u16* panels;
    _SPR_DATA_ spr;
    s16 reason_row;
    s8* text;
    _mh_ivec2_ pos;
    s32 selected;
    const u16* str_bases;
    bool enabled;
    bool lit;

    reason = -1;
    reason_row = 1;
    font_set_size(18, 18);
    closing = self->leave_0x003 != 0;
    if (closing == 0) {
        get_lsp_data(0x1A80, &base);
        if (self->list_kind_0x002 == 0) {
            draw_sprite_anim_ary(lb_box_top_frame, 0, &base);
        } else {
            draw_sprite_anim_ary(lb_box_top_frame_alt, 1, &base);
        }
        for (i = 0, panels = lb_box_top_row_panels; i < self->menu_rows_0x00C; i++) {
            if (self->menu_row_0x006 == i) {
                selected = self->menu_page_0x00A == 0;
                lit = 1;
            } else {
                selected = 0;
                lit = 0;
            }
            color = GetMenuFontColor(1, lit, self->menu_page_0x00A == 0, 0);
            lb_frame_draw_at(0x1A80, *panels, (s8*)LbStr(0, i + 0x212), selected, color, 0);
            panels++;
        }
        if (self->menu_page_0x00A >= 1) {
            sprite_frame_apply(&spr, 0x1A9C, self->menu_row_0x006, &base);
            draw_sprite_anim_ary(lb_box_sub_frame, lb_box_sub_frame_kinds[self->menu_subrows_0x00E], &base);
            reason = -1;
            for (i = 0, sub_panels = lb_box_sub_row_panels, str_bases = lb_box_sub_str_bases;
                 i < self->menu_subrows_0x00E; i++) {
                enabled = 1;
                switch (self->menu_row_0x006) {
                case 5:
                    if (((u32)(i - 2) <= 1 || i == 0) && my_set_any_ck(lobby_world_block) == 0) {
                        enabled = 0;
                        if (self->menu_cursor_0x008 == i) {
                            reason = 0xB1;
                            reason_row = 2;
                        }
                    }
                    break;
                case 6:
                    switch (i) {
                    case 0:
                    case 2:
                    case 3:
                        if (gun_set_any_ck(lobby_world_block) == 0) {
                            enabled = 0;
                            if (self->menu_cursor_0x008 == i) {
                                reason = 0xB1;
                                reason_row = 2;
                            }
                        }
                        break;
                    case 1:
                        if (userdata_gunner_ck(lobby_world_block) == 0) {
                            enabled = 0;
                            if (self->menu_cursor_0x008 == i) {
                                reason = 0xB2;
                                reason_row = 2;
                            }
                        }
                        break;
                    }
                    break;
                case 4:
                    if (i == 5 && lb_dye_part_ck((::_PLW*)self->page_0x010, 0) == 0) {
                        enabled = 0;
                        if (self->menu_cursor_0x008 == i) {
                            reason = 0xA2;
                            reason_row = 2;
                        }
                    }
                    break;
                }
                if (self->menu_cursor_0x008 == i) {
                    selected = 1;
                    lit = 1;
                } else {
                    selected = 0;
                    lit = 0;
                }
                color = GetMenuFontColor(enabled, lit, 1, 0);
                sprite_frame_apply(&spr, *sub_panels, self->menu_row_0x006, &pos);
                pos.x += base.x;
                pos.y += base.y;
                if ((u32)self->menu_row_0x006 <= 1) {
                    if (i == 1) {
                        if (userdata_gunner_ck(lobby_world_block) == 1) {
                            text = (s8*)LbStr(0, 0x21B);
                        } else {
                            text = (s8*)LbStr(0, 0x21C);
                        }
                    } else {
                        text = (s8*)LbStr(0, i + str_bases[self->menu_row_0x006]);
                    }
                } else {
                    text = (s8*)LbStr(0, i + str_bases[self->menu_row_0x006]);
                }
                lb_chg::lb_list_row_draw((s32)text, selected, (s16*)&pos, color, 6);
                sub_panels++;
            }
        }
        lb_panel_msg_draw(0x1877, fn_801F5534((LbPage*)self));
        if (reason != -1) {
            lb_panel_line_draw(0x1877, reason, 2, reason_row);
        }
    }
    if (self->list_kind_0x002 == 0) {
        lb_box_guide_draw(0x1AB0, 0x1AB1, 0x1ACC, closing, self->changed_0x004);
    }
}

/* 0x801F59B8 (0x434): the item lists of the box: one pouch page of eight rows (the parts mask picks which rows are
 * live, marked or greyed), the page arrows and the picked row's details, or the sort question; returns 1 while the
 * question is up. */
u8 lb_box_lists_draw(::LbListPane* self, u16 parts)
{
    _mh_ivec2_ pos;
    MenuListEntry entries[8];
    MenuListEntry* entry;
    u8 page;
    u8 sort_sorted;
    u16 mode;
    s32 sorting;
    s32 marking;
    s32 i;
    s16 index;
    s16 marked;
    s16 row;
    u32 flags;
    u8 side;
    u8 pages;
    ::IdValue* pouch;

    marked = -1;
    flags = 0x800;
    mode = 0;
    if ((parts & 1) != 0) {
        side = self->side_0x005 == 0;
        page = self->column_sub_0x01A;
        pages = self->column_sub_max_0x01B;
        row = self->column_0x018;
    } else {
        side = self->side_0x005;
        page = self->row_sub_0x016;
        pages = self->row_sub_max_0x017;
        row = self->row_0x014;
    }
    get_lsp_data(0x1BC5, &pos);
    if (side == 1) {
        if (page + 1 == pages) {
            flags = 0x800 | 1;
        }
        draw_sprite_anim_ary(lb_box_pouch_tabs, 1, &pos);
    } else {
        draw_sprite_anim_ary(lb_box_pouch_tabs, 0, &pos);
    }
    sorting = parts & 2;
    if (sorting != 0) {
        flags |= 0x200;
        mode = 3;
    }
    memset(entries, 0, sizeof(entries));
    entry = entries;
    index = page * 8;
    marking = parts & 4;
    for (i = 0; i < 8; i++) {
        entry->present_0x02 = 1;
        entry->field_0x01 = 0;
        if (marking != 0) {
            if (i == row) {
                entry->field_0x01 = 1;
                marked = index;
            } else {
                entry->field_0x01 = 0;
            }
        } else {
            entry->field_0x01 = 0;
        }
        if ((parts & 8) != 0) {
            if (index == self->entry_max_0x022) {
                entry->present_0x02 = 1;
                marked = index;
            } else {
                entry->present_0x02 = 0;
            }
        } else if ((parts & 0x10) != 0) {
            if (fn_801EDA9C(self, index, self->item_id_0x024) == 0) {
                entry->present_0x02 = 1;
            } else {
                entry->present_0x02 = 0;
            }
        } else if ((parts & 0x20) != 0) {
            if (fn_801EDA34(index, self->side_0x005) > 0) {
                entry->present_0x02 = 1;
            } else {
                entry->present_0x02 = 0;
            }
        } else if ((parts & 0x40) != 0) {
            if (fn_801EDB34(self, index, self->item_id_0x024) == 0) {
                entry->present_0x02 = 1;
            } else {
                entry->present_0x02 = 0;
            }
        } else if ((parts & 0x80) != 0) {
            if (fn_801ED184(index) > 0) {
                entry->present_0x02 = 1;
            } else {
                entry->present_0x02 = 0;
            }
        } else if ((parts & 0x100) != 0) {
            if (fn_801ED200(index, self->side_0x005) > 0) {
                entry->present_0x02 = 1;
            } else {
                entry->present_0x02 = 0;
            }
        } else {
            entry->present_0x02 = 1;
        }
        entry->index_0x06 = index;
        entry++;
        index++;
    }
    if (mode == 0) {
        if ((self->move_0x01C & 4) != 0) {
            flags |= 8;
        }
        if ((self->move_0x01C & 8) != 0) {
            flags |= 0x10;
        }
    }
    pouch = userdata_pouch_get((::LbItemDb*)lobby_world_block, side);
    if (self->active_0x026 == 0) {
        get_lsp_data(0x1BC4, &pos);
        menu_frame_draw_blocks(pouch, entries, page + 1, pages, mode, flags, &pos);
        if (marking != 0 && sorting == 0) {
            if (get_option_cfg(7) == 0) {
                if (self->move_max_0x01E == 0) {
                    put_button_guide_b_idx(0x1BCF, 3);
                } else if (self->move_max_0x01E == 1) {
                    draw_lsp_sprite_variant(0x1BCF, 4);
                }
            } else {
                if (self->move_max_0x01E == 0) {
                    put_button_guide_b_idx(0x1BCF, 0xC);
                } else if (self->move_max_0x01E == 1) {
                    draw_lsp_sprite_variant(0x1BCF, 0xC);
                }
            }
        }
        if (marked != -1) {
            menu_item_row_draw_by_lsp(0x1B30, (u16*)&userdata_pouch_get((::LbItemDb*)lobby_world_block, side)[marked]);
        }
        return 0;
    }
    menu_frame_entries_build(side == 1 ? (u16*)&pouch[0x18] : NULL, NULL, pages, 0x1BC4, 1);
    lb_panel_msg_draw(0x1877, 0xC5);
    lb_panel_yes_no_draw(0x1879, self->stepper_0x02C);
    return 1;
}

/* 0x801F5DEC (0x1C4): the count box: the frame, the left/right arrows (lit while their count is non-negative, else
 * the pulse), and the two counts. */
void lb_count_box_draw(const _mh_ivec2_* unused, u8 value, u8 max, u16 flags, u16 frame, s16 pulse, s16 left,
                       s16 right)
{
    u32 color;
    u16 pulse_b;
    u16 arrow_r;
    u16 arrow_l;
    _mh_ivec2_ pos;
    u16 pulse_a;
    u16 shown;
    _SPR_DATA_ spr;

    color = 0;
    get_lsp_data(0x1B20, &pos);
    draw_sprite_anim_ary(lb_count_box_frame, frame, &pos);
    if (chk_pointer() == 0 && (flags & 8) == 0) {
        if (get_option_cfg(7) == 0) {
            pulse_a = 0x1B22;
            pulse_b = 0x1B23;
            arrow_l = 0x1B26;
            arrow_r = 0x1B27;
        } else {
            pulse_a = 0x1B24;
            pulse_b = 0x1B25;
            arrow_l = 0x1B28;
            arrow_r = 0x1B29;
        }
        if (left < 0) {
            shown = 0;
        } else {
            shown = left;
            pulse = -1;
        }
        draw_sprite_anim_idx(arrow_l, shown, &pos);
        if (right < 0) {
            shown = 0;
        } else {
            shown = right;
            pulse = -1;
        }
        draw_sprite_anim_idx(arrow_r, shown, &pos);
        if (pulse >= 0) {
            set_blendmode(4, 1, 1);
            draw_sprite_anim_idx(pulse_a, pulse, &pos);
            draw_sprite_anim_idx(pulse_b, pulse, &pos);
            set_blendmode(4, 5, 1);
        }
        if ((flags & 3) != 0) {
            sprite_frame_apply(&spr, 22, (u16)(frame != 0 ? frame : 1), NULL);
            color = spr.color_0x1C;
        }
    }
    draw_number_anim(0x1B2A, frame, max, 0, &pos);
    draw_number_anim(0x1B2B, frame, value, color, &pos);
}

/* 0x801F5FB0 - draw a page arrow at the given screen position. */
void fn_801F5FB0(s16 a, s16 b, u16 c, u8 kind)
{
    _mh_ivec2_ pos;

    if (kind == 1) {
        get_lsp_data(6877, &pos);
    } else {
        get_lsp_data(6862, &pos);
    }
    put_page_count(lbl_805B88D4, a, b, c, &pos);
}

/* 0x801F6034 - draws one entry's icon with the item icons' add-blend, and the cursor under it on request (kept out of
 * line: `fn_801F60D4` calls it). */
__declspec(noinline) void fn_801F6034(const _mh_ivec2_* pos, u32 id, u8 flag)
{
    if ((s16)id != -1 && (u32)chk_pointer() == 0) {
        set_blendmode(4, 1, 1);
        draw_sprite_anim_idx(6941, (u16)id, pos);
        set_blendmode(4, 5, 1);
    }
    if (flag == 1) {
        draw_sprite_idx(6942, pos);
    }
}

/* 0x801F60D4 - draw one item icon through the shared sprite block. */
void fn_801F60D4(const _mh_ivec2_* pos, u32 id, s16 sel, u32 flag, u32 extra)
{
    _SPR_DATA_ spr;

    fn_801F6034(pos, (s16)sel, (u8)flag);
    if ((u16)id != 0) {
        sprite_frame_apply(&spr, 6943, (u8)extra, 0);
        if ((u8)extra != 0) {
            spr.color_0x1C = 0x505050FF;
        }
        draw_itemicon_item_id(spr, (u16)id, pos);
    }
}


/* 0x801F6168 (0x934): the item box grid: one page of 10 x 10 slots, each greyed or lit by the pane's kind (pick,
 * combine, swap, sell or the plain view), the picked slot's row and details, the count box and the help line. */
void lb_box_grid_draw(::LbDigitPane* digits, ::MenuSelCursor* cursor, const _mh_ivec2_* base, u8 kind, s16 value,
                      ::IdValue* items, ::_PLW* plw)
{
    _SPR_DATA_ spr;
    s16 reason;
    s16 count;
    u8 counts;
    u8 selected;
    const u16* cols;
    _mh_ivec2_ cell;
    ::IdValue* slot;
    _mh_ivec2_ row_pos;
    _mh_ivec2_ pos;
    s16 right;
    const u16* rows;
    s32 index;
    s16 row;
    s16 marked;
    s32 marking;
    s16 picked;
    s32 c;
    s16 pulse;
    u8 confirm;
    s16 help;
    u8 enabled;
    ::IdValue* listed;
    u8 dim;
    s16 mark;
    s16 left;
    ::IdValue* pick_slot;
    u16 flags;
    _SPR_DATA_* uv;
    u8 page;
    u32 base_color;
    u8 guide;
    u8 pages;
    _mh_ivec2_ col;

    enabled = 1;
    selected = 0;
    help = -1;
    reason = -1;
    flags = 0;
    guide = 1;
    counts = 1;
    marking = 1;
    draw_sprite_ary(lb_box_grid_frame, base);
    switch (kind) {
    case 5:
        picked = -1;
        if (value != -1) {
            page = value / 100;
        } else {
            page = 0;
        }
        pages = menu_page_count(userdata_box_capacity((::LbItemDb*)lobby_world_block), 100);
        break;
    default:
        picked = fn_801ED160(digits);
        pulse = digits->anim_0x0E;
        page = digits->digit_hundreds_0x02;
        pages = digits->digit_hundreds_max_0x03;
        break;
    }
    uv = (_SPR_DATA_*)get_lsp_data(lb_box_grid_cols[0], &cell);
    spr_data_copy((s16*)&spr, get_lsp_data(lb_box_grid_cols[8], NULL));
    uv_pair_copy(&spr, uv);
    base_color = spr.color_0x1C;
    if (digits != NULL && digits->active_0x1C != 0) {
        confirm = 1;
        counts = 0;
        page = 0;
        index = 0;
    } else {
        confirm = 0;
        index = page * 100;
    }
    row = 0;
    rows = lb_box_grid_rows;
    listed = &items[index];
    do {
        get_lsp_data(*rows, &row_pos);
        cols = lb_box_grid_cols;
        for (c = 0; c < 10; c++) {
            if (confirm != 0) {
                slot = listed;
                mark = -1;
            } else {
                slot = &((::LbItemDb*)lobby_world_block)->slots_0x180[index];
                switch (kind) {
                case 1:
                    enabled = fn_801ED9E4(index) > 0;
                    if (index == picked) {
                        marked = index;
                        if (slot->id != 0) {
                            if (enabled == 0) {
                                count = userdata_item_slot_count((::LbItemDb*)lobby_world_block);
                                if (item_pair_index_find(slot->id,
                                                         userdata_equip_item_slots_get((::LbItemDb*)lobby_world_block),
                                                         count) >= 0) {
                                    reason = 0xCB;
                                } else {
                                    reason = 0xC6;
                                }
                            } else if (pl_item_pouch_ok_ck(plw, slot->id) == 0) {
                                reason = 0xD0;
                            }
                        }
                    }
                    break;
                case 4:
                    if (digits->phase_0x00 == 0) {
                        if (index == picked) {
                            marked = index;
                        }
                        if (slot->id == 0) {
                            enabled = 0;
                        } else {
                            enabled = 1;
                        }
                    } else {
                        marking = 0;
                        if (index == picked) {
                            enabled = 1;
                            marked = index;
                        } else {
                            enabled = 0;
                        }
                    }
                    break;
                case 2:
                    if (index == picked) {
                        marked = index;
                    }
                    if ((u32)(digits->phase_0x00 - 2) > 2) {
                        switch (digits->phase_0x00) {
                        default:
                            enabled = fn_801EE900(slot->id) == 1;
                            selected = 0;
                            break;
                        case 1:
                            enabled = fn_801EE940(cursor->item_a_0x04, slot->id) == 1;
                            if (slot->id != 0 && slot->id == cursor->item_a_0x04) {
                                selected = 1;
                            } else {
                                selected = 0;
                            }
                            break;
                        }
                    } else {
                        marking = 0;
                        if (slot->id == cursor->item_a_0x04 || slot->id == cursor->item_b_0x06) {
                            selected = 1;
                            enabled = 1;
                        } else {
                            enabled = 0;
                            selected = 0;
                        }
                    }
                    break;
                case 3:
                    if (index == picked) {
                        marked = index;
                    }
                    switch (digits->phase_0x00) {
                    default:
                        selected = 0;
                        break;
                    case 1:
                        if (((::LbItemDb*)lobby_world_block)->slots_0x180[digits->cursor_0x1A].id == 0 && slot->id == 0) {
                            enabled = 0;
                        } else {
                            enabled = 1;
                        }
                        if (index == digits->cursor_0x1A) {
                            selected = 1;
                            enabled = 1;
                        } else {
                            selected = 0;
                        }
                        break;
                    }
                    break;
                case 5:
                    if (index == value) {
                        enabled = 1;
                        marked = index;
                    } else {
                        enabled = 0;
                    }
                    break;
                }
                if (marking != 0 && index == picked) {
                    mark = pulse;
                } else {
                    mark = -1;
                }
            }
            if (enabled == 0) {
                dim = 3;
                spr.color_0x1C = color_mult(base_color, 0x323232FF);
            } else {
                dim = 0;
                spr.color_0x1C = color_mult(base_color, 0x888888FF);
            }
            get_lsp_data(*cols, &cell);
            pos.x = cell.x + (row_pos.x + base->x);
            pos.y = cell.y + (row_pos.y + base->y);
            draw_sprite(spr, &pos);
            fn_801F60D4(&pos, slot->id, mark, selected, dim);
            cols++;
            listed++;
            index++;
        }
        rows++;
        row++;
    } while (row < 10);
    if (confirm == 0 && (u32)(kind - 1) <= 4) {
        pick_slot = &((::LbItemDb*)lobby_world_block)->slots_0x180[marked];
        if (kind != 2 && kind != 5) {
            menu_item_row_draw_by_lsp(0x1B30, (u16*)pick_slot);
        }
        get_lsp_data(0x1AE0, &pos);
        lb_item_info_draw(pick_slot->id, pick_slot->value, 99, kind == 4, &pos);
    }
    draw_sprite_ary(lb_box_grid_overlay, base);
    draw_sprite_anim_ary(lb_box_grid_arrow_l, 0, base);
    draw_sprite_anim_ary(lb_box_grid_arrow_r, 0, base);
    switch (kind) {
    case 4:
        if (digits->phase_0x00 != 0) {
            counts = 0;
            guide = 0;
        }
        break;
    case 3:
        switch (digits->phase_0x00) {
        case 1:
            guide = 0;
            break;
        }
        break;
    case 2:
        switch (digits->phase_0x00) {
        case 0:
            break;
        case 1:
            guide = 0;
            break;
        default:
            counts = 0;
            guide = 0;
            break;
        }
        break;
    }
    if (digits != NULL && counts != 0) {
        if ((digits->flags_0x08 & 0x100) != 0) {
            flags |= 1;
        }
        if ((digits->flags_0x08 & 0x200) != 0) {
            flags |= 2;
        }
        left = digits->timer_0x10;
        right = digits->blink_a_0x12;
        count = digits->blink_b_0x14;
    } else {
        flags |= 8;
        left = -1;
        right = -1;
        count = -1;
        guide = 0;
    }
    lb_count_box_draw(base, page + 1, pages, flags, 0, left, right, count);
    menu_money_draw(0x1B2F);
    font_flush();
    if (confirm == 1) {
        help = 0xBC;
        lb_panel_yes_no_draw(0x1893, digits->stepper_0x04);
    } else if (digits != NULL) {
        switch (kind) {
        case 2:
            switch (digits->phase_0x00) {
            case 0:
                help = 0xBD;
                break;
            case 1:
                help = 0xBE;
                break;
            }
            break;
        case 3:
            switch (digits->phase_0x00) {
            case 0:
                help = 0xC1;
                break;
            case 1:
                help = 0xC2;
                break;
            }
            break;
        case 4:
            switch (digits->phase_0x00) {
            case 0:
                help = 0xC3;
                break;
            case 1:
                help = 0xC4;
                lb_count_window_draw(0x1ADE, 0x1ADF, digits->pick_count_0x18, digits->pick_max_0x16,
                                     eft052_item_value_get(slot->id), 0, digits->flags_0x08);
                break;
            case 2:
                lb_panel_yes_no_draw(0x1893, digits->stepper_0x04);
                if (slot->id != 0) {
                    lb_help_num_put_c1(0x1878, 0x36, digits->pick_count_0x18 * eft052_item_value_get(slot->id));
                }
                break;
            }
            break;
        }
    }
    if (guide != 0) {
        if (get_option_cfg(7) == 0) {
            draw_lsp_sprite_variant(0x1B32, 4);
        } else {
            draw_lsp_sprite_variant(0x1B32, 0xC);
        }
    }
    if (help != -1) {
        lb_panel_msg_draw(0x1878, help);
    }
    if (reason != -1) {
        lb_panel_str_print(0x1878, reason, 2);
    }
}

/* 0x801F6A9C - run the page's entry walk with the row and kind mask the caller names. */
void fn_801F6A9C(LbPageOwner* self, const _mh_ivec2_* pos, u32 flags)
{
    lb_box_grid_draw((::LbDigitPane*)self->slots_0x054, (::MenuSelCursor*)self->rows_0x030, pos, (u8)flags, -1,
                     (::IdValue*)self->tail_0x3B8, (::_PLW*)self->page_0x010);
}

/* 0x801F6AC8 - run the page's entry walk from its head, in the "no page" state. */
void lb_page_walk_head(const _mh_ivec2_* pos, s16 row)
{
    lb_box_grid_draw(NULL, NULL, pos, 5, row, NULL, NULL);
}

/* 0x801F6AEC (0x1F8): the combine pane: the two picked items' cells (the active one lit), then the result row and,
 * at the confirm step, the question. */
void lb_box_combine_draw(::LbDigitPane* digits, ::MenuSelCursor* cursor)
{
    _mh_ivec2_ frame;
    _mh_ivec2_ pos;
    const u16* table;
    s32 has;
    s32 active;
    s32 dim;

    get_lsp_data(0x1B36, &frame);
    draw_sprite_anim_ary(get_menu_lsp_tbl(0x36), cursor->blend_0x20, &frame);
    get_lsp_data(0x1B34, &pos);
    draw_sprite_anim_ary(get_menu_lsp_tbl(0x37), cursor->blend_0x20, &pos);
    has = (u16)cursor->slot_a_0x08 != (u16)0xFFFF;
    dim = cursor->blend_0x20 == 0;
    if (digits->phase_0x00 == 0) {
        active = 1;
        dim = 1;
    } else {
        active = 0;
    }
    menu_item_cell_draw(&pos, cursor->item_a_0x04,
                        item_slots_count_sum(cursor->item_a_0x04, ((::LbItemDb*)lobby_world_block)->slots_0x180,
                                             userdata_box_capacity((::LbItemDb*)lobby_world_block)),
                        has, active, dim);
    get_lsp_data(0x1B35, &pos);
    if (digits->phase_0x00 >= 1) {
        table = get_menu_lsp_tbl(0x37);
    } else {
        table = get_menu_lsp_tbl(0x38);
    }
    draw_sprite_anim_ary(table, cursor->blend_0x20, &pos);
    has = (u16)cursor->slot_b_0x0A != (u16)0xFFFF;
    if (digits->phase_0x00 == 1) {
        active = 1;
        dim = 1;
    } else {
        active = 0;
    }
    menu_item_cell_draw(&pos, cursor->item_b_0x06,
                        item_slots_count_sum(cursor->item_b_0x06, ((::LbItemDb*)lobby_world_block)->slots_0x180,
                                             userdata_box_capacity((::LbItemDb*)lobby_world_block)),
                        has, active, dim);
    draw_sprite_anim_idx(0x374, cursor->blend_0x20, &frame);
    if (digits->phase_0x00 >= 2) {
        menu_sel_cursor_result_draw(cursor, lb_combine_result_rows);
    }
    if (digits->phase_0x00 == 2) {
        menu_sel_cursor_confirm_draw(cursor, digits->stepper_0x04, 0x1B63);
    }
}

} /* extern "C" */
extern "C" {

/* 0x801F6CE4 (0xD8): one equipment cell: the cursor icon, the record's icon (dimmed on request) and, on request, the
 * highlight frame around it. */
void lb_equip_icon_cell_draw(const _mh_ivec2_* pos, ::LbEquipRec* rec, u8 framed, s16 cursor, u8 cursor_flag, u8 dim)
{
    _SPR_DATA_ spr;

    fn_801F6034(pos, cursor, cursor_flag);
    if ((u8)equip_kind_table_class(rec->kind_0x00) != 0xFF) {
        sprite_frame_apply(&spr, 6943, dim, NULL);
        if (dim != 0) {
            spr.color_0x1C = 0x505050FF;
        }
        draw_equipicon(spr, (::_EQUIP*)rec, pos);
        if (framed != 0) {
            draw_frame_rect_z(pos->x, pos->y, spr.width, spr.height, 95, 0xFFC400FF, 0.0f);
        }
    }
}


/* 0x801F6DBC (0x1390): the equipment box's entry pane: the title list while the list button is held, else the box
 * page of 10 x 10 records (each lit or greyed by the slot kind, worn ones marked), the picked record's cell and
 * scroll list, the count box, the open sort menu, and the help line with the reason a pick is refused. */
void lb_pane_draw(::LbEntryPane* self, s16 x, s16 y, LbBoxMenu* owner)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ base;
    _mh_ivec2_ row_pos;
    _mh_ivec2_ cell;
    _mh_ivec2_ pos;
    u8 kakou_icon[0x10];
    u8 kakou_table[0x10];
    const u16* rows;
    const u16* cols;
    const u16* titles;
    const u16* classes;
    ::LbEquipRec* rec;
    _SPR_DATA_* uv;
    s32 sorting;
    s32 active;
    s32 c;
    s32 slot;
    u32 kind;
    s16 index;
    s16 row;
    s16 page;
    s16 picked;
    s16 help;
    s16 reason;
    s16 line;
    u16 frame;
    s16 selected;
    u16 flags;
    u16 dim;
    u8 enabled;
    u8 marked;
    u8 worn;
    u8 has_pick;
    u8 guide;
    u8 counts;

    enabled = 1;
    marked = 0;
    counts = 1;
    guide = 1;
    flags = 0;
    kind = lb_entry_pane_scroll_kind_get(self);
    if ((u8)kind == 7) {
        if (self->held_0x020 == 0) {
            lb_box_guide_draw(0x1AB0, 0x1AB1, 0x1ACC, 1, owner != NULL ? owner->changed_0x004 : 0);
            return;
        }
        lb_title_frame_draw(&self->list_0x02C, lb_entry_pane_kind_tbl_get(self));
        return;
    }
    if (self->sort_step_0x023 == 0xFF) {
        sorting = 1;
        active = 1;
        frame = 0;
    } else if (self->held_0x020 != 0 || self->sort_step_0x023 != 0) {
        sorting = 0;
        active = 0;
        frame = 3;
    } else {
        sorting = 0;
        active = 1;
        frame = 0;
    }
    switch (self->slot_kind_0x016) {
    case 4:
        if (self->phase_0x000 != 0) {
            counts = 0;
            guide = 0;
        }
        break;
    case 5:
        switch (self->phase_0x000) {
        case 1:
            guide = 0;
            break;
        }
        break;
    }
    if ((u8)kind == 4 || (u8)kind == 10) {
        picked = lb_digit_pane_value(self);
        has_pick = 1;
        sorting = 0;
        if (active == 0) {
            guide = 0;
        }
    } else {
        base.x = x;
        base.y = y;
        draw_sprite_anim_ary(lb_box_grid_frame, frame, &base);
        uv = (_SPR_DATA_*)get_lsp_data(lb_box_grid_cols[0], &cell);
        spr_data_copy((s16*)&spr, get_lsp_data(lb_box_grid_cols[9], NULL));
        uv_pair_copy(&spr, uv);
        if (self->slot_kind_0x016 == 6) {
            picked = self->set_ids_0x018[0];
            selected = -1;
            if (picked != -1) {
                page = picked / 100;
            } else {
                page = 0;
            }
            has_pick = 0;
        } else {
            selected = lb_digit_pane_value(self);
            picked = selected;
            page = self->digit_hundreds_0x002;
            has_pick = 1;
        }
        if (sorting == 0) {
            index = page * 100;
        }
        if (sorting != 0) {
            page = 0;
        }
        row = 0;
        rows = lb_box_grid_rows;
        slot = 0;
        do {
            get_lsp_data(*rows, &row_pos);
            cols = lb_box_grid_cols;
            for (c = 0; c < 10; c++) {
                if (sorting != 0) {
                    index = self->sorted_ids_0x16C[slot + c];
                }
                rec = &((::LbItemDb*)lobby_world_block)->equip_0x0E00[index];
                worn = userdata_equip_box_worn_ck((::LbItemDb*)lobby_world_block, index) == 1;
                if (sorting == 0) {
                    switch (self->slot_kind_0x016) {
                    case 0:
                        if ((u8)equip_kind_table_class(rec->kind_0x00) == 0) {
                            if (equip_wearable_ck(lobby_world_block, (::_EQUIP*)rec) == 1) {
                                enabled = 1;
                            } else {
                                enabled = 0;
                            }
                        } else if ((u32)(rec->kind_0x00 - 0xB) <= 2) {
                            enabled = 1;
                        } else if ((u8)equip_kind_table_class(rec->kind_0x00) == 1 && worn == 0) {
                            enabled = 1;
                        } else {
                            enabled = 0;
                        }
                        break;
                    case 1:
                        enabled = rec->kind_0x00 == 0xB;
                        if (index == self->set_ids_0x018[0] || index == self->set_ids_0x018[1]
                            || index == self->set_ids_0x018[2]) {
                            marked = 1;
                        } else {
                            marked = 0;
                        }
                        break;
                    case 2:
                        enabled = rec->kind_0x00 == 0xC;
                        if (index == self->set_ids_0x018[0] || index == self->set_ids_0x018[1]
                            || index == self->set_ids_0x018[2]) {
                            marked = 1;
                        } else {
                            marked = 0;
                        }
                        break;
                    case 3:
                        enabled = rec->kind_0x00 == 0xD;
                        if (index == self->set_ids_0x018[0] || index == self->set_ids_0x018[1]
                            || index == self->set_ids_0x018[2]) {
                            marked = 1;
                        } else {
                            marked = 0;
                        }
                        break;
                    case 5:
                        switch (self->phase_0x000) {
                        default:
                            marked = 0;
                            break;
                        case 1:
                            if (rec->kind_0x00 == 0
                                && ((::LbItemDb*)lobby_world_block)->equip_0x0E00[self->mark_0x01E].kind_0x00 == 0) {
                                enabled = 0;
                            } else {
                                enabled = 1;
                            }
                            if (index == self->mark_0x01E) {
                                marked = 1;
                                enabled = 1;
                            } else {
                                marked = 0;
                            }
                            break;
                        }
                        break;
                    case 6:
                        if (index == self->set_ids_0x018[0] || index == self->set_ids_0x018[1]
                            || index == self->set_ids_0x018[2]) {
                            enabled = 1;
                        } else {
                            enabled = 0;
                        }
                        break;
                    case 4:
                        if (worn == 1 || (u8)equip_kind_table_class(rec->kind_0x00) == 0xFF) {
                            enabled = 0;
                        } else {
                            enabled = 1;
                        }
                        break;
                    case 7:
                        if ((u8)equip_kind_table_class(rec->kind_0x00) == 0xFF) {
                            enabled = 0;
                        } else if (lb_kakou_icon_build((::_EQUIP*)rec, 0, kakou_icon, kakou_table) == 0) {
                            enabled = 1;
                        } else {
                            enabled = 0;
                        }
                        break;
                    case 8:
                        if ((u8)equip_kind_table_class(rec->kind_0x00) == 0xFF) {
                            enabled = 0;
                        } else if (lb_kakou_icon_build((::_EQUIP*)rec, 1, kakou_icon, kakou_table) == 0) {
                            enabled = 1;
                        } else {
                            enabled = 0;
                        }
                        break;
                    case 9:
                        if ((u8)equip_kind_table_class(rec->kind_0x00) == 0xFF) {
                            enabled = 0;
                        } else if (equip_deco_slots_free((::_EQUIP*)rec) > 0) {
                            enabled = 1;
                        } else {
                            enabled = 0;
                        }
                        break;
                    case 10:
                        if ((u8)equip_kind_table_class(rec->kind_0x00) == 0xFF) {
                            enabled = 0;
                        } else {
                            enabled = equip_deco_count((::_EQUIP*)rec) > 0;
                        }
                        break;
                    }
                }
                if (enabled == 0 || active == 0) {
                    dim = 3;
                } else {
                    dim = 0;
                }
                get_lsp_data(*cols, &cell);
                pos.x = cell.x + (row_pos.x + base.x);
                pos.y = cell.y + (row_pos.y + base.y);
                spr_anim_frame_set(&spr, spr.anim_0x20, dim, NULL);
                if (dim != 0) {
                    if (worn != 0) {
                        spr.color_0x1C = color_mult(0x3C68B8FF, 0xB4B4B4FF);
                    } else {
                        spr.color_0x1C = color_mult(spr.color_0x1C, 0x323232FF);
                    }
                } else if (worn != 0) {
                    spr.color_0x1C = 0x3C68B8FF;
                } else {
                    spr.color_0x1C = color_mult(spr.color_0x1C, 0x7E7E7EFF);
                }
                draw_sprite(spr, &pos);
                if (index == selected && self->held_0x020 == 0 && sorting == 0) {
                    lb_equip_icon_cell_draw(&pos, rec, worn, self->anim_0x00E, marked, dim);
                } else {
                    lb_equip_icon_cell_draw(&pos, rec, worn, -1, marked, dim);
                }
                if (sorting == 0) {
                    index++;
                }
                cols++;
            }
            rows++;
            slot += 10;
            row++;
        } while (row < 10);
        if (picked != -1 && sorting == 0) {
            rec = &((::LbItemDb*)lobby_world_block)->equip_0x0E00[picked];
            get_lsp_data(0x1AE0, &pos);
            lb_equip_cell_draw(0x1AE0, (::_EQUIP*)rec, self->slot_kind_0x016 == 4 ? 2 : 0, self->slot_kind_0x016);
            font_flush();
        }
        frame = -(active == 0) & 3;
        if (active == 0) {
            draw_sprite_ary(lb_box_grid_lock, &base);
        }
        draw_sprite_anim_ary(lb_box_grid_overlay, frame, &base);
        draw_sprite_anim_ary(lb_box_grid_arrow_l, 1, &base);
        draw_sprite_anim_ary(lb_box_grid_arrow_r, 1, &base);
        if (active == 0) {
            draw_sprite_ary(lb_box_grid_lock_b, &base);
        }
        menu_money_draw(0x1B2F);
        if (self->slot_kind_0x016 != 6 && counts == 1 && active != 0) {
            if ((self->flags_0x008 & 0x100) != 0) {
                flags |= 1;
            }
            if ((self->flags_0x008 & 0x200) != 0) {
                flags |= 2;
            }
            if ((u32)sorting == 1) {
                flags |= 8;
            }
        } else {
            flags |= 8;
        }
        lb_count_box_draw(&base, page + 1, self->digit_hundreds_max_0x003, flags, frame, self->timer_0x010,
                          self->blink_a_0x012, self->blink_b_0x014);
    }
    if (picked != -1 && sorting == 0) {
        if (has_pick != 0) {
            rec = &((::LbItemDb*)lobby_world_block)->equip_0x0E00[picked];
            if (rec->kind_0x00 != 0) {
                lb_entry_pane_scroll_refresh(self);
                switch (self->slot_kind_0x016) {
                case 7:
                    self->scroll_0x04C.kakou_0x28 = lb_kakou_icon_build((::_EQUIP*)rec, 0, kakou_icon, kakou_table);
                    self->scroll_0x04C.kakou_icon_0x2C = kakou_icon;
                    break;
                case 8:
                    self->scroll_0x04C.kakou_0x28 = lb_kakou_icon_build((::_EQUIP*)rec, 1, kakou_icon, kakou_table);
                    self->scroll_0x04C.kakou_icon_0x2C = kakou_icon;
                    break;
                default:
                    self->scroll_0x04C.kakou_0x28 = 0;
                    self->scroll_0x04C.kakou_icon_0x2C = NULL;
                    break;
                }
                if (active == 0 || counts == 0) {
                    lb_equip_page_draw((struct LbScrollWork*)&self->scroll_0x04C, 0);
                } else {
                    lb_equip_page_draw((struct LbScrollWork*)&self->scroll_0x04C, 1);
                }
            }
        }
        font_flush();
    }
    if ((flags & 8) == 0 && guide == 1) {
        if (get_option_cfg(7) == 0) {
            if ((u8)kind != 4 && (u8)kind != 10) {
                draw_lsp_sprite_variant(0x1B32, 4);
                draw_lsp_sprite_variant(0x1BCE, 0);
            } else {
                draw_lsp_sprite_variant(0x694, 0);
            }
        } else if ((u8)kind != 4 && (u8)kind != 10) {
            draw_lsp_sprite_variant(0x1B32, 0xC);
            draw_lsp_sprite_variant(0x1BCE, 0xD);
        } else {
            draw_lsp_sprite_variant(0x694, 0xD);
        }
    }
    help = -1;
    reason = -1;
    line = -1;
    if (self->held_0x020 != 0) {
        lb_title_frame_draw(&self->list_0x02C, lb_entry_pane_kind_tbl_get(self));
        help = 0xD9;
    } else {
        get_lsp_data(0x1968, &pos);
        switch (self->sort_step_0x023) {
        case 1:
            lb_choice_draw_c1((u8*)&self->sort_lists_0x0CC[0], 0x1968, lb_sort_menu_frame, lb_sort_menu_rows, 0, 0);
            draw_sprite_idx(0x196B, &pos);
            help = 0xEA;
            break;
        case 2:
            help = 0xEA;
            switch (self->sort_lists_0x0CC[0].cursor_0x00) {
            case 1:
                titles = lb_sort_title_kind;
                break;
            case 2:
                titles = lb_sort_title_gun;
                break;
            case 3:
                titles = lb_sort_title_armor;
                break;
            default:
                titles = NULL;
                break;
            }
            lb_list_draw_ex(&self->sort_lists_0x0CC[1], 0x1968, lb_sort_menu_frame, lb_sort_menu_rows, 0,
                            self->sort_sub_0x025 != 0, titles);
            if (self->sort_sub_0x025 == 1) {
                switch (self->sort_lists_0x0CC[0].cursor_0x00) {
                case 1:
                    switch (self->sort_lists_0x0CC[1].cursor_0x00) {
                    case 1:
                        if (userdata_progress_flag_ck(0xF) == 0 && userdata_progress_flag_ck(0xE) == 1) {
                            titles = lb_sort_blade_titles_late;
                        } else {
                            titles = lb_sort_blade_titles;
                        }
                        break;
                    case 2:
                        titles = lb_sort_title_blade_class;
                        break;
                    default:
                        titles = NULL;
                        break;
                    }
                    break;
                case 2:
                    titles = lb_sort_gun_titles;
                    break;
                }
                lb_list_draw_ex(&self->sort_lists_0x0CC[2], 0x1985, lb_sort_sub_frame, lb_sort_sub_rows, 0, 0,
                                titles);
            }
            draw_sprite_idx(0x196B, &pos);
            break;
        case 3:
            help = 0xEA;
            switch (self->sort_lists_0x0CC[0].cursor_0x00) {
            case 1:
                titles = lb_sort_title_order_a;
                switch (self->sort_lists_0x0CC[1].cursor_0x00) {
                case 0:
                    line = 0x103;
                    break;
                case 1:
                    if (userdata_progress_flag_ck(0xF) == 0 && userdata_progress_flag_ck(0xE) == 1U) {
                        classes = lb_sort_blade_classes_late;
                    } else {
                        classes = lb_sort_blade_classes;
                    }
                    switch (classes[self->sort_lists_0x0CC[2].cursor_0x00]) {
                    case 0:
                        line = 0xEC;
                        break;
                    case 1:
                        line = 0xF0;
                        break;
                    case 6:
                        line = 0xF1;
                        break;
                    case 2:
                        line = 0xF2;
                        break;
                    case 4:
                        line = 0xF3;
                        break;
                    case 3:
                        line = 0xF4;
                        break;
                    case 5:
                        line = 0xF5;
                        break;
                    }
                    break;
                case 2:
                    switch (self->sort_lists_0x0CC[2].cursor_0x00) {
                    case 0:
                        line = 0xED;
                        break;
                    case 1:
                        line = 0xF6;
                        break;
                    case 2:
                        line = 0xF7;
                        break;
                    case 3:
                        line = 0xF8;
                        break;
                    }
                    break;
                }
                break;
            case 2:
                titles = lb_sort_title_gun_order;
                switch (self->sort_lists_0x0CC[1].cursor_0x00) {
                case 0:
                    switch (self->sort_lists_0x0CC[2].cursor_0x00) {
                    case 0:
                        line = 0x103;
                        break;
                    case 1:
                        line = 0x104;
                        break;
                    case 2:
                        line = 0x105;
                        break;
                    case 3:
                        line = 0x106;
                        break;
                    case 4:
                        line = 0x107;
                        break;
                    case 5:
                        line = 0x108;
                        break;
                    }
                    break;
                case 1:
                    switch (self->sort_lists_0x0CC[2].cursor_0x00) {
                    case 0:
                        line = 0xEE;
                        break;
                    case 1:
                        line = 0xF9;
                        break;
                    case 2:
                        line = 0xFA;
                        break;
                    case 3:
                        line = 0xFB;
                        break;
                    case 4:
                        line = 0xFC;
                        break;
                    case 5:
                        line = 0xFD;
                        break;
                    }
                    break;
                case 2:
                    switch (self->sort_lists_0x0CC[2].cursor_0x00) {
                    case 0:
                        line = 0xEF;
                        break;
                    case 1:
                        line = 0xFE;
                        break;
                    case 2:
                        line = 0xFF;
                        break;
                    case 3:
                        line = 0x100;
                        break;
                    case 4:
                        line = 0x101;
                        break;
                    case 5:
                        line = 0x102;
                        break;
                    }
                    break;
                }
                break;
            default:
                titles = NULL;
                break;
            }
            reason = line;
            line = -1;
            lb_list_draw_ex(&self->sort_lists_0x0CC[3], 0x1968, lb_sort_menu_frame, lb_sort_menu_rows, 0,
                            self->sort_sub_0x025 != 0, titles);
            if (self->sort_sub_0x025 == 1) {
                switch (self->sort_lists_0x0CC[0].cursor_0x00) {
                case 1:
                    switch (self->sort_lists_0x0CC[3].cursor_0x00) {
                    case 2:
                        titles = lb_sort_order_titles;
                        break;
                    case 3:
                        titles = lb_sort_title_order_b;
                        break;
                    default:
                        titles = NULL;
                        break;
                    }
                    break;
                case 2:
                    titles = lb_sort_gun_order_titles;
                    break;
                }
                lb_list_draw_ex(&self->sort_lists_0x0CC[4], 0x1985, lb_sort_sub_frame, lb_sort_sub_rows, 0, 0,
                                titles);
            }
            draw_sprite_idx(0x196B, &pos);
            break;
        }
    }
    font_flush();
    if (help == -1) {
        if ((u32)sorting == 1) {
            help = 0xD8;
            lb_panel_yes_no_draw(0x1893, self->stepper_0x004);
        } else {
            if (picked != -1) {
                rec = &((::LbItemDb*)lobby_world_block)->equip_0x0E00[picked];
            }
            switch (self->slot_kind_0x016) {
            case 0:
                if (self->phase_0x000 == 0) {
                    help = 0xD1;
                    if ((u32)(rec->kind_0x00 - 0xB) <= 2) {
                        if (userdata_gunner_ck(lobby_world_block) == 0) {
                            reason = 0xE1;
                        }
                    } else {
                        switch ((u8)equip_kind_table_class(rec->kind_0x00)) {
                        case 0:
                            if (userdata_equip_box_worn_ck((::LbItemDb*)lobby_world_block, picked) == 1) {
                                reason = 0xE3;
                            } else if (equip_wearable_ck(lobby_world_block, (::_EQUIP*)rec) == 0) {
                                reason = 0xE9;
                            }
                            break;
                        case 1:
                            if (userdata_equip_box_worn_ck((::LbItemDb*)lobby_world_block, picked) == 1) {
                                reason = 0xE0;
                            } else if (userdata_gunner_ck(lobby_world_block) == 1) {
                                reason = 0xE1;
                            }
                            break;
                        }
                    }
                }
                break;
            case 1:
                help = 0xD4;
                if (rec->kind_0x00 != 0xB && rec->kind_0x00 != 0) {
                    line = 0xE4;
                }
                break;
            case 2:
                help = 0xD5;
                if (rec->kind_0x00 != 0xC && rec->kind_0x00 != 0) {
                    line = 0xE4;
                }
                break;
            case 3:
                help = 0xD6;
                if (rec->kind_0x00 != 0xD && rec->kind_0x00 != 0) {
                    line = 0xE4;
                }
                break;
            case 5:
                switch (self->phase_0x000) {
                case 0:
                    if ((u8)kind == 4 || (u8)kind == 10) {
                        lb_help_text_put(0x1878, (s8*)"");
                    } else {
                        help = 0xDA;
                    }
                    break;
                case 1:
                    help = 0xDB;
                    break;
                }
                break;
            case 4:
                switch (self->phase_0x000) {
                case 0:
                    help = 0xDC;
                    if (userdata_equip_box_worn_ck((::LbItemDb*)lobby_world_block, picked) == 1) {
                        reason = 0xE6;
                    } else if (my_set_record_used_ck(lobby_world_block, picked) == 1
                               || gun_set_record_used_ck(lobby_world_block, picked) == 1) {
                        reason = 0xE5;
                    }
                    break;
                case 1:
                    lb_panel_yes_no_draw(0x1893, self->stepper_0x004);
                    if (picked != -1) {
                        if (equip_deco_count((::_EQUIP*)rec) > 0) {
                            lb_help_num_put_c1(0x1878, 0xDE, equip_sell_price_get((::_EQUIP*)rec));
                        } else {
                            lb_help_num_put_c1(0x1878, 0x36, equip_sell_price_get((::_EQUIP*)rec));
                        }
                    }
                    break;
                }
                break;
            }
        }
    }
    if (help != -1) {
        lb_panel_msg_draw(0x1878, help);
    }
    if (reason != -1) {
        lb_panel_str_print(0x1878, reason, 2);
    }
    if (line != -1) {
        lb_panel_line_draw(0x1878, line, 2, 2);
    }
}

/* 0x801F814C (0x1CC): a gunner set's three cells (bowgun and its two options; an empty option shows the bowgun's own
 * part or a blank one); returns 0 when the bowgun takes no options. */
s32 lb_gun_set_cells_draw(u16* ids, s16 cursor, const _mh_ivec2_* base, u8 mode, u8 mark)
{
    _mh_ivec2_ pos;
    const u16* lsp;
    s16 i;
    ::_EQUIP* gun;
    u32 sel;
    u32 lit;
    ::_EQUIP* rec;
    s32 result;
    ::_EQUIP blank;
    u8 flags;
    s32 options;

    result = 1;
    if (*ids != 0xFFFF) {
        gun = (::_EQUIP*)&((::LbItemDb*)lobby_world_block)->equip_0x0E00[*ids];
        if (Gunner_opt_ok_ck(gun) != 0) {
            options = 0;
        } else {
            options = 1;
        }
    } else {
        options = 0;
    }
    for (i = 0, lsp = lb_gun_set_cell_lsp; i < 3; i++) {
        flags = 0;
        get_lsp_data(*lsp, &pos);
        pos.x += base->x;
        pos.y += base->y;
        sel = i == cursor;
        lit = mark != 1;
        if (*ids != 0xFFFF) {
            rec = (::_EQUIP*)&((::LbItemDb*)lobby_world_block)->equip_0x0E00[*ids];
        } else {
            memset(&blank, 0, sizeof(blank));
            switch (i) {
            default:
                blank.kind = 0xB;
                break;
            case 1:
                blank.kind = 0xC;
                break;
            case 2:
                blank.kind = 0xD;
                break;
            }
            if (options == 0) {
                result = 0;
                blank.item_id = 0;
                if (mode == 0) {
                    flags |= 4;
                }
            } else {
                if (mode == 0) {
                    lit = 0;
                }
                blank.item_id = gun->item_id;
            }
            rec = &blank;
        }
        lb_equip_cell_draw_layout3(rec, 0, &pos, lit, sel, 1, flags, 0);
        lsp++;
        ids++;
    }
    flfntFlush();
    flfntStackReset();
    return result;
}

/* 0x801F8318 (0x344): the set pane: the frame (or the title mark when not the first pane), the set's three cells with
 * the cursor, the scroll list of the set's records and the help line with the reason a slot is refused. */
void lb_set_pane_draw(::LbEquipPane* self, s16 x, s16 y)
{
    _mh_ivec2_ base;
    _mh_ivec2_ title;
    _mh_ivec2_ pos;
    ::LbEquipRec recs[3];
    u8 prompt;
    s16 help;
    s16 reason;
    s16 cursor;

    prompt = 0;
    help = -1;
    reason = -1;
    if (self->side_0x01 == 0) {
        base.x = x;
        base.y = y;
        draw_sprite_anim_ary(lb_gun_set_frame, 0, &base);
        get_lsp_data(0x1BF5, &pos);
        pos.x += base.x;
        pos.y += base.y;
        title.x = pos.x + 8;
        title.y = pos.y;
        draw_sprite_ary(lb_gun_set_title, &title);
        draw_sprite_anim_ary(lb_gun_set_panel, 0, &base);
        draw_sprite_anim_idx(0x1BD7, 0, &base);
    } else {
        pos.x = x;
        pos.y = y;
        lb_anim_mark_draw(0, 1, &pos);
        draw_sprite_ary(lb_gun_set_title, &pos);
    }
    if (self->phase_0x00 == 2) {
        cursor = -1;
        prompt = 0;
    } else {
        cursor = self->index_0x02;
    }
    if (cursor != -1) {
        if (self->index_0x02 == 3) {
            prompt = 1;
            help = 0xD7;
        } else {
            prompt = 0;
            help = 0xD3;
            if (Gunner_opt_ok_ck((::_EQUIP*)&((::LbItemDb*)lobby_world_block)->equip_0x0E00[self->ids_0x04[0]]) == 0
                && (u16)(self->index_0x02 - 1) <= 1) {
                reason = 0xE7;
            }
        }
    }
    if (lb_gun_set_cells_draw(self->ids_0x04, cursor, &pos, 0, 0) == 0) {
        prompt = 2;
        reason = 0xE8;
    }
    if (self->side_0x01 == 1) {
        draw_sprite_idx(0x1A33, &pos);
    }
    get_lsp_data(0x1A31, &pos);
    lb_cursor_prompt_draw(prompt, (s16*)&pos);
    memset(recs, 0, sizeof(recs));
    if (self->ids_0x04[0] != 0xFFFF) {
        equip_record_copy((LbIconRec*)&recs[0], (LbIconRec*)&((::LbItemDb*)lobby_world_block)->equip_0x0E00[self->ids_0x04[0]]);
    } else {
        recs[0].kind_0x00 = 0xB;
        recs[0].value_0x02 = 0;
    }
    if (Gunner_opt_ok_ck((::_EQUIP*)&recs[0]) == 1) {
        if (self->ids_0x04[1] != 0xFFFF) {
            equip_record_copy((LbIconRec*)&recs[1],
                              (LbIconRec*)&((::LbItemDb*)lobby_world_block)->equip_0x0E00[self->ids_0x04[1]]);
        } else {
            recs[1].kind_0x00 = 0xC;
            recs[1].value_0x02 = 0;
        }
        if (self->ids_0x04[2] != 0xFFFF) {
            equip_record_copy((LbIconRec*)&recs[2],
                              (LbIconRec*)&((::LbItemDb*)lobby_world_block)->equip_0x0E00[self->ids_0x04[2]]);
        } else {
            recs[2].kind_0x00 = 0xD;
            recs[2].value_0x02 = 0;
        }
    } else {
        recs[1].kind_0x00 = 0xC;
        recs[1].value_0x02 = recs[0].value_0x02;
        recs[2].kind_0x00 = 0xD;
        recs[2].value_0x02 = recs[0].value_0x02;
    }
    lb_scroll_tables_set(&self->scroll_0x10, NULL, NULL, &recs[0], &recs[1]);
    if (self->phase_0x00 == 2) {
        lb_equip_page_draw((struct LbScrollWork*)&self->scroll_0x10, 0);
        help = 0xDF;
        reason = -1;
        lb_panel_yes_no_draw(0x1879, self->stepper_0x0C);
    } else {
        lb_equip_page_draw((struct LbScrollWork*)&self->scroll_0x10, 1);
    }
    if (help != -1) {
        lb_panel_msg_draw(0x1877, help);
    }
    if (reason != -1) {
        lb_panel_str_print(0x1877, reason, 2);
    }
}

/* 0x801F865C - run the state's own draw call: the entry list for states 0 and 2, the icon strip for
 * state 1. */
void lb_equip_set_pane_draw(::LbEquipPane* self, u32 id_a, void* page_b, u32 id_c, u8* ptr_f)
{
    _mh_ivec2_ pos;

    switch (self->phase_0x00) {
    case 0:
    case 2:
        get_lsp_data((u16)id_a, &pos);
        lb_set_pane_draw(self, pos.x, pos.y);
        break;
    case 1:
        get_lsp_data((u16)id_c, &pos);
        lb_pane_draw((::LbEntryPane*)page_b, pos.x, pos.y, (LbBoxMenu*)ptr_f);
        break;
    }
}

/* 0x801F86FC - set one icon record: copy the world's row for the id, or clear and label it when the id
 * is the "none" marker, then OR the record's bit into the caller's flag word. */
void fn_801F86FC(LbIconRec* recs, u32 id, u32 sel, u32 kind, u32 mode, u16* flags)
{
    LbIconRec* src = lobby_world_block->entries_0x0E00;
    LbIconRec* rec;

    if ((u16)id != 0xFFFF) {
        rec = &recs[(u8)kind];
        equip_record_copy(rec, &src[(u16)id]);
    } else {
        rec = &recs[(u8)kind];
        memset(rec, 0, sizeof(LbIconRec));
        rec->kind_0x00 = (u8)sel;
    }
    if ((u8)mode == 1) {
        *flags |= (u16)(1 << (u8)kind);
    }
}

/* 0x801F87B0 (0x30C): one saved equipment set's detail: the nine records it names (the set's bowgun options dropped
 * when the bowgun takes none), their rows, and for a stored set the status, element and skill pages. */
void lb_my_set_detail_draw(u16 lsp, ::LbMySetPane* pane, u16 weapon, u16 armor_d, u16 armor_e, u16 armor_a,
                           u16 armor_b, u16 armor_c, u16 armor_f, u16 opt_a, u16 opt_b, u8 page_mode, u8 mark)
{
    _mh_ivec2_ pos;
    ::_EQUIP recs[9];
    u16 present;
    ::_PLW* plw;
    bool arrows;
    u8 rows;
    u8 page;

    plw = (::_PLW*)pane->page_0x14;
    arrows = 1;
    present = 0;
    if (mark == 2) {
        mark = 1;
    } else if (weapon != 0xFFFF) {
        mark = 0;
    }
    fn_801F86FC((LbIconRec*)recs, weapon, 7, 6, mark, &present);
    fn_801F86FC((LbIconRec*)recs, armor_e, 5, 4, mark, &present);
    fn_801F86FC((LbIconRec*)recs, armor_a, 1, 0, mark, &present);
    fn_801F86FC((LbIconRec*)recs, armor_b, 2, 1, mark, &present);
    fn_801F86FC((LbIconRec*)recs, armor_c, 3, 2, mark, &present);
    fn_801F86FC((LbIconRec*)recs, armor_d, 4, 3, mark, &present);
    fn_801F86FC((LbIconRec*)recs, armor_f, 6, 5, mark, &present);
    fn_801F86FC((LbIconRec*)recs, opt_a, 0xC, 7, mark, &present);
    fn_801F86FC((LbIconRec*)recs, opt_b, 0xD, 8, mark, &present);
    if (recs[6].kind == 0xB && Gunner_opt_ok_ck(&recs[6]) == 0) {
        recs[7].kind = 0xC;
        recs[7].item_id = recs[6].item_id;
        recs[8].kind = 0xD;
        recs[8].item_id = recs[6].item_id;
    }
    equip_list_draw(recs, -1, lobby_world_block->field_0x39BA, lsp, present);
    if (page_mode != 0 && weapon != 0xFFFF) {
        get_lsp_data(0x1C3E, &pos);
        draw_sprite_ary(get_menu_lsp_tbl(0x88), &pos);
        get_lsp_data(0x1BFB, &pos);
        draw_sprite_ary(lb_my_set_detail_frame, &pos);
        if (page_mode != 3) {
            rows = lb_my_set_pane_rows_get(pane);
            if (rows >= 2) {
                if (page_mode == 2) {
                    arrows = 0;
                }
                LbPutAnaPageArrow(0x1C00, 0, pane->row_0x07, rows, pane->row_moved_0x0A, &pos, arrows, 0);
            }
            page = pane->row_0x07;
        } else {
            page = 0;
        }
        switch (page) {
        case 0:
            get_lsp_data(0x1C01, &pos);
            draw_sprite_ary(lb_my_set_status_frame, &pos);
            Put_status_equip_status_block(plw, recs, 0x1C18, NULL);
            Put_status_equip_element_block(plw, recs, 0x1C33, NULL);
            column_skills_page_draw(plw, recs, 0x1C39);
            break;
        case 1:
            gun_stats_columns_draw(plw, recs, 0x1C09);
            break;
        }
    }
}

/* 0x801F8ABC - draw the page-turn arrow for one entry, with the "disabled" bit set when the caller
 * passes no room. */
void fn_801F8ABC(u32 id, u8 a, u8 b, u32 c, s32 room)
{
    _mh_ivec2_ pos;
    u8 flags = 1;

    get_lsp_data((u16)id, &pos);
    if (room == 0) {
        flags |= 0x80;
    }
    PutPageArrow(lb_page_arrow_ids, (s16)a, (s16)b, (u16)c, &pos, flags);
}


/* 0x801F8B3C (0x5A4): the equipment-set pane: the set under the cursor (the worn equipment while saving), the page
 * arrows, the moved set's copy and mark while moving, and the help line. */
void lb_my_set_list_draw(::LbMySetPane* self)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ mark_pos;
    LbMySet* set;
    LbMySet* moved;
    s32 room;
    s16 help;
    s16 reason;
    LbItemDb* db;
    s16 reason_row;
    u8 same;

    db = (::LbItemDb*)lobby_world_block;
    set = &db->my_sets_0x3380[self->index_0x04];
    help = -1;
    reason = -1;
    reason_row = 1;
    if (chk_pointer() == 0) {
        room = 1;
        switch (self->kind_0x03) {
        case 0:
            switch (self->phase_0x00) {
            case 1:
                room = 0;
                break;
            }
            break;
        case 1:
            switch (self->phase_0x00) {
            case 1:
                room = 0;
                break;
            }
            break;
        case 3:
            switch (self->phase_0x00) {
            case 1:
                room = 0;
                break;
            }
            break;
        }
    } else {
        room = 0;
    }
    get_lsp_data(0x1BF6, &pos);
    switch (self->kind_0x03) {
    case 1:
        switch (self->phase_0x00) {
        case 0:
            lb_my_set_detail_draw(0x1BF6, self, set->weapon_0x00, set->armor_0x06[3], set->armor_0x06[4],
                                  set->armor_0x06[0], set->armor_0x06[1], set->armor_0x06[2], set->armor_0x06[5],
                                  set->opts_0x02.ids[0], set->opts_0x02.ids[1], 1, 0);
            break;
        case 1:
            lb_my_set_detail_draw(0x1BF6, self, db->kind_0x8C, db->worn_d_0x98, db->worn_e_0x9A, db->worn_a_0x92,
                                  db->worn_b_0x94, db->worn_c_0x96, db->worn_f_0x9C, db->sub_0x8E, db->sub_0x90, 3, 0);
            break;
        }
        draw_sprite_ary(lb_my_set_list_frame, &pos);
        fn_801F8ABC(0x1BD5, self->index_0x04, self->count_0x05, self->moved_0x08, room);
        break;
    case 2:
        switch (self->phase_0x00) {
        case 0:
            lb_my_set_detail_draw(0x1BF6, self, set->weapon_0x00, set->armor_0x06[3], set->armor_0x06[4],
                                  set->armor_0x06[0], set->armor_0x06[1], set->armor_0x06[2], set->armor_0x06[5],
                                  set->opts_0x02.ids[0], set->opts_0x02.ids[1], 0, 0);
            draw_sprite_ary(lb_my_set_list_frame, &pos);
            fn_801F8ABC(0x1BD5, self->index_0x04, self->count_0x05, self->moved_0x08, room);
            break;
        case 1:
            moved = &((::LbItemDb*)lobby_world_block)->my_sets_0x3380[self->mark_0x06];
            lb_my_set_detail_draw(0x1BF6, self, moved->weapon_0x00, moved->armor_0x06[3], moved->armor_0x06[4],
                                  moved->armor_0x06[0], moved->armor_0x06[1], moved->armor_0x06[2],
                                  moved->armor_0x06[5], moved->opts_0x02.ids[0], moved->opts_0x02.ids[1], 0, 0);
            fn_801F8ABC(0x1BD5, self->mark_0x06, self->count_0x05, 0, 0);
            draw_sprite_ary(lb_my_set_list_frame, &pos);
            same = self->mark_0x06 == self->index_0x04 ? 2 : 0;
            get_lsp_data(0x1BF9, &pos);
            lb_my_set_detail_draw(0x1BF9, self, set->weapon_0x00, set->armor_0x06[3], set->armor_0x06[4],
                                  set->armor_0x06[0], set->armor_0x06[1], set->armor_0x06[2], set->armor_0x06[5],
                                  set->opts_0x02.ids[0], set->opts_0x02.ids[1], 0, same);
            draw_sprite_ary(lb_my_set_list_frame, &pos);
            fn_801F8ABC(0x1BFA, self->index_0x04, self->count_0x05, self->moved_0x08, room);
            sprite_frame_apply(&spr, 0x1BD0, (u16)self->blink_0x0C, &mark_pos);
            sprite_frame_apply(&spr, 0x1BD3, (u16)self->blink_0x0C, &pos);
            pos.x = mark_pos.x;
            draw_sprite_ary(lb_my_set_move_mark, &pos);
            break;
        }
        break;
    default:
        switch (self->phase_0x00) {
        case 0:
            lb_my_set_detail_draw(0x1BF6, self, set->weapon_0x00, set->armor_0x06[3], set->armor_0x06[4],
                                  set->armor_0x06[0], set->armor_0x06[1], set->armor_0x06[2], set->armor_0x06[5],
                                  set->opts_0x02.ids[0], set->opts_0x02.ids[1], 1, 1);
            break;
        case 1:
            lb_my_set_detail_draw(0x1BF6, self, set->weapon_0x00, set->armor_0x06[3], set->armor_0x06[4],
                                  set->armor_0x06[0], set->armor_0x06[1], set->armor_0x06[2], set->armor_0x06[5],
                                  set->opts_0x02.ids[0], set->opts_0x02.ids[1], 2, 1);
            break;
        }
        draw_sprite_ary(lb_my_set_list_frame, &pos);
        fn_801F8ABC(0x1BD5, self->index_0x04, self->count_0x05, self->moved_0x08, room);
        break;
    }
    switch (self->kind_0x03) {
    case 0:
        switch (self->phase_0x00) {
        case 0:
            help = 0x109;
            if (set->weapon_0x00 == 0xFFFF) {
                reason = 0xB1;
                reason_row = 2;
            }
            break;
        case 1:
            help = 0x10A;
            lb_panel_yes_no_draw(0x1879, self->stepper_0x10);
            break;
        }
        break;
    case 1:
        switch (self->phase_0x00) {
        case 0:
            help = 0x10B;
            break;
        case 1:
            help = 0x10C;
            lb_panel_yes_no_draw(0x1879, self->stepper_0x10);
            break;
        }
        break;
    case 2:
        switch (self->phase_0x00) {
        case 0:
            help = 0x10D;
            break;
        case 1:
            help = 0x10E;
            break;
        }
        break;
    case 3:
        switch (self->phase_0x00) {
        case 0:
            help = 0x10F;
            if (set->weapon_0x00 == 0xFFFF) {
                reason = 0xB1;
                reason_row = 2;
            }
            break;
        case 1:
            help = 0x110;
            lb_panel_yes_no_draw(0x1879, self->stepper_0x10);
            break;
        }
        break;
    }
    if (help != -1) {
        lb_panel_msg_draw(0x1877, help);
    }
    if (reason != -1) {
        lb_panel_line_draw(0x1877, reason, 2, reason_row);
    }
}


/* 0x801F90E0 (0x314): one saved gunner set's detail: its three cells and, for a stored set, the bowgun's icon, the
 * page arrows and the status page the row selects. */
void lb_gun_set_detail_draw(const _mh_ivec2_* base, ::LbMySetPane* pane, u16 weapon, u16 opt_a, u16 opt_b,
                            u8 page_mode, u8 mark)
{
    _mh_ivec2_ frame;
    _mh_ivec2_ pos;
    u16 ids[3];
    ::_EQUIP opts[2];
    ::_PLW* plw;
    ::_EQUIP* table;
    ::_EQUIP* gun;
    bool arrows;
    u8 page;
    u8 type;

    plw = (::_PLW*)pane->page_0x14;
    table = (::_EQUIP*)((::LbItemDb*)lobby_world_block)->equip_0x0E00;
    arrows = 1;
    ids[0] = weapon;
    ids[1] = opt_a;
    ids[2] = opt_b;
    if (mark == 2) {
        mark = 1;
    } else if (weapon != 0xFFFF) {
        mark = 0;
    }
    lb_gun_set_cells_draw(ids, -1, base, 1, mark);
    if (page_mode != 0 && weapon != 0xFFFF) {
        gun = &table[weapon];
        Gunner_opt_ok_ck(gun);
        if (opt_a != 0xFFFF) {
            equip_record_copy((LbIconRec*)&opts[0], (LbIconRec*)&table[opt_a]);
        } else {
            memset(&opts[0], 0, sizeof(::_EQUIP));
            opts[0].kind = 0xC;
            opts[0].item_id = gun->item_id;
        }
        if (opt_b != 0xFFFF) {
            equip_record_copy((LbIconRec*)&opts[1], (LbIconRec*)&table[opt_b]);
        } else {
            memset(&opts[1], 0, sizeof(::_EQUIP));
            opts[1].kind = 0xD;
            opts[1].item_id = gun->item_id;
        }
        get_lsp_data(0x1B8E, &frame);
        draw_sprite_ary(get_menu_lsp_tbl(0x88), &frame);
        if (page_mode != 3) {
            if (page_mode == 2) {
                arrows = 0;
            }
            lb_ana_page_arrow_put(0x1B7D, 0, pane->row_0x07, (u8)lb_my_set_pane_rows_get(pane), pane->row_moved_0x0A,
                                  &frame, arrows);
            page = pane->row_0x07;
        } else {
            page = 0;
        }
        get_lsp_data(0x1B8C, &pos);
        pos.x += frame.x;
        pos.y += frame.y;
        lb_equip_cell_draw_layout3(gun, 0, &pos, 1, 0, 1, 8, 0);
        type = Get_pl_type(gun, &opts[0]);
        draw_weaponicon_idx(0x13D, type, get_rare_color(Get_equip_rare(gun)), &pos);
        switch (page) {
        case 0:
            put_equip_row_variant(plw, gun, &opts[0], &opts[1], 0x19B1, 7);
            get_lsp_data(0x1C3D, &pos);
            draw_sprite_ary(get_menu_lsp_tbl(0x89), &pos);
            put_lsp_sprite_offset(0x19B2);
            break;
        case 1:
            put_equip_panel_row(plw, gun, &opts[0], &opts[1], 0x19B1, 7);
            break;
        case 2:
            put_equip_row_variant_ex(plw, gun, &opts[0], &opts[1], 0x19B1, 7);
            get_lsp_data(0x1C3D, &pos);
            draw_sprite_ary(get_menu_lsp_tbl(0x89), &pos);
            put_equip_row_variant2(plw, gun, &opts[0], &opts[1], 0x19B2, 7);
            break;
        }
    }
}

/* 0x801F93F4 (0x548): the gunner-set pane: the frame, the set under the cursor (the worn bowgun while saving), the
 * page arrows, the moved set's copy and mark while moving, and the help line. */
void lb_gun_set_list_draw(::LbMySetPane* self)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _mh_ivec2_ title;
    ::LbItemDb* db;
    ::LbGunSet* set;
    ::LbGunSet* moved;
    s32 room;
    s16 help;
    s16 reason;
    s16 reason_row;

    db = (::LbItemDb*)lobby_world_block;
    set = &db->gun_sets_0x36C8[self->index_0x04];
    help = -1;
    reason = -1;
    reason_row = 1;
    if (chk_pointer() == 0) {
        room = 1;
        switch (self->kind_0x03) {
        case 0:
            switch (self->phase_0x00) {
            case 1:
                room = 0;
                break;
            }
            break;
        case 1:
            switch (self->phase_0x00) {
            case 1:
                room = 0;
                break;
            }
            break;
        case 3:
            switch (self->phase_0x00) {
            case 1:
                room = 0;
                break;
            }
            break;
        }
    } else {
        room = 0;
    }
    get_lsp_data(0x1BD6, &base);
    draw_sprite_anim_ary(lb_gun_set_frame, 1, &base);
    get_lsp_data(0x1BF5, &pos);
    pos.x += base.x;
    pos.y += base.y;
    title.x = pos.x + 8;
    title.y = pos.y;
    draw_sprite_ary(lb_gun_set_title, &title);
    draw_sprite_anim_ary(lb_gun_set_panel, 1, &base);
    draw_sprite_anim_idx(0x1BD8, 1, &base);
    switch (self->kind_0x03) {
    case 1:
        switch (self->phase_0x00) {
        case 0:
            lb_gun_set_detail_draw(&pos, self, set->weapon_0x00, set->opts_0x02.ids[0], set->opts_0x02.ids[1], 1, 0);
            break;
        case 1:
            lb_gun_set_detail_draw(&pos, self, db->kind_0x8C, db->sub_0x8E, db->sub_0x90, 3, 0);
            break;
        }
        fn_801F8ABC(0x1BD5, self->index_0x04, self->count_0x05, self->moved_0x08, room);
        break;
    case 2:
        switch (self->phase_0x00) {
        case 0:
            lb_gun_set_detail_draw(&pos, self, set->weapon_0x00, set->opts_0x02.ids[0], set->opts_0x02.ids[1], 0, 0);
            fn_801F8ABC(0x1BD5, self->index_0x04, self->count_0x05, self->moved_0x08, room);
            break;
        case 1:
            moved = &((::LbItemDb*)lobby_world_block)->gun_sets_0x36C8[self->mark_0x06];
            lb_gun_set_detail_draw(&pos, self, moved->weapon_0x00, moved->opts_0x02.ids[0], moved->opts_0x02.ids[1], 0,
                                   0);
            fn_801F8ABC(0x1BD5, self->mark_0x06, self->count_0x05, 0, 0);
            get_lsp_data(0x1BF9, &base);
            draw_sprite_anim_ary(lb_gun_set_frame, 1, &base);
            get_lsp_data(0x1BF5, &pos);
            pos.x += base.x;
            pos.y += base.y;
            title.x = pos.x + 8;
            title.y = pos.y;
            draw_sprite_ary(lb_gun_set_title, &title);
            draw_sprite_anim_ary(lb_gun_set_panel, 1, &base);
            draw_sprite_anim_idx(0x1BD8, 1, &base);
            lb_gun_set_detail_draw(&pos, self, set->weapon_0x00, set->opts_0x02.ids[0], set->opts_0x02.ids[1], 0,
                                   self->mark_0x06 == self->index_0x04 ? 2 : 0);
            fn_801F8ABC(0x1BFA, self->index_0x04, self->count_0x05, self->moved_0x08, room);
            sprite_frame_apply(&spr, 0x1BD0, (u16)self->blink_0x0C, &base);
            draw_sprite_ary(lb_my_set_move_mark, &base);
            break;
        }
        break;
    default:
        switch (self->phase_0x00) {
        case 0:
            lb_gun_set_detail_draw(&pos, self, set->weapon_0x00, set->opts_0x02.ids[0], set->opts_0x02.ids[1], 1, 1);
            break;
        case 1:
            lb_gun_set_detail_draw(&pos, self, set->weapon_0x00, set->opts_0x02.ids[0], set->opts_0x02.ids[1], 2, 1);
            break;
        }
        fn_801F8ABC(0x1BD5, self->index_0x04, self->count_0x05, self->moved_0x08, room);
        break;
    }
    switch (self->kind_0x03) {
    case 0:
        switch (self->phase_0x00) {
        case 0:
            help = 0x111;
            if (set->weapon_0x00 == 0xFFFF) {
                reason = 0xB1;
                reason_row = 2;
            }
            break;
        case 1:
            help = 0x112;
            if (userdata_gunner_ck(db) == 0) {
                reason = 0x113;
            }
            lb_panel_yes_no_draw(0x1879, self->stepper_0x10);
            break;
        }
        break;
    case 1:
        switch (self->phase_0x00) {
        case 0:
            help = 0x114;
            break;
        case 1:
            help = 0x115;
            lb_panel_yes_no_draw(0x1879, self->stepper_0x10);
            break;
        }
        break;
    case 2:
        switch (self->phase_0x00) {
        case 0:
            help = 0x116;
            break;
        case 1:
            help = 0x117;
            break;
        }
        break;
    case 3:
        switch (self->phase_0x00) {
        case 0:
            help = 0x118;
            if (set->weapon_0x00 == 0xFFFF) {
                reason = 0xB1;
                reason_row = 2;
            }
            break;
        case 1:
            help = 0x119;
            lb_panel_yes_no_draw(0x1879, self->stepper_0x10);
            break;
        }
        break;
    }
    if (help != -1) {
        lb_panel_msg_draw(0x1877, help);
    }
    if (reason != -1) {
        lb_panel_line_draw(0x1877, reason, 2, reason_row);
    }
}


/* 0x801F993C (0x1BC): the model pane: the frame, the header of its mode, one row per part (the worn one marked) and
 * the help line. */
void lb_model_pane_draw(::LbModelPane* self)
{
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u32 color;
    LbWorldBlock* world;
    bool selected;
    s16 help;
    bool enabled;
    s32 text;
    bool worn;
    u8 i;

    help = -1;
    world = lobby_world_block;
    get_lsp_data(0x1CD6, &base);
    draw_sprite_anim_ary(lb_model_pane_frame, self->part_0x06, &base);
    switch (self->mode_0x02) {
    case 0:
        draw_sprite_anim_ary(lb_model_pane_head_a, 1, &base);
        help = 0x11A;
        break;
    case 1:
        draw_sprite_anim_ary(lb_model_pane_head_b, 0, &base);
        help = 0x11D;
        break;
    }
    for (i = 0; i < self->part_0x06; i++) {
        text = i + 0x23F;
        selected = self->cursor_0x04 == i;
        enabled = 1;
        worn = 0;
        switch (self->mode_0x02) {
        case 0:
            if (world->count_0x009E == i) {
                enabled = 0;
                worn = 1;
            }
            break;
        case 1:
            if (world->count_0x0002 == i) {
                enabled = 0;
                worn = 1;
            }
            break;
        }
        color = GetMenuFontColor(enabled, selected, 1, worn);
        get_lsp_data(lb_model_pane_rows[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_chg::lb_list_row_draw((s32)LbStr(0, text), selected, (s16*)&pos, color, 0xD);
    }
    fn_801F54C4();
    if (help != -1) {
        if (self->mode_0x02 == 1) {
            lb_panel_msg_draw(0x1878, help);
        } else {
            lb_panel_msg_draw(0x1877, help);
        }
    }
}


/* 0x801F9AF8 (0x1DC): one colour slot: the frame (with the cursor when selected), the colour's name and its swatch. */
void lb_color_slot_draw(s16* pos, u32 color, u8 selected, s16 entry, u8 mode)
{
    _SPR_DATA_ spr;
    char text[0x10];
    const LbSelRec* rec;
    u32 flags;

    flags = 0;
    if (selected != 0) {
        draw_sprite_anim_ary(lb_color_slot_frames[mode], 1, (_mh_ivec2_*)pos);
        put_menu_cursor(lb_color_slot_cursors[mode], 0, (_mh_ivec2_*)pos);
        draw_sprite_anim_ary(lb_color_slot_marks[mode], 1, (_mh_ivec2_*)pos);
        flags = 4;
    } else {
        draw_sprite_anim_ary(lb_color_slot_frames[mode], 0, (_mh_ivec2_*)pos);
        draw_sprite_anim_ary(lb_color_slot_marks[mode], 0, (_mh_ivec2_*)pos);
    }
    rec = &lbl_805B8674[entry];
    flags |= 1;
    spr_data_copy((s16*)&spr, get_lsp_data(lb_color_slot_name_lsp[mode], NULL));
    spr.color_0x1C = color;
    if (mode == 0) {
        switch (entry) {
        default:
            strcpy(text, "1");
            break;
        case 5:
            strcpy(text, "2");
            break;
        case 10:
            strcpy(text, "3");
            break;
        }
        draw_font(spr, (s8*)text, flags, (_mh_ivec2_*)pos);
    } else {
        draw_font(spr, (s8*)LbStr(0, rec->name_id_0x00), flags, (_mh_ivec2_*)pos);
    }
    spr_data_copy((s16*)&spr, get_lsp_data(lb_color_slot_swatch_lsp[mode], NULL));
    spr.color_0x1C = ((u32)rec->bits_0x03 << 24) | ((u32)rec->bits_0x04 << 16) | ((u32)rec->bits_0x05 << 8) | 0xFF;
    draw_sprite(spr, (_mh_ivec2_*)pos);
}


/* 0x801F9CD4 (0x408): the colour pane: the frame, the grid of presets (the free cell picks its colour off the frame
 * buffer under the cursor), the picked colour's swatch and the help line. */
void lb_color_pane_draw(LbColorPane* self)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ grid;
    _mh_ivec2_ origin;
    u32 color;
    const s16* cells;
    u16* head;
    u16 picked;
    u16 count;
    u16 frame;
    u8 i;
    u8 selected;
    bool lit;
    s16 help;

    cells = lbl_805B87D8;
    if (self->mode_0x02 == 0) {
        cells = lbl_805B87F4;
    }
    get_lsp_data(0x1CF8, &pos);
    draw_sprite_ary(lb_color_pane_frame, &pos);
    head = lb_color_pane_head_inner;
    if (self->mode_0x02 == 1) {
        head = lb_color_pane_head_hair;
    }
    draw_sprite_ary(head, &pos);
    get_lsp_data(0x1CF6, &grid);
    get_lsp_data(0x1CF7, &origin);
    draw_sprite_anim_ary(lb_color_pane_grid_frame, 1, &origin);
    draw_sprite_anim_ary(lb_color_pane_grid_mark, 1, &origin);
    picked = lb_color_grid_index(lb_color_grid_widths, self->column_0x04, self->row_0x06);
    count = fn_801F3588(lb_color_grid_widths);
    for (i = 0; i < count; i++) {
        if (i == picked) {
            selected = 1;
            lit = 1;
        } else {
            selected = 0;
            lit = 0;
        }
        color = GetMenuFontColor(1, lit, 1, 0);
        switch (cells[i]) {
        case -2:
            sprite_frame_apply(&spr, 0x1CBA, 1, &pos);
            pos.x += origin.x;
            pos.y += origin.y;
            lb_cursor_prompt_draw(selected, (s16*)&pos);
            break;
        case -3:
            if (lit == 1) {
                frame = self->anim_0x0C + 10;
            } else {
                frame = 1;
            }
            draw_sprite_anim_ary(lb_color_pane_free_frame, frame, &origin);
            if (lit == 1) {
                GXDrawDone();
                sprite_frame_apply(&spr, 0x1CA8, 1, &pos);
                spr_data_copy((s16*)&spr, get_lsp_data(0x1CA6, NULL));
                spr.pos_x = self->cursor_x_0x0E;
                spr.pos_y = self->cursor_y_0x10;
                pos.x += origin.x;
                pos.y += origin.y;
                GXPeekARGB((u16)((f32)(pos.x + spr.pos_x) * ((f32)(s16)Screen_w.width / Screen_w.width_f)),
                           (u16)((f32)(pos.y + spr.pos_y) * ((f32)(s16)Screen_w.height / Screen_w.height_f)), &color);
                self->color_0x14 = (color << 8) | 0xFF;
                draw_sprite(spr, &pos);
            }
            break;
        default:
            get_lsp_data(lb_color_pane_cell_lsp[i + 1], &pos);
            pos.x += grid.x;
            pos.y += grid.y;
            lb_color_slot_draw((s16*)&pos, color, selected, cells[i], 1);
            break;
        }
    }
    spr_data_copy((s16*)&spr, get_lsp_data(0x1CB2, NULL));
    spr.color_0x1C = self->picked_0x18;
    spr_anim_draw((s16*)&spr, 1, (s16*)&origin);
    help = -1;
    switch (self->phase_0x00) {
    case 0:
        fn_801F54C4();
        switch (self->mode_0x02) {
        case 0:
            help = 0x11C;
            break;
        case 1:
            help = 0x11F;
            break;
        }
        break;
    case 1:
        help = 0x124;
        lb_panel_yes_no_draw(0x1879, self->stepper_0x20);
        break;
    }
    if (help != -1) {
        lb_panel_msg_draw(0x1877, help);
    }
}

#pragma peephole on

} /* extern "C" */

} /* namespace s_801F3294 */

/* The character-edit (hair/inner colour) screen band (0x801F9CD4-0x801FBF78). */
#include "types.h"

#include "lobby/lb_npc.h" /* LbResId / LbResource / LbResRec - the lobby resource types (rule 2) */
#include "unsplit/lobby.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "fn_8004CAD8/get_qResult_work.h" /* get_qResult_work (rule 2) */
#include "quest/quest_result_work.h"        /* Q_ResultWork (rule 1) */
#include "quest/quest_file_table.h"         /* stage_dcm_file_table (rule 2) */
#include "ef/PlayMode_set.h"
#include "gallery_open.h"
#include "enemy/lobby_state_reset.h"
#include "enemy/enemy_net_work_reset.h"
#include "lobby/lb_progress_unlock.h"
#include "menu/quest_group_clear_count.h"
#include "menu/menu_plsearch.h" /* the save-file steps and chg_nand_err2msgcode (rule 2) */

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * The two shared spellings that disagree with the DOL (see the header): `lobby_world_block` is a pointer,
 * and `lb_list_row_draw` takes five arguments.  A scoped `extern "C"` declaration cannot be beaten by a
 * global re-declaration - MWCC reports `illegal function overloading` (10197) - so the correct shape
 * lives in this scope, which names the same symbols.
 * ------------------------------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------------------------------
 * Types this unit reconstructs.
 * ------------------------------------------------------------------------------------------------- */
/* The character-edit screen work object the two `self`-taking functions read. */
typedef struct LbChgColorWork {
    /* +0x00 */ u8 mode_0x00; /* 0 = body, 1 = hair - the tail's switch value */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 table_0x02; /* 0 selects the body colour table, non-zero the hair one */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ s16 value_0x04;
    /* +0x06 */ s16 value_0x06;
    /* +0x08 */ s16 selected_0x08;
    /* +0x0A */ u8 unused_0x0A[2];
    /* +0x0C */ s16 value_0x0C;
    /* +0x0E */ s16 offset_0x0E;
    /* +0x10 */ s16 offset_0x10;
    /* +0x12 */ u8 unused_0x12[2];
    /* +0x14 */ u32 color_0x14;
    /* +0x18 */ u32 value_0x18;
    /* +0x1C */ u8 unused_0x1C[4];
    /* +0x20 */ void* cursor_0x20;
    /* +0x24 */ void* cursor_0x24;
    /* +0x28 */ void* table_0x28;
} LbChgColorWork; /* size: 0x2C */

/* The sequence record `lobby_enter_seq_step`'s caller owns: a step counter and the state the driver switches
 * on. */
typedef struct LbChgSeqWork {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01[9];
    /* +0x0A */ u8 step_0x0A;
    /* +0x0B */ u8 state_0x0B;
    /* +0x0C */ u8 unused_0x0C[8];
    /* +0x14 */ u8 mode_0x14;          /* the entry mode the setup sequence resets the lobby with (1 = online) */
} LbChgSeqWork; /* size: 0x15 (approximate: +0x14 is the last byte read) */

/* One 8-byte row of the archive table `fn_801FB478` streams: the byte count and the source name. */
typedef struct LbChgFileReq {
    /* +0x0 */ u32 size_0x00;
    /* +0x4 */ const char* name_0x04;
} LbChgFileReq; /* size: 0x8 */

/* ---------------------------------------------------------------------------------------------------
 * Declarations.  `fn_*`/`ckResourceName` are plain C symbols; the lobby UI helpers come from
 * `unsplit/lobby.h`; the mangled C++ callees are declared with the signatures that reproduce their map
 * names (rule 9).  Some belong to other units whose headers do not declare them (`fn_801FCA80`, `fn_801FCADC`,
 * `demo_work_update`, ...).
 * ------------------------------------------------------------------------------------------------- */
extern "C" {
s32 score_add_clamped(s32 delta, s32* value);
void stage_map_set(u8 mode);
s32 res_file_assign(void* dst, void* src);
void* res_file_ctor(void* out, u32 value);
void nw_res_list_load(void* rec);
void mhchar_res_bank_set(u32 kind, u8 arg, void* str);
void eft_res_pool_reset(void);
void eft_control_init(void);
void eft_common_load(void);
void fn_801E92B0(void);
void fn_801F3588(const void* table);
s32 fn_801F3EDC(LbChgColorWork* self);
s32 fn_801F3F14(void* table, u8 index);
void fn_801F54C4(void);
void lb_color_slot_draw(s16* pos, u32 color, u8 selected, s16 entry, u8 mode);
void fn_801FB2A8(u8 a, u8 b, u8 c);
void fn_801FB364(u8 index);
void fn_801FB438(void);
void fn_801FB478(void);
void fn_801FBAE4(void);
void lb_quest_return_apply(void);
void fn_801FC6A0(u32 unused, s32* value);
void fn_801FC810(void);
void fn_801FC874(void);
void fn_801FC8C8(void);
void lb_npc_map_setup(void);
void fn_801FCA80(u32 index);
void fn_801FCADC(u8* self, u32 index);
void lb_menu_scratch_reset(void);
void menu_item_work_init(void);
void stage_work_init(void);
s32 stage_area_load(u32 a, u8 b, u8 c);
s32 stage_area_enter(u8 a, u8 b);
void stage_area_slots_reset(void);
s32 stage_area_slot_get(u8 a, u8 b);
void stage_map_obj_load(u8 mode);
void camera_work_init(void);
void camera_area_default_set(void);
void stage_dcm_path_get(char* buf, u8 index);
void sprite_frame_apply(void* dst, u16 id, u8 flag, const _mh_ivec2_* src);
void lobby_hud_reset(void);
void hud_msg_init(void);
void lobby_res_slots_init(void);
void demo_work_start(void);
void demo_work_update(void);
}

/* The C++-linkage callees: their map names are manglings, and these signatures reproduce them. */
void* ckResourceName(char* name);
s32 create_move_work(long kind);
s32 file_loading_ck(char* name, s32* out);
void init_player_work(void);
void light_init(void);
void load_file(char* name, u32 dst, long size);
void player_control_move(void);
void player_init_data_load(void);
void setSoftresetFlag(bool flag);
void subTransSet(u32 a, long b, u32* value);
void village_tex_load(void);
void* work_mem_alloc(u32 size);

/* The range's own `fn_XXXXXXXX` definitions keep C linkage so objdiff pairs them by the map's
 * spelling (`extern "C"` - the mangled stem would pair nothing; playbook row 42). */
extern "C" {

/* 0x801FA0DC - draw the colour-slot list: one row per selectable colour with the cursor on the
 * current one, then the mode-dependent tail. */
void lb_dye_inner_draw(LbChgColorWork* self)
{
    _mh_ivec2_ origin;
    _mh_ivec2_ pos;
    u32 i;
    u16 count;
    s16 value;
    s32 color;
    s32 selected;
    bool active;

    get_lsp_data(0x1C61U, &origin);
    draw_sprite_ary((const u16*)lbl_805B89D0, &origin);
    get_lsp_data(0x1C5BU, &origin);
    count = (u16)fn_801F3EDC(self);
    i = 0;
    for (; (u8)i < count; i++) {
        if (self->selected_0x08 == (s32)(u8)i) {
            selected = 1;
            active = 1;
        } else {
            selected = 0;
            active = 0;
        }
        color = GetMenuFontColor(1, active, 1, fn_801F3F14(self->table_0x28, (u8)i) == 1);
        get_lsp_data(lbl_805B8A04[(u8)i], &pos);
        pos.x += origin.x;
        pos.y += origin.y;
        value = lbl_805B875C[(u8)i];
        switch (value) {
        case -1:
            lb_chg::lb_list_row_draw((s32)LbStr(0, 0x231), (u8)active, (s16*)&pos, color, 8);
            break;
        case -2:
            lb_cursor_prompt_draw((u8)(selected | 0x80), (s16*)&pos);
            break;
        default:
            get_lsp_data(lbl_805B8A04[(u8)i], &pos);
            pos.x += origin.x;
            pos.y += origin.y;
            lb_color_slot_draw((s16*)&pos, color, selected, lbl_805B875C[(u8)i], 0);
            break;
        }
    }
    {
        s32 mode = -1;

        switch (self->mode_0x00) {
        case 0:
            fn_801F54C4();
            mode = 0x120;
            break;
        case 1:
            mode = 0x124;
            lb_panel_yes_no_draw(0x1879, (s32)self->cursor_0x24);
            break;
        }
        if (mode != -1) {
            lb_panel_msg_draw(0x1877, (s16)mode);
        }
    }
}

} /* extern "C" */

/* The box menu's draw pass reads the page band's types, so it is defined in that band's scope. */
namespace s_801F3294 {
extern "C" {


/* 0x801FA2C8 (0x690): the dye pane: the armour piece list (each piece's dye state), or the colour grid of the picked
 * piece with the free cell picking its colour off the frame buffer, the colour's swatch and the help line. */
void lb_dye_pane_draw(LbPage* self)
{
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ base;
    _mh_ivec2_ origin;
    u32 color;
    const s16* cells;
    ::_EQUIP* equip;
    u16 frame;
    u16 anim;
    u16 count;
    s16 picked;
    u8 i;
    u8 selected;
    u8 dye;
    bool lit;
    bool enabled;
    bool locked;
    s16 help;

    if ((u32)(self->state_0x000 - 1) > 1) {
        if (self->state_0x000 == 0) {
            get_lsp_data(0x1C7D, &pos);
            draw_sprite_ary(lb_dye_pane_frame, &pos);
            get_lsp_data(0x1C76, &base);
            i = 0;
            do {
                lit = self->part_0x004 == i;
                if (lb_dye_part_ck((::_PLW*)self->next_0x028, (u16)i) == 1) {
                    enabled = 1;
                    locked = 0;
                } else {
                    enabled = 0;
                    locked = 1;
                }
                color = GetMenuFontColor(enabled, lit, 1, 0);
                if (i == 0) {
                    get_lsp_data(lb_dye_part_lsp[i], &pos);
                    pos.x += base.x;
                    pos.y += base.y;
                    lb_chg::lb_list_row_draw((s32)LbStr(0, 0x232), lit, (s16*)&pos, color, 9);
                } else {
                    equip = &((::_PLW*)self->next_0x028)->equipA[lb_dye_part_equip_index[i]];
                    if (equip->item_id == 0) {
                        dye = 0;
                    } else if (EnableChangeColor(equip) == 1) {
                        dye = 0;
                    } else {
                        dye = self->next_0x028->sub_0x262;
                    }
                    Put_equip_dtl_syo_item1(equip, NULL, lit, locked, dye, base, lb_dye_part_lsp[i]);
                }
                i++;
            } while (i < 6);
            fn_801F54C4();
            lb_panel_msg_draw(0x1877, 0x121);
        }
    } else {
        if (self->kind_0x003 == 1) {
            frame = 0;
            cells = lb_dye_grid_cells_a;
            picked = lb_color_grid_index(lbl_805B8768, self->count_0x006, self->field_0x008);
        } else {
            frame = 1;
            cells = lb_dye_grid_cells_b;
            picked = lb_color_grid_index(lbl_805B8798, self->count_0x006, self->field_0x008);
        }
        sprite_frame_apply(&spr, 0x1CBB, frame, &pos);
        draw_sprite_anim_ary(lb_dye_grid_frame, frame, &pos);
        draw_sprite_anim_idx(lb_dye_part_icon[self->part_0x004], frame, &pos);
        sprite_frame_apply(&spr, 0x1C97, frame, &base);
        sprite_frame_apply(&spr, 0x1CA5, frame, &origin);
        draw_sprite_anim_ary(lb_color_pane_grid_frame, frame, &origin);
        draw_sprite_anim_ary(lb_color_pane_grid_mark, frame, &origin);
        count = fn_801F3EDC(self);
        for (i = 0; i < count; i++) {
            if (i == picked) {
                selected = 1;
                lit = 1;
            } else {
                selected = 0;
                lit = 0;
            }
            color = GetMenuFontColor(1, lit, 1, 0);
            switch (cells[i]) {
            case -1:
                get_lsp_data(lb_color_pane_cell_lsp[i], &pos);
                pos.x += base.x;
                pos.y += base.y;
                lb_chg::lb_list_row_draw((s32)LbStr(0, 0x231), lit, (s16*)&pos, color, 9);
                break;
            case -2:
                sprite_frame_apply(&spr, 0x1CBA, frame, &pos);
                pos.x += origin.x;
                pos.y += origin.y;
                lb_cursor_prompt_draw(selected, (s16*)&pos);
                break;
            case -3:
                if (lit == 1) {
                    anim = self->anim_0x00E + 10;
                } else {
                    anim = 1;
                }
                draw_sprite_anim_ary(lb_color_pane_free_frame, anim, &origin);
                if (lit == 1) {
                    GXDrawDone();
                    sprite_frame_apply(&spr, 0x1CA8, frame, &pos);
                    spr_data_copy((s16*)&spr, get_lsp_data(0x1CA6, NULL));
                    spr.pos_x = self->cursor_x_0x010;
                    spr.pos_y = self->cursor_y_0x012;
                    pos.x += origin.x;
                    pos.y += origin.y;
                    GXPeekARGB((u16)((f32)(pos.x + spr.pos_x) * ((f32)(s16)Screen_w.width / Screen_w.width_f)),
                               (u16)((f32)(pos.y + spr.pos_y) * ((f32)(s16)Screen_w.height / Screen_w.height_f)), &color);
                    color <<= 8;
                    self->sel_id_0x014 = (((color >> 24) & 0xF8) << 8) | (((color >> 16) & 0xFC) << 3)
                                         | (((color >> 8) & 0xF8) >> 3);
                    draw_sprite(spr, &pos);
                }
                break;
            default:
                get_lsp_data(lb_color_pane_cell_lsp[i], &pos);
                pos.x += base.x;
                pos.y += base.y;
                lb_color_slot_draw((s16*)&pos, color, selected, cells[i], 1);
                break;
            }
        }
        if (self->part_0x004 != 0 || self->sel_flag_0x022 != 1) {
            spr_data_copy((s16*)&spr, get_lsp_data(0x1CB2, NULL));
            spr.color_0x1C = ((u8)((self->sel_0x020 & 0xF800) >> 8) << 24) | ((u8)((self->sel_0x020 & 0x7E0) >> 3) << 16)
                             | ((u8)((self->sel_0x020 & 0x1F) << 3) << 8) | 0xFF;
            spr_anim_draw((s16*)&spr, frame, (s16*)&origin);
        }
        help = -1;
        switch (self->state_0x000) {
        case 1:
            fn_801F54C4();
            help = 0x123;
            break;
        case 2:
            help = 0x124;
            lb_panel_yes_no_draw(0x1879, self->stepper_0x024);
            break;
        }
        if (help != -1) {
            lb_panel_msg_draw(0x1877, help);
        }
    }
}


/* 0x801FA958 (0x880): the box menu's draw pass: the top list, or the active pane with its help line and the
 * reason a pick is refused. */
void lb_box_menu_draw(void)
{
    _mh_ivec2_ pos;
    LbBoxMenu* self;
    LbDigitPane* digits;
    LbEntryPane* entry;
    LbEquipPane* set;
    LbDyePane* dye;
    MenuSelCursor* cursor;
    s16 str;
    s16 index;
    s16 count;
    u16 id;

    self = (LbBoxMenu*)lobby_w.menu_0xAC;
    digits = &self->digits_0x054;
    entry = &self->entry_0x074;
    set = &self->set_0x2A8;
    dye = &self->dye_0x33C;
    cursor = &self->cursor_0x030;
    flfntFlush();
    flfntStackReset();
    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    if (self->leave_0x003 == 1) {
        lb_box_guide_draw(0x1AB0, 0x1AB1, 0x1ACC, 1, self->changed_0x004);
        return;
    }
    switch (self->state_0x000) {
    case 1:
        lb_box_top_draw(self);
        break;
    case 2:
        switch (self->sub_0x001) {
        case 0:
            if (lb_box_lists_draw((::LbListPane*)self, 0x84) == 0) {
                lb_panel_msg_draw(0x1877, 0xB3);
                index = fn_801ED53C((::LbListPane*)self);
                if (userdata_equip_item_id_get((::LbItemDb*)lobby_world_block, index) != 0 && fn_801ED184(index) <= 0) {
                    lb_panel_str_print(0x1877, 0xCC, 2);
                }
            }
            break;
        case 1:
            if (lb_box_lists_draw((::LbListPane*)self, 6) == 0) {
                lb_panel_msg_draw(0x1877, 0xB4);
            }
            fn_801F5FB0(self->list_0x020, self->list_0x01E, self->moved_0x01C, 0);
            break;
        case 2:
            get_lsp_data(0x1AE1, &pos);
            lb_page_walk_head(&pos, self->list_0x022);
            lb_panel_msg_draw(0x1878, 0xB5);
            break;
        }
        break;
    case 3:
        switch (self->sub_0x001) {
        case 0:
            if (lb_box_lists_draw((::LbListPane*)self, 0x104) == 0) {
                lb_panel_msg_draw(0x1877, 0xB3);
                index = fn_801ED53C((::LbListPane*)self);
                if (userdata_equip_item_id_get((::LbItemDb*)lobby_world_block, index) != 0 && fn_801ED200(index, self->side_0x005) <= 0) {
                    id = userdata_equip_item_id_get((::LbItemDb*)lobby_world_block, index);
                    if (self->side_0x005 == 0) {
                        count = userdata_pouch_size(1);
                        if (item_pair_index_find(id, userdata_pouch_get((::LbItemDb*)lobby_world_block, 1), count) >= 0) {
                            str = 0xCA;
                        } else {
                            str = 0xC8;
                        }
                    } else {
                        count = userdata_pouch_size(0);
                        str = 0xC7;
                        if (item_pair_index_find(id, userdata_pouch_get((::LbItemDb*)lobby_world_block, 0), count) >= 0) {
                            str = 0xC9;
                        }
                    }
                    lb_panel_str_print(0x1877, str, 2);
                }
            }
            break;
        case 1:
            if (lb_box_lists_draw((::LbListPane*)self, 6) == 0) {
                lb_panel_msg_draw(0x1877, 0xB4);
            }
            fn_801F5FB0(self->list_0x020, self->list_0x01E, self->moved_0x01C, 0);
            break;
        case 2:
            if (lb_box_lists_draw((::LbListPane*)self, 0xB) == 0) {
                lb_panel_msg_draw(0x1877, 0xB5);
            }
            break;
        }
        break;
    case 4:
        switch (digits->phase_0x00) {
        case 0:
            get_lsp_data(0x1AE1, &pos);
            fn_801F6A9C((LbPageOwner*)self, &pos, 1);
            if (digits->active_0x1C == 0) {
                lb_panel_msg_draw(0x1878, 0xB6);
            }
            break;
        case 1:
            if (lb_box_lists_draw((::LbListPane*)self, 0x14) == 0) {
                lb_panel_msg_draw(0x1877, 0xB9);
                index = fn_801ED53C((::LbListPane*)self);
                str = -1;
                switch (fn_801EDA9C((::LbListPane*)self, index, self->item_id_0x024)) {
                case 1:
                    str = 0xCD;
                    break;
                case 2:
                    str = 0xCC;
                    break;
                }
                if (str != -1) {
                    lb_panel_str_print(0x1877, str, 2);
                }
            }
            break;
        case 2:
            if (lb_box_lists_draw((::LbListPane*)self, 6) == 0) {
                lb_panel_msg_draw(0x1877, 0xB8);
            }
            fn_801F5FB0(digits->pick_count_0x18, digits->pick_max_0x16, digits->flags_0x08, 0);
            break;
        case 3:
            if (lb_box_lists_draw((::LbListPane*)self, 0xA) == 0) {
                lb_panel_msg_draw(0x1877, 0xBB);
            }
            break;
        }
        break;
    case 5:
        switch (self->sub_0x001) {
        case 0:
            get_lsp_data(0x1AE1, &pos);
            if (lb_box_lists_draw((::LbListPane*)self, 0x25) == 0) {
                lb_panel_msg_draw(0x1877, 0xB7);
                index = fn_801ED554((::LbListPane*)self);
                id = userdata_pouch_get((::LbItemDb*)lobby_world_block, self->side_0x005 == 0)[index].id;
                if (id != 0) {
                    if (fn_801EDA34(index, self->side_0x005) <= 0) {
                        count = userdata_pouch_size(self->side_0x005);
                        if (item_pair_index_find(id, userdata_pouch_get((::LbItemDb*)lobby_world_block, self->side_0x005), count)
                            >= 0) {
                            if (self->side_0x005 == 0) {
                                str = 0xC9;
                            } else {
                                str = 0xCA;
                            }
                        } else {
                            str = 0xC8 - (self->side_0x005 == 0);
                        }
                        lb_panel_str_print(0x1877, str, 2);
                    } else if (pl_item_pouch_ok_ck((::_PLW*)self->page_0x010, id) == 0) {
                        lb_panel_line_draw(0x1877, 0xD0, 2, 0);
                    }
                }
            }
            break;
        case 1:
            if (lb_box_lists_draw((::LbListPane*)self, 0x44) == 0) {
                lb_panel_msg_draw(0x1877, 0xB9);
                index = fn_801ED53C((::LbListPane*)self);
                str = -1;
                switch (fn_801EDB34((::LbListPane*)self, index, self->item_id_0x024)) {
                case 1:
                    str = 0xCD;
                    break;
                }
                if (str != -1) {
                    lb_panel_str_print(0x1877, str, 2);
                }
            }
            break;
        case 2:
            if (lb_box_lists_draw((::LbListPane*)self, 6) == 0) {
                lb_panel_msg_draw(0x1877, 0xB8);
            }
            fn_801F5FB0(self->list_0x020, self->list_0x01E, self->moved_0x01C, 0);
            break;
        case 3:
            if (lb_box_lists_draw((::LbListPane*)self, 0xA) == 0) {
                lb_panel_msg_draw(0x1877, 0xBB);
            }
            break;
        }
        break;
    case 6:
        get_lsp_data(0x1AE1, &pos);
        fn_801F6A9C((LbPageOwner*)self, &pos, 2);
        lb_box_combine_draw(digits, cursor);
        break;
    case 7:
        get_lsp_data(0x1AE1, &pos);
        fn_801F6A9C((LbPageOwner*)self, &pos, 3);
        break;
    case 8:
        get_lsp_data(0x1AE1, &pos);
        fn_801F6A9C((LbPageOwner*)self, &pos, 4);
        break;
    case 9:
        get_lsp_data(0x1AE1, &pos);
        switch (entry->phase_0x000) {
        case 0:
            lb_pane_draw(entry, pos.x, pos.y, self);
            break;
        case 1:
            lb_equip_set_pane_draw(set, 0x1BD6, entry, 0x1AE1, (u8*)self);
            break;
        case 2:
            lb_panel_msg_draw(0x1877, 0xDD);
            break;
        }
        break;
    case 10:
    case 11:
        get_lsp_data(0x1AE1, &pos);
        lb_pane_draw(entry, pos.x, pos.y, self);
        break;
    case 12:
    case 13:
    case 14:
    case 15:
        if (self->state_0x000 == 12 && self->my_set_0x3A0.phase_0x00 == 2) {
            lb_panel_msg_draw(0x1877, 0xDD);
            break;
        }
        lb_my_set_list_draw(&self->my_set_0x3A0);
        break;
    case 16:
    case 17:
    case 18:
    case 19:
        if (self->state_0x000 == 16 && self->my_set_0x3A0.phase_0x00 == 2) {
            lb_panel_msg_draw(0x1877, 0xDD);
            break;
        }
        lb_gun_set_list_draw(&self->my_set_0x3A0);
        break;
    case 20:
    case 22:
        switch (self->model_0x390.phase_0x00) {
        case 0:
            lb_model_pane_draw(&self->model_0x390);
            break;
        case 1:
            switch (self->model_0x390.mode_0x02) {
            case 0:
                lb_panel_msg_draw(0x1877, 0x11B);
                break;
            case 1:
                lb_panel_msg_draw(0x1877, 0x11E);
                break;
            }
            break;
        }
        break;
    case 21:
    case 23:
        lb_color_pane_draw(&self->color_0x368);
        break;
    case 24:
        lb_dye_inner_draw((LbChgColorWork*)dye);
        break;
    case 25:
        lb_dye_pane_draw((LbPage*)dye);
        break;
    }
}

} /* extern "C" */
} /* namespace s_801F3294 */

/* Fills the caller's colour word and shape outputs from the hair-colour table entry `index` names. */
void get_change_hair_color(u8 index, u32* color, u16* out_id, s16* out_value_0x06, s16* out_value_0x08)
{
    const LbChangeColorRec* rec = &lbl_805B8674[lbl_805B87D8[index]];

    *out_id = rec->id_0x00;
    *color = ((u32)rec->r_0x03 << 24) | ((u32)rec->g_0x04 << 16) | ((u32)rec->b_0x05 << 8) | 0xFF;
    *out_value_0x06 = rec->value_0x06;
    *out_value_0x08 = rec->value_0x08;
}

/* Fills the caller's colour word and shape outputs from the inner-colour table entry `index` names. */
void get_change_inner_color(u8 index, u32* color, u16* out_id, s16* out_value_0x06, s16* out_value_0x08)
{
    const LbChangeColorRec* rec = &lbl_805B8674[lbl_805B87F4[index]];

    *out_id = rec->id_0x00;
    *color = ((u32)rec->r_0x03 << 24) | ((u32)rec->g_0x04 << 16) | ((u32)rec->b_0x05 << 8) | 0xFF;
    *out_value_0x06 = rec->value_0x06;
    *out_value_0x08 = rec->value_0x08;
}

extern "C" {

/* 0x801FB2A8 - bring the editor screen up: the player/word init chain, the two lobby resource loads,
 * the colour-table setup and the mode handover. */
void fn_801FB2A8(u8 a, u8 b, u8 c)
{
    init_player_work();
    lobby_res_slots_init();
    stage_area_slots_reset();
    player_init_data_load();
    nw_res_list_load((void*)npc_model_res_ids);
    nw_res_list_load((void*)ainpc_anim_res_ids);
    fn_801FCA80(a);
    eft_res_pool_reset();
    eft_common_load();
    if (isCityMode() == 0 || system_w.field_0x8b1 == 1) {
        village_tex_load();
    }
    stage_map_set(b);
    stage_map_obj_load(b);
    stage_area_load(0, b, c);
}

/* 0x801FB364 - queue the model resources the editor needs: the `index + 1`-th row of the lobby
 * resource id table, the shared `mot_aicom.brres`, then the caller's own table entry. */
void fn_801FB364(u8 index)
{
    LbResRec dst;

    res_file_ctor(&dst, 0);
    {
        LbResId* ids = (LbResId*)pl_common_motion_res_ids;
        u32 name = ids[index + 1].b_0x04; /* the row's name pointer, a `u32` in the shared header */
        LbResource* res = (LbResource*)ckResourceName((char*)name);

        if (res != 0) {
            u32 tmp;

            res_file_assign(&dst, res_file_ctor(&tmp, (u32)res->payload_0x44));
            mhchar_res_bank_set(0, 0, &dst);
        }
    }
    {
        LbResource* res = (LbResource*)ckResourceName((char*)lbl_805B8CE0);

        if (res != 0) {
            u32 tmp;

            res_file_assign(&dst, res_file_ctor(&tmp, (u32)res->payload_0x44));
            mhchar_res_bank_set(3, 0, &dst);
        }
    }
    fn_801FCADC(NULL, index);
}

/* 0x801FB438 - run the nine subsystem initialisers the editor's first frame needs. */
void fn_801FB438(void)
{
    light_init();
    eft_control_init();
    stage_work_init();
    camera_work_init();
    lb_menu_scratch_reset();
    menu_item_work_init();
    lobby_hud_reset();
    hud_msg_init();
    demo_work_start();
}

/* 0x801FB478 - load the ten `07/dcm%03d.bin` archives the editor streams, one work allocation each. */
void fn_801FB478(void)
{
    char name[0x20];
    LbChgFileReq* req;
    void** dst;
    s32 i;

    i = 3;
    req = (LbChgFileReq*)stage_dcm_file_table + i;
    dst = (void**)stage_dcm_buffer_tbl + i;
    for (; i <= 0xD; i++) {
        if (req != NULL && req->size_0x00 != 0 && *dst == NULL) {
            stage_dcm_path_get(name, (u8)i);
            *dst = work_mem_alloc(req->size_0x00);
            load_file(name, (u32)*dst, (s32)req->size_0x00);
        }
        req++;
        dst++;
    }
}

/* 0x801FB524 - the editor's sequence driver: one state step per frame, returning whether the sequence
 * has finished. */
s32 lobby_enter_seq_step(LbChgSeqWork* self)
{
    u16 status = Psw[0].button_0x2C0.pressed_0x04;
    s32 done = 0;
    Q_ResultWork* q;
    s32 v;

    do {
        switch (self->state_0x0B) {
        case 0:
            fn_801FBAE4();
            fn_801FC810();
            lobby_w.field_0x006 = 1;
            lobby_w.field_0x001 = 0x16;
            lobby_w.field_0x002 = 0;
            create_move_work(2);
            if (system_w.field_0x7ce == 2) {
                lobby_w.field_0x002 = 2;
            }
            lb_quest_return_apply();
            fn_801FC874();
            fn_801FC8C8();
            fn_801E92B0();
            q = get_qResult_work();
            if (q != NULL && system_w.field_0x2d == 0 &&
                (q->kind_0x1E3 == 1 || q->kind_0x1E3 == 3)) {
                if (game_data_file_create_start() != 0) {
                    self->state_0x0B = 1;
                    system_w.field_0x865 = 1;
                    continue;
                }
                if (game_system_file_create_start() != 0) {
                    self->state_0x0B = 2;
                    continue;
                }
            }
            break;
        case 1:
            switch (game_save_wait()) {
            case 1:
                if (game_system_file_create_start() != 0) {
                    self->state_0x0B = 2;
                    continue;
                }
                break;
            case -2:
            case -1:
                self->state_0x0B = 0x64;
                continue;
            default:
                continue;
            }
            break;
        case 2:
            switch (game_system_file_create_wait()) {
            case 1:
                break;
            case -2:
            case -1:
                self->state_0x0B = 0x64;
                continue;
            default:
                continue;
            }
            break;
        case 3:
            fn_801FB2A8(0, lobby_w.field_0x001, lobby_w.field_0x002);
            self->state_0x0B++;
            setSoftresetFlag(0);
            continue;
        case 4:
            if (file_loading_ck(NULL, NULL) != 1U) {
                fn_801FB364(0);
                lobby_w.slots_0x00C[lobby_w.field_0x014] = (u32)(s8)stage_area_slot_get(lobby_w.field_0x001, lobby_w.field_0x002);
                self->state_0x0B++;
            }
            continue;
        case 5:
            fn_801FB438();
            fn_801FB478();
            self->state_0x0B++;
            continue;
        case 6:
            stage_area_enter(lobby_w.field_0x001, lobby_w.field_0x002);
            demo_work_update();
            lb_npc_map_setup();
            camera_area_default_set();
            self->step_0x0A++;
            self->state_0x0B = 0;
            setSoftresetFlag(1);
            return 0;
        case 0x64:
            if ((status & 0x10) != 0) {
                break;
            } else {
                s32 code;

                done = 1;
                code = chg_nand_err2msgcode();
                subTransSet((u32)fn_801FC6A0, 1, (u32*)&code);
            }
            continue;
        default:
            continue;
        }
        system_w.field_0x865 = 0;
        self->state_0x0B = 3;
    } while (0);
    if (self->state_0x0B >= 3) {
        player_control_move();
    }
    return done;
}

/* 0x801FB80C (0x2D8): the lobby scene's setup sequence: one state step per frame (the save, the area load and the
 * scene objects), returning 1 when a save error hands over to the error screen. */
s32 lobby_scene_setup_step(LbChgSeqWork* self)
{
    u16 status = Psw[0].button_0x2C0.pressed_0x04;
    s32 done = 0;
    Q_ResultWork* q;
    s32 v;

    do {
        switch (self->state_0x0B) {
        case 0:
            lobby_state_reset(self->mode_0x14);
            enemy_net_work_reset();
            NetCtrlWk::clearSessionFlag();
            fn_801FBAE4();
            fn_801FC810();
            lobby_w.field_0x001 = 0x15;
            if (self->mode_0x14 == 1) {
                lobby_w.field_0x002 = 2;
            } else {
                lobby_w.field_0x002 = 0;
            }
            PlayMode_set(4);
            create_move_work(1);
            lb_quest_return_apply();
            q = get_qResult_work();
            if (q != NULL && system_w.field_0x2d == 0 && (q->kind_0x1E3 == 1 || q->kind_0x1E3 == 3)) {
                if (game_data_file_create_start() != 0) {
                    self->state_0x0B = 1;
                    system_w.field_0x865 = 1;
                    continue;
                }
                if (game_system_file_create_start() != 0) {
                    self->state_0x0B = 2;
                    continue;
                }
            }
            break;
        case 1:
            switch (game_save_wait()) {
            case 1:
                if (game_system_file_create_start() != 0) {
                    self->state_0x0B = 2;
                    continue;
                }
                break;
            case -2:
            case -1:
                self->state_0x0B = 0x64;
                continue;
            default:
                continue;
            }
            break;
        case 2:
            switch (game_system_file_create_wait()) {
            case 1:
                break;
            case -2:
            case -1:
                self->state_0x0B = 0x64;
                continue;
            default:
                continue;
            }
            break;
        case 3:
            fn_801FB2A8(1, lobby_w.field_0x001, lobby_w.field_0x002);
            self->state_0x0B++;
            continue;
        case 4:
            if (file_loading_ck(NULL, NULL) != 1U) {
                fn_801FB364(1);
                lobby_w.slots_0x00C[lobby_w.field_0x014] = (u32)(s8)stage_area_slot_get(lobby_w.field_0x001, lobby_w.field_0x002);
                self->state_0x0B++;
            }
            continue;
        case 5:
            fn_801FB438();
            self->state_0x0B++;
            continue;
        case 6:
            stage_area_enter(lobby_w.field_0x001, lobby_w.field_0x002);
            lb_npc_map_setup();
            camera_area_default_set();
            self->step_0x0A++;
            self->state_0x0B = 0;
            NetCtrlWk::setFetchMode(1);
            return 0;
        case 0x64:
            if ((status & 0x10) != 0) {
                break;
            } else {
                s32 code;

                done = 1;
                code = chg_nand_err2msgcode();
                subTransSet((u32)fn_801FC6A0, 1, (u32*)&code);
            }
            continue;
        default:
            continue;
        }
        system_w.field_0x865 = 0;
        self->state_0x0B = 3;
    } while (0);
    if (self->state_0x0B >= 3) {
        player_control_move();
    }
    return done;
}

/* 0x801FBAE4 - hand the pending colour choice to the network side, then clear the option parameter
 * block back to its empty state. */
void fn_801FBAE4(void)
{
    Q_ResultWork* q = get_qResult_work();

    if (q->phase_0x1E2 == 3 && q->credit_0x3F0 > 0) {
        score_add_clamped(q->credit_0x3F0, (s32*)(lb_chg::lobby_world_block + 0x18));
    }
    lb_param_w.field_0x00 = 0;
    lb_param_w.field_0x04 = 0;
    lb_param_w.flag_0x0C[0] = 0;
    lb_param_w.value_0x10[0] = 0;
    lb_param_w.flag_0x0C[1] = 0;
    lb_param_w.value_0x10[1] = 0;
    lb_param_w.flag_0x0C[2] = 0;
    lb_param_w.value_0x10[2] = 0;
    lb_param_w.value_0x16 = 0;
    lb_param_w.value_0x18 = 0;
    lb_param_w.value_0x1A = 0;
    lb_param_w.value_0x1C = 0;
}

/* 0x801FBB64 (0x414): unlocks the lobby progress entries the quest clears earned: the two clear-count rewards, then
 * the reward of the current unlock condition when its flags are set. */
void lb_progress_rewards_check(void)
{
    u8 cleared;
    u8 rewards;
    u16 count;
    u16 total;
    u8 sets;
    u32 group;

    if (userdata_progress_flag_ck(0x5E) == 0 || userdata_progress_flag_ck(0x5D) == 0) {
        total = 0;
        group = 0;
        do {
            count = 0;
            quest_group_clear_count(&count, (u8)group);
            total += count;
            group++;
        } while (group < 8);
        if (total >= 15 && userdata_progress_flag_ck(0x5D) == 0) {
            lb_progress_unlock(0x5D);
        }
        if (total >= 25 && userdata_progress_flag_ck(0x5E) == 0) {
            lb_progress_unlock(0x5E);
        }
    }
    switch (lb_unlock_cond_top_get(0)) {
    case 25:
        if (userdata_flag_ck(0x7D5) == 1U) {
            lb_progress_unlock(0x31);
        }
        break;
    case 24:
        if (userdata_flag_ck(0x419) == 1U) {
            lb_progress_unlock(0x30);
        }
        break;
    case 22:
        if (userdata_flag_ck(0x414) == 1U) {
            lb_progress_unlock(0x2B);
        }
        if (userdata_flag_ck(0x415) == 1U) {
            lb_progress_unlock(0x2C);
        }
        break;
    case 20:
        sets = 0;
        if (userdata_flag_ck(0x410) == 1U) {
            sets = 1;
        }
        if (userdata_flag_ck(0x411) == 1U) {
            sets++;
        }
        if (userdata_flag_ck(0x412) == 1U) {
            sets++;
        }
        if (sets >= 2) {
            lb_progress_unlock(0x29);
        }
        break;
    case 19:
        if (userdata_flag_ck(0x7D4) == 1U) {
            lb_progress_unlock(0x28);
        }
        break;
    case 18:
        if (userdata_flag_ck(0x40C) == 1U) {
            lb_progress_unlock(0x27);
        }
        break;
    case 17:
        if (userdata_flag_ck(0x7D3) == 1U) {
            lb_progress_unlock(0x26);
        }
        break;
    case 16:
        cleared = 0;
        if (userdata_flag_ck(0x406) == 1U) {
            cleared = 1;
        }
        if (userdata_flag_ck(0x407) == 1U) {
            cleared++;
        }
        if (userdata_flag_ck(0x408) == 1U) {
            cleared++;
        }
        if (userdata_flag_ck(0x409) == 1U) {
            cleared++;
        }
        if (cleared >= 2) {
            lb_progress_unlock(0x25);
        }
        break;
    case 15:
        if (userdata_flag_ck(0x7D2) == 1U) {
            lb_progress_unlock(0x23);
        }
        break;
    case 14:
        if (userdata_flag_ck(0x403) == 1U) {
            lb_progress_unlock(0x22);
        }
        break;
    case 13:
        if (userdata_flag_ck(0x402) == 1U) {
            lb_progress_unlock(0x21);
        }
        break;
    case 12:
        if (userdata_flag_ck(0x400) == 1U) {
            lb_progress_unlock(0x20);
        }
        break;
    case 11:
        if (userdata_flag_ck(0x7D1) == 1U) {
            lb_progress_unlock(0x1F);
        }
        break;
    case 10:
        if (userdata_flag_ck(0x3F6) == 1U) {
            lb_progress_unlock(0x1E);
        }
        break;
    case 9:
        if (userdata_flag_ck(0x3F5) == 1U) {
            lb_progress_unlock(0x1D);
        }
        break;
    case 8:
        if (userdata_flag_ck(0x7D0) == 1U) {
            lb_progress_unlock(0x1C);
        }
        break;
    case 7:
        rewards = 0;
        if (userdata_flag_ck(0x3EB) == 1U) {
            lb_progress_unlock(0x38);
            rewards = 1;
        }
        if (userdata_flag_ck(0x3EC) == 1U) {
            lb_progress_unlock(0x39);
            lb_progress_unlock(0x1A);
            rewards++;
        }
        if (rewards >= 2) {
            lb_progress_unlock(0x24);
        }
        break;
    case 6:
        if (userdata_flag_ck(0x3E8) == 1U) {
            lb_progress_unlock(0x1B);
        }
        break;
    case 5:
        if (((LbItemDb*)lb_chg::lobby_world_block)->gallery_0x4918 != 0) {
            lb_progress_unlock(0x18);
            gallery_open(0xE);
            gallery_open(0x13);
            gallery_open(0x14);
        }
        break;
    }
}

} /* extern "C" */
#pragma peephole reset
