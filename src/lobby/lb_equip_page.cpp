/*
 * lobby/lb_equip_page.cpp - the lobby trade-ship screen (lobby screen 8), the player actor's model rig (its models,
 * dyes, hair/face parts and world-matrix callbacks) and the rig's per-attachment pose update.
 * RANGE. .text 0x80220038-0x80229ECC (118 functions, all written); .data 0x805BA750-0x805BDC48, .sdata
 *   0x80791EF0-0x80792030, .sdata2 0x80799CD8-0x80799E00, extab, extabindex.  Three runs: the trade-ship screen
 *   0x80220038-0x80222F18 (`lb_ship_*`, `lb_box_screen_*`, `lb_item_box_count_draw`), the model rig 0x80222F18-0x80224AC4
 *   and the rig update 0x80224AC4-0x80229ECC (`pl_rig_*`, `LbEquipModelCallbackA`/`B`).
 * FLAGS. `cflags_lobby`; `#pragma peephole off` (retail keeps the narrowing `clrlwi`s), back on around
 *   `lb_ship_sprite_draw`; `#pragma fp_contract off` over the rig half (retail keeps every `fmuls`+`fadds` unfused).
 * NAMES. `lb_equip_page` is a GUESS kept from the registration.  GUESS (from each body, its tables and its callers):
 *   every `lb_ship_*` and `pl_rig_*` name (the screen sends three ships out on voyages that bring goods back; the
 *   `pl_rig_kindN_*` rows are named by the attachment kind `pl_rig_attach_kind_tbl` gives, the `pl_rig_kN_*` key tables
 *   by kind and motion number), `lb_box_screen_open`/`_close`/`_draw`, `res_ofs_ptr_get`, `lb_item_box_count_draw`,
 *   `pl_equip_color_apply`, `res_mat_name_get`, `pl_slot_motion_allow_get`, `se_req_frame_set_parts3`,
 *   `se_req_frame_set_part4`, the classes and `ApplyTiming`, the `LbShip*`/`PlRig*` types and every renamed data row.
 *   Registry GUESSes for foreign callees: `mhchar_mdl_prepare`, `equip_blade_dye_ck`, `pl_equip_file_get`,
 *   `res_file_handle_copy`, `pl_weapon_alt_form_ck`, `res_node_mtx_id_get`, `vec3_float_copy`, `equip_variant_get`,
 *   `pl_charge_cycle_get`, `anm_chr_result_rot_get`/`_rot_set`/`_mtx_get`/`_mtx_set`, `mtx34_rot_xyz_deg`,
 *   `mhchar_wld_mtx_array_get`, `pl_rig_hair_style_tbls`, `pl_rig_face_style_tbls`.
 * RESIDUALS.
 *  - `pl_rig_kind0/2/4/8_pose`: retail jumps from several cases into shared pose blocks (a `goto` shape); the bodies use
 *    a pose flag and one switch after the case switch instead (kind4: 87.62 with the flag, 60.45 with the blocks
 *    inlined per case; kind8 79.15, kind2 86.58, kind0 88.36);
 *  - `LbEquipModelCallbackB::ExecCallbackB`: the same shape for the sway chain's found block (92.22);
 *  - `lb_ship_holds_fill`: retail recomputes `seed % 100` for the second roll test (ours keeps the first value);
 *  - `lb_ship_sprite_draw`: retail zero-extends the `u16 id` (`clrlwi r3,r3,16`), ours passes the parameter through;
 *  - `lb_ship_order_step`: the `toggle_word_step` mask argument is `clrlwi`-ed (the owner's prototype says `s32`);
 *  - `pl_rig_model_setup`/`pl_rig_weapon_load`: retail passes the light index unextended (`mhchar_model_create`'s
 *    prototype says `u8`);
 *  - `pl_rig_weapon_kind_set`/`pl_rig_pose_get`: retail tests the weapon-type range 4-6 with `subi`/`cmplwi`, our
 *    switch with `cmpwi`/`blt`;
 *  - `res_ofs_ptr_get`: retail branches over the add (`beq`), ours returns early (`bnelr`);
 *  - `pl_rig_model_move` and the remaining rows: register colouring only.
 *   flipcheck: `.data`/`.sdata` claimed, not emitted (only the two class tables and the switch tables are); the
 *   `.data` run is not in one TU's emission order (the class tables at 0x805BAB58 sit between the rig's tables), so
 *   it is left declared.  The inline base destructor `__dt__Q34nw4r3g3d18ICalcWorldCallbackFv` is emitted here (the
 *   g3d header defines it inline; retail references it), one extab/extabindex entry more than retail.
 *   `pl_rig_hair_style_tbls`/`pl_rig_face_style_tbls` (.sdata 0x807922B0/0x807922B8) lie outside every registered
 *   range (`unsplit/pl_rig_style.h`).
 */
#include "types.h"
#include "nw4r/math.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMat (rule 2) */

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#include "quest/quest_types.h"               /* Q_UserData, LbShipBlock (rule 1) */
#include "lobby/lobby_w.h"                   /* lobby_w */
#include "fn_80047398/lobby_world_block.h"   /* lobby_world_block */
#include "hud/layout.h"                      /* the sprite/font putters and _SPR_DATA_ */
#include "hud/cockpit.h"                     /* chk_pointer, ainpc_page_hold_set, menu_money_draw */
#include "hud/put_button_guide.h"
#include "lobby/lb_list_init.h"              /* the list and panel helpers */
#include "lobby/lb_cmd_pressed_ck.h"         /* the lobby command tests, weight_table_index */
#include "lobby/lb_cursor_prompt_draw.h"
#include "lobby/lb_button_prompt_draw.h"
#include "lobby/lb_speaker_name_get.h"
#include "lobby/lb_points_draw.h"
#include "lobby/lb_item_info_draw.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_progress_unlock.h"
#include "lobby/LbStr.h"
#include "lobby/lb_box_screen_step.h"
#include "lobby/lb_item_box_count_draw.h"
#include "menu/menu_message.h"               /* menu_cursor_step*, toggle_word_step, GetMenuFontColor */
#include "menu/menu_item.h"                  /* ItemName, GetItemData, put_menu_cursor */
#include "menu/get_pop_dat_ptr.h"            /* get_option_cfg */
#include "ef/eft052.h"
#include "ef/eft052_item_get_open.h"
#include "ef/eft052_box_open.h"
#include "ef/system_core.h"                  /* rand_lcg_step */
#include "ef/fn_800CDB2C.h"                  /* ran_suu */
#include "sound/fn_800D7F54.h"               /* sysSE_req, sysSE_stop */
#include "sound/set_zmode__FbUcb.h"           /* set_zmode, set_blendmode */
#include "camera/camera.h"                   /* camera_talk_lock_set */
#include "userdata_item.h"                   /* item_take, subTransSet */
#include "get_FqResult_work.h"               /* userdata_zenny_add */
#include "font/flfnt.h"                      /* font_set_size */
#include "MSL_C/printf.h"                    /* sprintf */
#include "main.h"                            /* ck_WideMode */
#include "lobby/lb_npc.h"                    /* lb_item_get_data */

/* One row of a trade goods table: an item and its price in resource points.  size: 0x4 */
typedef struct LbShipGoods {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ u16 price_0x02;
} LbShipGoods;

/* One ship tab of the order page (`lb_ship_tabs`): its layout, the cursor and tab animations, the tab's sprite
 * rows and the arrow rows shown when there are several ships.  size: 0x10 */
typedef struct LbShipTab {
    /* +0x00 */ u16 layout_0x00;
    /* +0x02 */ u16 cursor_anim_0x02;
    /* +0x04 */ u16 tab_anim_0x04;
    /* +0x06 */ u16 pad_0x06;
    /* +0x08 */ const u16* sprites_0x08;
    /* +0x0C */ const u16* arrows_0x0C;
} LbShipTab;

/* The item-hold strip entry the order page hands `eft052_hold_entry_set` (`ef/eft052.cpp`'s `CockpitHoldEntry`
 * layout).  size: 0x1C */
typedef struct LbShipHoldEntry {
    /* +0x00 */ s32 limit_0x00;
    /* +0x04 */ s16 msg_0x04;
    /* +0x06 */ s16 msg2_0x06;
    /* +0x08 */ s16 title_0x08;
    /* +0x0A */ s16 count_msg_0x0A;
    /* +0x0C */ s16 item_count_0x0C;
    /* +0x0E */ u16 sprite_0x0E;
    /* +0x10 */ u16 highlight_0x10;
    /* +0x12 */ u16 pad_0x12;
    /* +0x14 */ u16* items_0x14;
    /* +0x18 */ u8 kind_0x18;
    /* +0x19 */ u8 pad_0x19[3];
} LbShipHoldEntry;

/* The trade-ship screen's work block (`lobby_w.menu_0xAC` while screen 8 is open; `lb_box_screen_open` clears its
 * 0x2000 bytes): the screen step, the order page's input mode, the top menu, the per-field greying, the order being
 * edited, the item lists the hold strip shows and the three ships' holds as item-box cells.  size: 0x2000 */
typedef struct LbShipWork {
    /* +0x000 */ u8 step_0x000;           /* 0 opening, 1 menu, 2 hold, 3 order, 4 view, 5 closing */
    /* +0x001 */ u8 mode_0x001;           /* the order page: 0 ship, 1 field, 2 hold strip, 3 confirm, 5 discard */
    /* +0x002 */ u8 pad_0x002[2];
    /* +0x004 */ LbList menu_0x004;
    /* +0x024 */ u8 field_off_0x024[4];   /* nonzero greys an order field out */
    /* +0x028 */ u16 wait_0x028;
    /* +0x02A */ u16 confirm_anim_0x02A;
    /* +0x02C */ u16 sail_anim_0x02C;     /* 0 docked, 1 sailing, 2 just sent */
    /* +0x02E */ s16 ship_0x02E;
    /* +0x030 */ s16 field_0x030;         /* 0 goods, 1 bonus, 2 count, 3 send */
    /* +0x032 */ u8 pad_0x032[0xA];
    /* +0x03C */ s16 count_max_0x03C;
    /* +0x03E */ u8 pad_0x03E[2];
    /* +0x040 */ s32 affordable_0x040;
    /* +0x044 */ u8 pad_0x044[2];
    /* +0x046 */ s16 price_0x046;
    /* +0x048 */ s32 confirm_0x048;
    /* +0x04C */ LbShipOrder order_0x04C;
    /* +0x054 */ u8 pad_0x054[2];
    /* +0x056 */ u8 goods_set_0x056;
    /* +0x057 */ u8 bonus_set_0x057;
    /* +0x058 */ s32 arg_a_0x058;
    /* +0x05C */ s32 arg_b_0x05C;
    /* +0x060 */ u16 list_0x060[0x40];
    /* +0x0E0 */ Eft052BoxCell holds_0x0E0[3][30];
    /* +0x248 */ Eft052BoxCell sorted_0x248[90];
    /* +0x3B0 */ u8 unused_0x3B0[0x2000 - 0x3B0];
} LbShipWork;

/* The tables the trade-ship screen reads (this unit's `.data`/`.sdata`). */
extern "C" {
extern const LbShipGoods* lb_ship_goods_tables[4];
extern const u16* lb_ship_bonus_tables[4];
extern u16 lb_ship_filler_items[];
extern s16 lb_ship_roll_weights[];
extern u8 lb_ship_kind_chance[];
extern u16 lb_ship_menu_sprites[];
extern u16 lb_ship_menu_rows[4];
extern u16 lb_ship_header_sprites[4];
extern u16 lb_ship_header_frame[];
extern u16 lb_ship_order_frame[];
extern u16 lb_ship_slot_frame[];
extern u16 lb_ship_grade_icons_info[3];
extern char lb_ship_count_fmt[3];
extern u16 lb_ship_order_prompt[4];
extern u16 lb_ship_slot_busy[4];
extern u16 lb_ship_slot_free[4];
extern u16 lb_ship_field_goods[];
extern u16 lb_ship_field_bonus[];
extern u16 lb_ship_field_count[];
extern u16 lb_ship_confirm_anim[];
extern u16 lb_ship_edit_frame[];
extern u16 lb_ship_cursor_rows[];
extern u16 lb_ship_count_cursor_rows[];
extern u16 lb_ship_grade_icons_edit[3];
extern LbShipTab lb_ship_tabs[3];
extern u16 lb_ship_panel_frame[];
extern u16 lb_ship_panel_closed[];
extern u16 lb_ship_panel_open[];
extern u8 lb_ship_tab_order[3][3];
extern u16 lb_ship_levels_frame[];
extern u16 lb_ship_level_rows[];
extern u16 lb_ship_level_icons[3];
extern u16 lb_ship_level_layouts[4];
extern u16 lb_ship_grade_icons_view[3];
extern u16 lb_ship_backdrop[];
extern u16 lb_ship_backdrop_4x3[];
extern u16 lb_ship_backdrop_wide[];
extern u16 lb_ship_cell_columns[10];
extern u16 lb_ship_cell_rows[3];
extern u16 lb_ship_box_layouts[3];
extern u16 lb_ship_box_frames[];
extern u16 lb_ship_cell_sprites[3];
extern u16 lb_ship_box_backs[];
extern u16 lb_ship_box_names[3];
extern u16 lb_ship_box_list_rows[4];
}

/* The save block `lobby_world_block` points at. */
static inline Q_UserData* lb_userdata(void)
{
    return (Q_UserData*)lobby_world_block;
}

/* The trade-ship screen's work block. */
static inline LbShipWork* lb_ship_work(void)
{
    return (LbShipWork*)lobby_w.menu_0xAC;
}

/* Retail keeps the narrowing `clrlwi` the peephole pass folds away; `lb_ship_sprite_draw` keeps the pass on. */
#pragma peephole off

extern "C" {

u16 lb_ship_price_get(u16 item);
s16 lb_ship_goods_table_get(u16 item);
s16 lb_ship_bonus_table_get(u16 item);
void lb_box_screen_close(void);
void lb_ship_order_copy(LbShipOrder* dst, LbShipOrder* src);
void lb_ship_order_reset(LbShipWork* work);
s16 lb_ship_count_max_get(u8* grades, u16* item);
s32 lb_ship_list_has(LbShipWork* work, s32 count, u16 item);
s16 lb_ship_goods_list_build(LbShipWork* work, s32 table);
s16 lb_ship_bonus_list_build(LbShipWork* work, s32 table);
s32 lb_ship_order_step(LbShipWork* work);
void lb_ship_sprite_draw(u16 id);
u32 lb_ship_menu_active_ck(LbShipWork* work);
void lb_ship_order_frame_draw(LbShipWork* work, u8 prompt);
void lb_ship_order_edit_draw(LbShipWork* work);
void lb_ship_tabs_draw(LbShipWork* work);
void lb_ship_order_view_draw(LbShipWork* work);
void lb_ship_levels_draw(LbShipWork* work);
void lb_ship_cell_pos_get(s32 cell, _mh_ivec2_* out);
void lb_ship_box_grid_draw(Eft052ItemBox* box, u8 ship);
void lb_ship_box_draw(Eft052ItemBox* box);
void lb_box_screen_draw(void);

/* 0x80220038 (0x40): Opens the trade ship: unlocks its progress entry, the first two goods tables and one ship. */
void lb_ship_open(void)
{
    Q_UserData* ud = lb_userdata();

    lb_progress_unlock(0x19);
    ud->ship_0x4654.grade_0x00[0] = 1;
    ud->ship_0x4654.grade_0x00[1] = 1;
    ud->ship_0x4654.ships_0x1C = 1;
}

/* 0x80220078 (0x9C): Marks each sailing ship's goods table (plus one) in the lobby's NPC event bytes. */
void lb_ship_marks_set(void)
{
    u8 i;

    memset(lobby_w.npc_event_0x07A, 0xFF, 3);
    for (i = 0; i < lb_userdata()->ship_0x4654.ships_0x1C; i++) {
        u8 mark;

        if (lb_userdata()->ship_0x4654.orders_0x04[i].item_0x00 == 0) {
            mark = 0;
        } else {
            mark = lb_ship_goods_table_get(lb_userdata()->ship_0x4654.orders_0x04[i].item_0x00) + 1;
        }
        lobby_w.npc_event_0x07A[i] = mark;
    }
}

/* 0x80220114 (0x48): Counts one voyage down on every ship still at sea. */
void lb_ship_voyage_tick(void)
{
    LbShipBlock* ship = &lb_userdata()->ship_0x4654;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (ship->orders_0x04[i].voyages_0x04) {
            ship->orders_0x04[i].voyages_0x04--;
        }
    }
}

/* 0x8022015C (0x394): Fills the three ships' holds with what each voyage since the last visit brought back: the
 * ordered goods or the filler item per roll, plus a rare bonus for the special order. */
void lb_ship_holds_fill(LbShipWork* work, LbShipBlock* trade)
{
    s32 ship;

    memset(work->holds_0x0E0, 0, sizeof(work->holds_0x0E0));
    for (ship = 0; ship < 3; ship++) {
        LbShipOrder* order = &trade->orders_0x04[ship];
        s16 voyages;

        for (voyages = order->voyages_seen_0x05 - order->voyages_0x04; voyages > 0; voyages--) {
            Eft052BoxCell* cell;
            s32 i;
            s16 rolls;
            s16 table;
            u16 chance;
            u16 got;
            u16 missed;
            u16 bonus;
            s32 roll;

            for (i = 0; i < 30; i++) {
                if (work->holds_0x0E0[ship][i].item_0x00 == 0) {
                    cell = &work->holds_0x0E0[ship][i];
                    break;
                }
            }
            order->seed_0x06 = rand_lcg_step(order->seed_0x06);
            rolls = weight_table_index(lb_ship_roll_weights, 4, order->seed_0x06 % 100) + 1;
            table = lb_ship_goods_table_get(order->item_0x00);
            rolls += (s16)((trade->grade_0x00[table] - 1) * 2);
            chance = lb_ship_kind_chance[GetItemData(order->item_0x00)->level_0x01];
            switch (order->bonus_0x02) {
            case 0xF4:
            case 0xE0:
            case 0xE2:
            case 0xC5:
                chance += 10;
                rolls += 2;
                break;
            case 0xEE:
            case 0xF7:
            case 0x1A5:
            case 0xE1:
                chance += 20;
                rolls += 2;
                break;
            }
            got = 0;
            missed = 0;
            bonus = 0;
            for (roll = 0; roll < rolls; roll++) {
                order->seed_0x06 = rand_lcg_step(order->seed_0x06);
                if (chance > order->seed_0x06 % 100) {
                    got++;
                }
                if (chance <= order->seed_0x06 % 100) {
                    missed++;
                }
            }
            if (order->bonus_0x02 == 0xF5) {
                if (trade->grade_0x00[table] == 3) {
                    order->seed_0x06 = rand_lcg_step(order->seed_0x06);
                    if (order->seed_0x06 % 100 < 20) {
                        order->seed_0x06 = rand_lcg_step(order->seed_0x06);
                        if (order->seed_0x06 % 100 < 20) {
                            bonus = 0x9D;
                        } else {
                            bonus = 0xE3;
                        }
                    }
                } else {
                    order->seed_0x06 = rand_lcg_step(order->seed_0x06);
                    if (order->seed_0x06 % 100 < 20) {
                        bonus = 0xE3;
                    }
                }
            }
            if (got != 0) {
                item_take(order->item_0x00, got, (IdValue*)cell, 3, 0, 99);
            }
            if (missed != 0) {
                item_take(lb_ship_filler_items[table], missed, (IdValue*)cell, 3, 0, 99);
            }
            if (bonus != 0) {
                item_take(bonus, 1, (IdValue*)cell, 3, 0, 99);
            }
        }
    }
}

/* 0x802204F0 (0xE8): Returns the price of `item` in the first unlocked goods table that lists it, else 0. */
u16 lb_ship_price_get(u16 item)
{
    Q_UserData* ud = lb_userdata();
    s32 i;

    for (i = 0; i < 4; i++) {
        if (ud->ship_0x4654.grade_0x00[i] != 0) {
            const LbShipGoods* goods;

            for (goods = lb_ship_goods_tables[i]; goods->item_0x00 != 0; goods++) {
                if (goods->item_0x00 == item) {
                    return goods->price_0x02;
                }
            }
        }
    }
    return 0;
}

/* 0x802205D8 (0xE8): Returns the unlocked goods table that lists `item` (0 when none does). */
s16 lb_ship_goods_table_get(u16 item)
{
    Q_UserData* ud = lb_userdata();
    s16 i;

    for (i = 0; i < 4; i++) {
        if (ud->ship_0x4654.grade_0x00[i] != 0) {
            const LbShipGoods* goods;

            for (goods = lb_ship_goods_tables[i]; goods->item_0x00 != 0; goods++) {
                if (goods->item_0x00 == item) {
                    return i;
                }
            }
        }
    }
    return 0;
}

/* 0x802206C0 (0xE8): Returns the unlocked bonus list that lists `item` (0 when none does). */
s16 lb_ship_bonus_table_get(u16 item)
{
    Q_UserData* ud = lb_userdata();
    s16 i;

    for (i = 0; i < 4; i++) {
        if (ud->ship_0x4654.grade_0x00[i] != 0) {
            const u16* bonus;

            for (bonus = lb_ship_bonus_tables[i]; *bonus != 0; bonus++) {
                if (*bonus == item) {
                    return i;
                }
            }
        }
    }
    return 0;
}

/* 0x802207A8 (0x70): Opens the trade-ship screen (lobby screen 8) with its two arguments. */
void lb_box_screen_open(s32 arg_a, s32 arg_b)
{
    LbShipWork* work = lb_ship_work();

    memset(work, 0, 0x2000);
    work->arg_a_0x058 = arg_a;
    work->arg_b_0x05C = arg_b;
    lobby_w.state_0x000 = 8;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* 0x80220818 (0x4): Closes the screen. */
void lb_box_screen_close(void)
{
    lb_panel_close();
}

/* 0x8022081C (0x8C): Sends the edited order: pays for it, takes the two items and stores the order on its ship. */
void lb_ship_order_commit(LbShipWork* work, LbShipBlock* trade)
{
    work->order_0x04C.voyages_seen_0x05 = work->order_0x04C.voyages_0x04;
    eft052_page_count_add(work->order_0x04C.item_0x00, 1);
    userdata_zenny_add(-(work->price_0x046 * work->order_0x04C.voyages_0x04));
    eft052_page_count_add(work->order_0x04C.bonus_0x02, 1);
    work->order_0x04C.seed_0x06 = ran_suu(1);
    lb_ship_order_copy(&trade->orders_0x04[work->ship_0x02E], &work->order_0x04C);
}

/* 0x802208A8 (0x2C): Copies one order. */
void lb_ship_order_copy(LbShipOrder* dst, LbShipOrder* src)
{
    *dst = *src;
}

/* 0x802208D4 (0xA8): Clears the order being edited and re-greys its fields; the sail icon follows the ship's own
 * order. */
void lb_ship_order_reset(LbShipWork* work)
{
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;

    work->field_off_0x024[0] = 1;
    work->field_off_0x024[1] = 1;
    work->field_off_0x024[2] = 1;
    work->field_off_0x024[3] = 1;
    memset(&work->order_0x04C, 0, sizeof(work->order_0x04C));
    work->price_0x046 = 0;
    work->count_max_0x03C = 0;
    work->field_0x030 = 0;
    work->field_off_0x024[0] = 0;
    work->field_off_0x024[1] = 0;
    work->goods_set_0x056 = 0;
    work->bonus_set_0x057 = 0;
    if (trade->orders_0x04[work->ship_0x02E].item_0x00 != 0) {
        work->sail_anim_0x02C = 1;
    } else {
        work->sail_anim_0x02C = 0;
    }
}

/* 0x8022097C (0x18): Leaves the order page: back to the ship row, then the reset above. */
void lb_ship_page_reset(LbShipWork* work)
{
    work->ship_0x02E = 0;
    work->field_0x030 = 0;
    work->mode_0x001 = 0;
    work->sail_anim_0x02C = 0;
    lb_ship_order_reset(work);
}

/* 0x80220994 (0x15C): Moves the field cursor one step (up when the up command repeats), skipping greyed fields;
 * returns the new field, or the old one when every other field is greyed. */
s16 lb_ship_field_step(LbShipWork* work)
{
    s32 dir;
    s32 i;

    dir = lb_cmd_repeat_ck(1) != 0 ? -1 : 1;
    for (i = 1; i < 4; i++) {
        s16 field = work->field_0x030 + dir * i;

        if (field < 0) {
            field = field + 4;
        } else {
            field = field % 4;
        }
        if (work->field_off_0x024[field] == 0) {
            sysSE_req(3);
            return field;
        }
    }
    return work->field_0x030;
}

/* 0x80220AF0 (0x60): The most voyages an order of `item` may book, from its goods table's grade. */
s16 lb_ship_count_max_get(u8* grades, u16* item)
{
    switch (grades[lb_ship_goods_table_get(*item)]) {
    case 3:
        return 10;
    case 2:
        return 5;
    default:
        return 3;
    }
}

/* 0x80220B50 (0x34): Whether `item` is among the first `count` entries of the hold-strip list. */
s32 lb_ship_list_has(LbShipWork* work, s32 count, u16 item)
{
    s32 i;

    for (i = 0; i < count; i++) {
        if (work->list_0x060[i] == item) {
            return 1;
        }
    }
    return 0;
}

/* 0x80220B84 (0x150): Builds the goods list of `table` (every unlocked table for -1, or for the special bonus); returns
 * its length. */
s16 lb_ship_goods_list_build(LbShipWork* work, s32 table)
{
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;
    s16 count;
    s16 i;

    memset(work->list_0x060, 0, sizeof(work->list_0x060));
    count = 0;
    if (table != -1 && work->order_0x04C.bonus_0x02 != 0xF5) {
        const LbShipGoods* goods;

        for (goods = lb_ship_goods_tables[table]; goods->item_0x00 != 0; goods++) {
            if (count >= 0x40) {
                break;
            }
            work->list_0x060[count] = goods->item_0x00;
            count++;
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (trade->grade_0x00[i] != 0) {
                const LbShipGoods* goods;

                for (goods = lb_ship_goods_tables[i]; goods->item_0x00 != 0; goods++) {
                    if (count >= 0x40) {
                        break;
                    }
                    work->list_0x060[count] = goods->item_0x00;
                    count++;
                }
            }
        }
    }
    return count;
}

/* 0x80220CD4 (0x128): Builds the bonus list of `table` (every unlocked list for -1) without repeats; returns its
 * length. */
s16 lb_ship_bonus_list_build(LbShipWork* work, s32 table)
{
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;
    s16 count;
    s16 i;

    memset(work->list_0x060, 0, sizeof(work->list_0x060));
    count = 0;
    if (table != -1) {
        const u16* bonus;

        for (bonus = lb_ship_bonus_tables[table]; *bonus != 0; bonus++) {
            if (count >= 0x40) {
                break;
            }
            if (lb_ship_list_has(work, count, *bonus) == 0) {
                work->list_0x060[count] = *bonus;
                count++;
            }
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (trade->grade_0x00[i] != 0) {
                const u16* bonus;

                for (bonus = lb_ship_bonus_tables[i]; *bonus != 0; bonus++) {
                    if (count >= 0x40) {
                        break;
                    }
                    if (lb_ship_list_has(work, count, *bonus) == 0) {
                        work->list_0x060[count] = *bonus;
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

/* 0x80220DFC (0x5B0): One input step of the order page; returns 2 when the player backs out of it. */
s32 lb_ship_order_step(LbShipWork* work)
{
    s32 result = 0;
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;

    if (work->sail_anim_0x02C == 2) {
        work->sail_anim_0x02C = 1;
    }
    switch (work->mode_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (trade->orders_0x04[work->ship_0x02E].item_0x00 == 0) {
                work->mode_0x001 = 1;
                work->sail_anim_0x02C = 0;
                sysSE_req(5);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            work->ship_0x02E = menu_cursor_step_open_last(work->ship_0x02E, trade->ships_0x1C, lb_cmd_repeat_get(), 4, 8, 6);
            lb_ship_order_reset(work);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            switch (work->field_0x030) {
            case 0: {
                LbShipHoldEntry entry;
                s16 count;

                work->mode_0x001 = 2;
                if (work->order_0x04C.bonus_0x02 != 0) {
                    count = lb_ship_goods_list_build(work, lb_ship_bonus_table_get(work->order_0x04C.bonus_0x02));
                } else {
                    count = lb_ship_goods_list_build(work, -1);
                }
                entry.limit_0x00 = 0;
                entry.msg_0x04 = 0x12B;
                entry.msg2_0x06 = -1;
                entry.count_msg_0x0A = -1;
                entry.title_0x08 = 0x12C;
                entry.sprite_0x0E = 0x13E3;
                entry.highlight_0x10 = 0;
                entry.item_count_0x0C = count;
                entry.items_0x14 = work->list_0x060;
                entry.kind_0x18 = 0;
                eft052_hold_entry_set(&entry, 0);
                sysSE_req(5);
                break;
            }
            case 1: {
                LbShipHoldEntry entry;
                s16 count;

                work->mode_0x001 = 2;
                if (work->order_0x04C.item_0x00 != 0) {
                    count = lb_ship_bonus_list_build(work, lb_ship_goods_table_get(work->order_0x04C.item_0x00));
                } else {
                    count = lb_ship_bonus_list_build(work, -1);
                }
                entry.limit_0x00 = 0;
                entry.msg_0x04 = 0x130;
                entry.msg2_0x06 = 0x134;
                entry.count_msg_0x0A = -1;
                entry.title_0x08 = 0x131;
                entry.highlight_0x10 = work->order_0x04C.bonus_0x02;
                entry.sprite_0x0E = 0x13E4;
                entry.item_count_0x0C = count;
                entry.items_0x14 = work->list_0x060;
                entry.kind_0x18 = 0;
                eft052_hold_entry_set(&entry, 0);
                sysSE_req(5);
                break;
            }
            case 3:
                work->mode_0x001 = 3;
                work->confirm_anim_0x02A = 0;
                work->confirm_0x048 = 0;
                sysSE_req(0);
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            if (work->goods_set_0x056 != 0 || work->bonus_set_0x057 != 0) {
                work->mode_0x001 = 5;
                work->confirm_0x048 = 0;
            } else {
                work->mode_0x001 = 0;
                work->field_0x030 = 0;
                lb_ship_order_reset(work);
            }
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            if (work->field_0x030 == 2) {
                if (lb_cmd_repeat_ck(4) != 0) {
                    if (work->order_0x04C.voyages_0x04 > 1) {
                        work->order_0x04C.voyages_0x04--;
                        sysSE_req(6);
                    }
                } else {
                    work->order_0x04C.voyages_0x04++;
                    if (work->order_0x04C.voyages_0x04 > work->count_max_0x03C) {
                        work->order_0x04C.voyages_0x04 = work->count_max_0x03C;
                    } else {
                        sysSE_req(6);
                    }
                }
                if (work->order_0x04C.voyages_0x04 <= work->affordable_0x040) {
                    work->field_off_0x024[3] = 0;
                } else {
                    work->field_off_0x024[3] = 1;
                }
            }
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->field_0x030 = lb_ship_field_step(work);
        }
        break;
    case 2:
        switch (eft052_hold_step()) {
        case 1:
            work->mode_0x001 = 1;
            switch (work->field_0x030) {
            case 0: {
                s16 count_max;
                s32 affordable;

                eft052_hold_row_get(&work->order_0x04C.item_0x00, 0);
                work->price_0x046 = lb_ship_price_get(work->order_0x04C.item_0x00);
                work->order_0x04C.voyages_0x04 = 1;
                count_max = lb_ship_count_max_get(trade->grade_0x00, &work->order_0x04C.item_0x00);
                work->count_max_0x03C = count_max;
                affordable = lb_userdata()->points_0x3F04 / work->price_0x046;
                work->affordable_0x040 = affordable;
                if (affordable > count_max) {
                    work->affordable_0x040 = count_max;
                }
                work->field_off_0x024[2] = 0;
                work->goods_set_0x056 = 1;
                if (work->order_0x04C.voyages_0x04 <= work->affordable_0x040) {
                    work->field_off_0x024[3] = 0;
                } else {
                    work->field_off_0x024[3] = 1;
                }
                break;
            }
            case 1: {
                u16 bonus;

                eft052_hold_row_get(&bonus, 0);
                if (work->order_0x04C.bonus_0x02 == bonus) {
                    work->order_0x04C.bonus_0x02 = 0;
                } else {
                    work->order_0x04C.bonus_0x02 = bonus;
                }
                work->bonus_set_0x057 = 1;
                break;
            }
            }
            break;
        case 2:
            work->mode_0x001 = 1;
            break;
        }
        break;
    case 3:
        work->confirm_anim_0x02A++;
        if (work->confirm_anim_0x02A >= 20) {
            work->confirm_anim_0x02A = 0;
        }
        switch (toggle_word_step(&work->confirm_0x048, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            work->mode_0x001 = 0;
            lb_ship_order_commit(work, trade);
            lb_ship_order_reset(work);
            work->sail_anim_0x02C = 2;
            sysSE_stop(0x14);
            break;
        case 2:
            work->mode_0x001 = 1;
            break;
        }
        break;
    case 5:
        switch (lb_yes_no_step(&work->confirm_0x048)) {
        case 1:
            work->mode_0x001 = 0;
            work->field_0x030 = 0;
            lb_ship_order_reset(work);
            break;
        case 2:
            work->mode_0x001 = 1;
            break;
        }
        break;
    }
    return result;
}

/* 0x802213AC (0x308): The trade-ship screen's step: the top menu, the holds, the order page, the ship view and the
 * closing talk page. */
void lb_box_screen_step(void)
{
    LbShipWork* work = lb_ship_work();
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;
    s32 i;

    switch (work->step_0x000) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            work->step_0x000++;
            work->wait_0x028 = 0;
            lb_list_init(&work->menu_0x004, 3, 0, 0x176, 0x125, 0, -1, 3, 0);
            camera_talk_lock_set(1);
            sysSE_stop(0x29);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        if (work->wait_0x028 != 0) {
            work->wait_0x028--;
        }
        switch (lb_choice_step(&work->menu_0x004)) {
        case 1:
            work->mode_0x001 = 0;
            work->field_0x030 = 0;
            work->ship_0x02E = 0;
            switch (work->menu_0x004.cursor_0x00) {
            case 1:
                work->step_0x000 = 2;
                sysSE_stop(0xC);
                ((Eft052BoxGrid*)lb_item_get_data)->rows_0x02 = trade->ships_0x1C * 3;
                lb_ship_holds_fill(work, trade);
                eft052_box_open(work->holds_0x0E0[0], work->sorted_0x248, (Eft052BoxGrid*)lb_item_get_data);
                break;
            case 0:
                work->step_0x000 = 3;
                sysSE_stop(0xE);
                lb_ship_page_reset(work);
                break;
            case 2:
                work->step_0x000 = 4;
                sysSE_stop(0xE);
                lb_ship_order_reset(work);
                break;
            }
            break;
        case 2:
            work->step_0x000 = 5;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            lb_ship_marks_set();
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if (eft052_item_get_step() == 1) {
            work->step_0x000 = 1;
            for (i = 0; i < 3; i++) {
                trade->orders_0x04[i].voyages_seen_0x05 = trade->orders_0x04[i].voyages_0x04;
                if (trade->orders_0x04[i].voyages_seen_0x05 == 0) {
                    trade->orders_0x04[i].item_0x00 = 0;
                    trade->orders_0x04[i].bonus_0x02 = 0;
                }
            }
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if ((u32)(lb_ship_order_step(work) - 1) <= 1) {
            work->step_0x000 = 1;
        }
        break;
    case 4:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(0x20) != 0) {
            work->step_0x000 = 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            work->ship_0x02E = menu_cursor_step(work->ship_0x02E, trade->ships_0x1C, lb_cmd_repeat_get(), 4, 8);
            lb_ship_order_reset(work);
        }
        break;
    case 5:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_box_screen_close();
        }
        break;
    }
    subTransSet((u32)lb_box_screen_draw, 0, 0);
}

#pragma peephole on
/* 0x802216B4 (0x34): Draws the sprite rows of the trade header at layout `id`. */
void lb_ship_sprite_draw(u16 id)
{
    _mh_ivec2_ pos;

    get_lsp_data(id, &pos);
    draw_sprite_ary(lb_ship_header_sprites, &pos);
}
#pragma peephole off

/* 0x802216E8 (0x17C): Puts the order page's help line for its mode. */
void lb_ship_help_draw(LbShipWork* work)
{
    s16 msg;
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;

    switch (work->mode_0x001) {
    case 0: {
        LbShipOrder* order = &trade->orders_0x04[work->ship_0x02E];

        if (order->item_0x00 != 0) {
            if (order->voyages_0x04 != 0) {
                lb_panel_str_print(0x1877, 0x136, 2);
            } else {
                lb_panel_str_print(0x1877, 0x135, 2);
            }
        }
        msg = 0x129;
        break;
    }
    case 1:
        switch (work->field_0x030) {
        case 0:
            msg = 0x12A;
            break;
        case 2:
            msg = 0x12D;
            break;
        case 1:
            msg = 0x12F;
            break;
        case 3:
            msg = 0x132;
            break;
        }
        if (work->order_0x04C.voyages_0x04 > work->affordable_0x040) {
            lb_panel_str_print(0x1877, 0x12E, 2);
        }
        break;
    case 2:
        if (work->field_0x030 != 0) {
            return;
        }
        return;
    case 5:
        lb_panel_msg_draw(0x1877, 0x137);
        lb_panel_yes_no_draw(0x1879, work->confirm_0x048);
        return;
    case 3:
        lb_help_num_put(0x1877, 0x128, work->price_0x046 * work->order_0x04C.voyages_0x04);
        lb_panel_yes_no_draw(0x1879, work->confirm_0x048);
        return;
    default:
        return;
    }
    lb_panel_msg_draw(0x1877, msg);
}

/* 0x80221864 (0x2C): Whether the order page is editing a field (the order step, field mode or hold strip). */
u32 lb_ship_menu_active_ck(LbShipWork* work)
{
    if (work->step_0x000 == 3 && (u32)(work->mode_0x001 - 1) <= 1) {
        return 1;
    }
    return 0;
}

/* 0x80221890 (0x32C): Draws the selected ship's order card: captain, goods, its table, the voyages and the bonus. */
void lb_ship_order_info_draw(LbShipWork* work)
{
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;
    LbShipOrder* order = &trade->orders_0x04[work->ship_0x02E];
    _mh_ivec2_ pos;
    char text[16];
    _SPR_DATA_ spr;
    _SPR_DATA_* tint;
    s32 tint_id;

    get_lsp_data(0x131B, &pos);
    draw_sprite_ary(lb_ship_order_frame, &pos);
    switch (work->ship_0x02E) {
    default:
        tint_id = 0x139C;
        break;
    case 1:
        tint_id = 0x13AB;
        break;
    case 2:
        tint_id = 0x13BB;
        break;
    }
    tint = get_lsp_data(tint_id, NULL);
    spr_data_copy(&spr, get_lsp_data(0x1329, NULL));
    spr.color = tint->color;
    draw_sprite(spr, &pos);
    if (trade->orders_0x04[work->ship_0x02E].item_0x00 != 0 && trade->orders_0x04[work->ship_0x02E].voyages_0x04 == 0) {
        draw_sprite_idx(0x132A, &pos);
    }
    draw_font_idx(0x1324, lb_speaker_name_get(work->ship_0x02E + 0x14), 4, &pos);
    if (work->field_0x030 == 0 && lb_ship_menu_active_ck(work) == 1) {
        spr_data_copy(&spr, get_lsp_data(0x1325, NULL));
        spr.color = GetMenuFontColor(1, 1, 1, 0);
        draw_font(spr, (s8*)ItemName(order->item_0x00), 5, &pos);
    } else {
        draw_font_idx(0x1325, (s8*)ItemName(order->item_0x00), 1, &pos);
    }
    if (order->item_0x00 != 0) {
        s16 table = lb_ship_goods_table_get(order->item_0x00);

        draw_font_idx(0x1326, (s8*)LbStr(0, table + 0x179), 2, &pos);
        draw_sprite_idx(0x1320, &pos);
        draw_sprite_idx(lb_ship_grade_icons_info[trade->grade_0x00[table] - 1], &pos);
    }
    if (order->voyages_0x04 != 0) {
        sprintf(text, lb_ship_count_fmt, order->voyages_0x04);
        if (work->field_0x030 == 2 && lb_ship_menu_active_ck(work) == 1) {
            spr_data_copy(&spr, get_lsp_data(0x1327, NULL));
            spr.color = GetMenuFontColor(1, 1, 1, 0);
            draw_font(spr, (s8*)text, 5, &pos);
        } else {
            draw_font_idx(0x1327, (s8*)text, 1, &pos);
        }
    }
    if (work->field_0x030 == 1 && lb_ship_menu_active_ck(work) == 1) {
        spr_data_copy(&spr, get_lsp_data(0x1328, NULL));
        spr.color = GetMenuFontColor(1, 1, 1, 0);
        draw_font(spr, (s8*)ItemName(order->bonus_0x02), 5, &pos);
    } else {
        draw_font_idx(0x1328, (s8*)ItemName(order->bonus_0x02), 1, &pos);
    }
}

/* 0x80221BBC (0x6C): Draws the trade header strip and the sail icon. */
void lb_ship_header_draw(LbShipWork* work)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x131B, &pos);
    draw_sprite_ary(lb_ship_header_frame, &pos);
    lb_ship_sprite_draw(0x13C9);
    lb_ship_sprite_draw(0x13CD);
    lb_ship_sprite_draw(0x13CE);
    draw_sprite_anim_idx(0x132B, work->sail_anim_0x02C, &pos);
}

/* 0x80221C28 (0xF0): Draws the selected ship's slot frame, busy or free, and the order prompt on a free ship. */
void lb_ship_order_frame_draw(LbShipWork* work, u8 prompt)
{
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;
    LbShipOrder* order = &trade->orders_0x04[work->ship_0x02E];
    _mh_ivec2_ pos;

    get_lsp_data(0x1339, &pos);
    draw_sprite_ary(lb_ship_slot_frame, &pos);
    if (order->item_0x00 != 0) {
        draw_sprite_ary(lb_ship_slot_busy, &pos);
    } else {
        draw_sprite_ary(lb_ship_slot_free, &pos);
    }
    if (chk_pointer() == 0 && prompt != 0 && trade->orders_0x04[work->ship_0x02E].item_0x00 == 0) {
        draw_sprite_ary(lb_ship_order_prompt, &pos);
        get_lsp_data(0x1319, &pos);
        lb_button_prompt_draw(0, &pos);
    }
}

/* 0x80221D18 (0xEC): Draws the field cursor frame of the order page (the confirm blink while confirming). */
void lb_ship_field_cursor_draw(LbShipWork* work)
{
    _mh_ivec2_ pos;

    if ((u32)chk_pointer() == 1) {
        return;
    }
    get_lsp_data(0x1353, &pos);
    if (work->mode_0x001 == 3) {
        draw_sprite_anim_ary(lb_ship_confirm_anim, work->confirm_anim_0x02A % 20, &pos);
        return;
    }
    if (lb_ship_menu_active_ck(work) == 1) {
        if (work->field_0x030 == 0) {
            draw_sprite_ary(lb_ship_field_goods, &pos);
        }
        if (work->field_0x030 == 1) {
            draw_sprite_ary(lb_ship_field_bonus, &pos);
        }
        if (work->field_0x030 == 2) {
            draw_sprite_ary(lb_ship_field_count, &pos);
        }
    }
}

/* 0x80221E04 (0x434): Draws the order being edited: goods, its table, the voyages and their limit, the bonus, the
 * price and the send prompt. */
void lb_ship_order_edit_draw(LbShipWork* work)
{
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;
    _mh_ivec2_ pos;
    _mh_ivec2_ cursor;
    char text[16];
    _SPR_DATA_ spr;
    u8 prompt;

    get_lsp_data(0x1353, &pos);
    draw_sprite_ary(lb_ship_edit_frame, &pos);
    draw_font_idx(0x135A, (s8*)ItemName(work->order_0x04C.item_0x00), 5, &pos);
    if (work->field_0x030 == 0 && work->mode_0x001 == 1) {
        get_lsp_data(0x1348, &cursor);
        put_menu_cursor(lb_ship_cursor_rows, 0, &cursor);
    }
    if (work->order_0x04C.item_0x00 != 0) {
        s16 table = lb_ship_goods_table_get(work->order_0x04C.item_0x00);

        draw_font_idx(0x1359, (s8*)LbStr(0, table + 0x179), 2, &pos);
        draw_sprite_idx(0x1355, &pos);
        draw_sprite_idx(lb_ship_grade_icons_edit[trade->grade_0x00[table] - 1], &pos);
    }
    sprintf(text, lb_ship_count_fmt, work->order_0x04C.voyages_0x04);
    spr_data_copy(&spr, get_lsp_data(0x135B, NULL));
    if (work->order_0x04C.voyages_0x04 > work->affordable_0x040) {
        if (work->field_0x030 == 2) {
            spr.color = 0xF53737FF;
        } else {
            spr.color = 0xDC2323FF;
        }
    }
    draw_font(spr, (s8*)text, 5, &pos);
    if (work->field_0x030 == 2 && chk_pointer() == 0 && work->mode_0x001 == 1) {
        get_lsp_data(0x134E, &cursor);
        put_menu_cursor(lb_ship_count_cursor_rows, 0, &cursor);
        if (work->order_0x04C.voyages_0x04 > 1) {
            spr_data_copy(&spr, get_lsp_data(0x1361, NULL));
            if (lb_cmd_repeat_ck(4) != 0 && work->mode_0x001 == 1) {
                spr.color = 0x7BCA60FF;
            }
            draw_sprite(spr, &pos);
        }
        if (work->order_0x04C.voyages_0x04 < work->count_max_0x03C) {
            spr_data_copy(&spr, get_lsp_data(0x1362, NULL));
            if (lb_cmd_repeat_ck(8) != 0 && work->mode_0x001 == 1) {
                spr.color = 0x7BCA60FF;
            }
            draw_sprite(spr, &pos);
        }
    }
    sprintf(text, (char*)LbStr(0, 0x17E), work->count_max_0x03C);
    draw_font_idx(0x135D, (s8*)text, 0, &pos);
    draw_font_idx(0x135E, (s8*)ItemName(work->order_0x04C.bonus_0x02), 5, &pos);
    if (work->field_0x030 == 1 && work->mode_0x001 == 1) {
        get_lsp_data(0x134D, &cursor);
        put_menu_cursor(lb_ship_cursor_rows, 0, &cursor);
    }
    draw_font_idx(0x135C, (s8*)LbStr(0, 0x17F), 0, &pos);
    sprintf(text, lb_ship_count_fmt, work->price_0x046 * work->order_0x04C.voyages_0x04);
    spr_data_copy(&spr, get_lsp_data(0x135F, NULL));
    if (work->order_0x04C.voyages_0x04 > work->affordable_0x040) {
        spr.color = 0xDC2323FF;
    }
    draw_font(spr, (s8*)text, 2, &pos);
    spr_data_copy(&spr, get_lsp_data(0x1360, NULL));
    if (work->order_0x04C.voyages_0x04 > work->affordable_0x040) {
        spr.color = 0xDC2323FF;
    }
    draw_font(spr, (s8*)LbStr(1, 0x2C), 0, &pos);
    if (work->field_0x030 == 3 && work->mode_0x001 == 1) {
        prompt = 1;
    } else {
        prompt = work->field_off_0x024[3] == 1 ? 2 : 0;
    }
    get_lsp_data(0x131A, &pos);
    lb_cursor_prompt_draw(prompt, (s16*)&pos);
}

/* 0x80222238 (0x88): Draws the order panel: the free-ship frame on the ship row, else the order being edited. */
void lb_ship_order_panel_draw(LbShipWork* work)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x138E, &pos);
    draw_sprite_ary(lb_ship_panel_frame, &pos);
    if (work->mode_0x001 == 0) {
        draw_sprite_ary(lb_ship_panel_closed, &pos);
        lb_ship_order_frame_draw(work, 1);
        return;
    }
    draw_sprite_ary(lb_ship_panel_open, &pos);
    lb_ship_order_edit_draw(work);
}

/* 0x802222C0 (0x1DC): Draws the three ship tabs in the selected ship's order, the selected tab's cursor and the
 * ship arrows. */
void lb_ship_tabs_draw(LbShipWork* work)
{
    Q_UserData* ud = lb_userdata();
    u16 dim;
    s32 i;
    _mh_ivec2_ pos;

    if (work->step_0x000 == 3 && work->mode_0x001 == 2) {
        dim = 6;
    } else {
        dim = 0;
    }
    for (i = 0; i < 3; i++) {
        s32 ship = lb_ship_tab_order[work->ship_0x02E][i];

        if (ship < ud->ship_0x4654.ships_0x1C) {
            u16 anim;
            u16 frame;
            LbShipTab* tab;

            if (ship != work->ship_0x02E) {
                anim = 6;
                frame = 0;
            } else {
                anim = dim;
                frame = (u32)lobby_w.counter_0x030 % 40;
            }
            tab = &lb_ship_tabs[ship];
            get_lsp_data(tab->layout_0x00, &pos);
            draw_sprite_anim_ary(tab->sprites_0x08, anim, &pos);
            if (chk_pointer() == 0) {
                if (ship == work->ship_0x02E) {
                    draw_sprite_anim_idx(tab->cursor_anim_0x02, frame, &pos);
                }
                if (ud->ship_0x4654.ships_0x1C > 1 && ship == work->ship_0x02E &&
                    (work->mode_0x001 == 0 || work->step_0x000 == 4)) {
                    draw_sprite_ary(tab->arrows_0x0C, &pos);
                    if (lb_cmd_repeat_ck(4) != 0) {
                        draw_sprite_anim_idx(tab->arrows_0x0C[2], 1, &pos);
                    } else if (lb_cmd_repeat_ck(8) != 0) {
                        draw_sprite_anim_idx(tab->arrows_0x0C[3], 1, &pos);
                    }
                }
            }
            draw_sprite_anim_idx(tab->tab_anim_0x04, anim, &pos);
        }
    }
    if (work->mode_0x001 != 0 && work->step_0x000 != 4) {
        lb_ship_sprite_draw(0x13CF);
    }
}

/* 0x8022249C (0x1C0): Draws a sailing ship's order card on the view page. */
void lb_ship_order_view_draw(LbShipWork* work)
{
    LbShipBlock* trade = &lb_userdata()->ship_0x4654;
    LbShipOrder* order = &trade->orders_0x04[work->ship_0x02E];
    _mh_ivec2_ pos;
    char text[16];

    get_lsp_data(0x13F7, &pos);
    draw_sprite_ary(lb_ship_order_frame, &pos);
    if (trade->orders_0x04[work->ship_0x02E].item_0x00 != 0 && trade->orders_0x04[work->ship_0x02E].voyages_0x04 == 0) {
        draw_sprite_idx(0x1405, &pos);
    }
    draw_font_idx(0x1400, lb_speaker_name_get(work->ship_0x02E + 0x14), 4, &pos);
    draw_font_idx(0x1401, (s8*)ItemName(order->item_0x00), 1, &pos);
    if (order->item_0x00 != 0) {
        s16 table = lb_ship_goods_table_get(order->item_0x00);

        draw_font_idx(0x1402, (s8*)LbStr(0, table + 0x179), 2, &pos);
        draw_sprite_idx(0x13FC, &pos);
        draw_sprite_idx(lb_ship_grade_icons_view[trade->grade_0x00[table] - 1], &pos);
    }
    if (order->voyages_0x04 != 0) {
        sprintf(text, lb_ship_count_fmt, order->voyages_0x04);
        draw_font_idx(0x1403, (s8*)text, 1, &pos);
    }
    draw_font_idx(0x1404, (s8*)ItemName(order->bonus_0x02), 1, &pos);
    if (work->sail_anim_0x02C != 0) {
        draw_sprite_idx(0x1406, &pos);
    }
    lb_ship_sprite_draw(0x1436);
    lb_ship_sprite_draw(0x1437);
    lb_ship_sprite_draw(0x1438);
}

/* 0x8022265C (0xB8): Draws the goods tables' grades. */
void lb_ship_levels_draw(LbShipWork* work)
{
    LbShipBlock* ship = &lb_userdata()->ship_0x4654;
    _mh_ivec2_ pos;
    s32 i;
    u16* layout;

    get_lsp_data(0x1422, &pos);
    draw_sprite_ary(lb_ship_levels_frame, &pos);
    i = 0;
    layout = lb_ship_level_layouts;
    for (; i < 4; i++) {
        if (ship->grade_0x00[i] != 0) {
            get_lsp_data(*layout, &pos);
            draw_sprite_anim_ary(lb_ship_level_rows, i, &pos);
            draw_sprite_anim_idx(lb_ship_level_icons[ship->grade_0x00[i] - 1], i, &pos);
            layout++;
        }
    }
}

/* 0x80222714 (0xA8): Draws the ship view page. */
void lb_ship_view_draw(LbShipWork* work)
{
    LbShipOrder* order = &lb_userdata()->ship_0x4654.orders_0x04[work->ship_0x02E];
    _mh_ivec2_ pos;

    lb_ship_tabs_draw(work);
    if (order->item_0x00 != 0) {
        lb_ship_order_view_draw(work);
    } else {
        get_lsp_data(0x138E, &pos);
        draw_sprite_ary(lb_ship_panel_frame, &pos);
        draw_sprite_ary(lb_ship_panel_closed, &pos);
        lb_ship_order_frame_draw(work, 0);
    }
    lb_ship_levels_draw(work);
    lb_panel_msg_draw(0x1877, 0x133);
}

/* 0x802227BC (0x8C): Draws the screen's backdrop (4:3 or wide). */
void lb_ship_backdrop_draw(void)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x129C, &pos);
        draw_sprite_ary(lb_ship_backdrop_wide, &pos);
    } else {
        get_lsp_data(0x1297, &pos);
        draw_sprite_ary(lb_ship_backdrop_4x3, &pos);
    }
    draw_sprite_idx(0x1290, &pos);
    get_lsp_data(0x1286, &pos);
    draw_sprite_ary(lb_ship_backdrop, &pos);
}

/* 0x80222848 (0xC0): The screen position of hold cell `cell` (ten to a row). */
void lb_ship_cell_pos_get(s32 cell, _mh_ivec2_* out)
{
    _SPR_DATA_* column = get_lsp_data(lb_ship_cell_columns[cell % 10], NULL);
    _SPR_DATA_* row = get_lsp_data(lb_ship_cell_rows[cell / 10], NULL);

    out->x = column->pos.x + 4;
    out->y = row->pos.y;
}

/* 0x80222908 (0x2BC): Draws ship `ship`'s hold page of the item box: its frame, the thirty cells and their items, the
 * pointed cell and the picked cell. */
void lb_ship_box_grid_draw(Eft052ItemBox* box, u8 ship)
{
    _SPR_DATA_ cell;
    _SPR_DATA_ icon;
    _SPR_DATA_ mark;
    _mh_ivec2_ pos;
    _mh_ivec2_ cell_pos;
    _mh_ivec2_ base;
    _mh_ivec2_ picked;
    Eft052BoxCell* cells;
    s32 i;
    s32 first;
    s32 found;

    get_lsp_data(lb_ship_box_layouts[ship], &pos);
    draw_sprite_anim_ary(lb_ship_box_frames, ship, &pos);
    found = 0;
    first = ship * 30;
    spr_data_copy(&icon, get_lsp_data(0x1307, NULL));
    uv_pair_copy(&base, &icon);
    if (box->state_0x00 == 4) {
        cells = box->sorted_0x28;
    } else {
        cells = box->cells_0x24;
    }
    cells += first;
    for (i = 0; i < 30; i++) {
        s32 index;

        lb_ship_cell_pos_get(i, &cell_pos);
        spr_data_copy(&cell, get_lsp_data(lb_ship_cell_sprites[ship], NULL));
        uv_pair_copy(&cell, &cell_pos);
        icon.pos.x = cell_pos.x + base.x;
        icon.pos.y = cell_pos.y + base.y;
        draw_sprite(cell, &pos);
        index = first + i;
        if (box->cursor_0x0E == index) {
            found = 1;
            uv_pair_copy(&picked, &cell_pos);
        }
        if ((u8)(box->state_0x00 + 0xFF) <= 1 && chk_pointer() == 0 && box->hover_0x10 == index) {
            spr_data_copy(&mark, get_lsp_data(0x1304, NULL));
            mark.pos.x += cell_pos.x;
            mark.pos.y += cell_pos.y;
            set_blendmode(4, 1, 1);
            spr_anim_draw(&mark, box->frame_0x08, &pos);
            set_blendmode(4, 5, 1);
        }
        if (cells->item_0x00 != 0) {
            draw_itemicon_item_id(icon, cells->item_0x00, &pos);
        }
        cells++;
    }
    if (box->picked_0x0C != 0 && found != 0) {
        set_blendmode(4, 1, 1);
        spr_data_copy(&mark, get_lsp_data(0x1305, NULL));
        mark.pos.x += picked.x;
        mark.pos.y += picked.y;
        spr_anim_draw(&mark, box->frame_0x0A, &pos);
        set_blendmode(4, 5, 1);
        spr_data_copy(&mark, get_lsp_data(0x1306, NULL));
        mark.pos.x += picked.x;
        mark.pos.y += picked.y;
        spr_anim_draw(&mark, box->frame_0x0A, &pos);
    }
}

/* 0x80222BC4 (0x94): Draws the count of the item box's pointed cell at layout `panel`, frame `frame`. */
void lb_item_box_count_draw(Eft052ItemBox* box, u16 panel, u8 frame)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u16 item = box->cells_0x24[box->hover_0x10].item_0x00;

    sprite_frame_apply(&spr, panel, frame, &pos);
    if (box->state_0x00 != 1 && box->state_0x00 != 2) {
        item = 0;
    }
    lb_item_info_draw(item, box->cells_0x24[box->hover_0x10].count_0x02, 99, 2, &pos);
}

/* 0x80222C58 (0x140): Draws the holds page: the backing for the ship count, each ship's hold and captain, the
 * pointed cell's count and the button guide. */
void lb_ship_box_draw(Eft052ItemBox* box)
{
    Q_UserData* ud = lb_userdata();
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u16* name;
    s32 i;
    u8 frame;

    frame = ud->ship_0x4654.ships_0x1C - 1;
    get_lsp_data(0x12B3, &pos);
    draw_sprite_anim_ary(lb_ship_box_backs, frame, &pos);
    name = lb_ship_box_names;
    for (i = 0; i < ud->ship_0x4654.ships_0x1C; i++) {
        lb_ship_box_grid_draw(box, i);
        draw_font_anim_idx(*name, frame, lb_speaker_name_get(i + 0x14), 5, &pos);
        name++;
    }
    lb_item_box_count_draw(box, 0x12E5, frame);
    if ((u8)(box->state_0x00 + 0xFF) <= 1 && chk_pointer() == 0 && box->total_0x1C != 0) {
        sprite_frame_apply(&spr, 0x1308, ud->ship_0x4654.ships_0x1C - 1, &pos);
        if (get_option_cfg(7) == 0) {
            put_button_guide(4, &pos);
        } else {
            put_button_guide(0xC, &pos);
        }
    }
}

/* 0x80222D98 (0x44): Draws the holds page of the shared item box and its option list. */
void lb_ship_box_page_draw(void)
{
    lb_ship_box_draw(&eft052_item_box);
    eft052_box_list_draw(0x130A, lb_ship_menu_sprites, lb_ship_box_list_rows, 0x12B2, 0);
}

/* 0x80222DDC (0x13C): The trade-ship screen's draw callback. */
void lb_box_screen_draw(void)
{
    LbShipWork* work = lb_ship_work();

    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    if (work->step_0x000 != 0 && work->step_0x000 != 5) {
        lb_ship_backdrop_draw();
        if (work->step_0x000 == 2) {
            menu_money_draw(0x14FC);
        } else {
            lb_points_draw(0x12A1);
        }
    }
    font_set_size(0x12, 0x12);
    switch (work->step_0x000) {
    case 1:
        lb_choice_draw(&work->menu_0x004, 0x12A3, lb_ship_menu_sprites, lb_ship_menu_rows, 0x1877, 0);
        return;
    case 2:
        lb_ship_box_page_draw();
        return;
    case 4:
        lb_ship_view_draw(work);
        return;
    case 3:
        if ((s32)work->mode_0x001 == 2) {
            eft052_hold_draw(0);
        } else {
            lb_ship_tabs_draw(work);
            lb_ship_header_draw(work);
            lb_ship_field_cursor_draw(work);
            lb_ship_order_info_draw(work);
            lb_ship_order_panel_draw(work);
        }
        lb_ship_help_draw(work);
        return;
    }
}

}  /* extern "C" */
#pragma peephole reset

/* The player actor's per-model SE/motion rig update (0x80224AC4-0x80229ECC). */
#pragma peephole off
/* Retail keeps every `fmuls`+`fadds` pair of the rig update unfused. */
#pragma fp_contract off

#define MHTRI_SPLIT_SCREEN_SIZE_CLASS_TAKES_ACTOR 1

#include "types.h"
#include "pl.h"
#include "Pl/Pl_master_ck.h"
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "enemy/fn_80138074.h"
#include "sound/fn_800DD1F0.h"
#include "sound/sound_job.h" /* split_screen_size_class (rule 2) */
#include "sound/fn_800D7F54.h"
#include "sound/se_w.h"
#include "unsplit/Pl.h"
#include "Pl/fn_80258FCC.h"
#include "unsplit/g3d.h"
#include "g3d/g3d_scnmdlsmpl.h" /* ScnMdlSimple, FuncObjCalcWorld, ICalcWorldCallback (rule 2) */

#include "unsplit/unknown.h"
#include "sys_mem.h"
#include "Pl/pl_se_rig.h"
#include "Pl/fn_802693C4.h"

#include "MSL_C/alloc.h"                    /* memcmp */
#include "MSL/strlen.h"
#include "draw_shape.h"                      /* res_file_ctor, res_file_assign */
#include "color_rgba_copy.h"
#include "mh3_pad.h"                         /* copyVec3 */
#include "sound/mhchar.h"                    /* mhchar_reset, mhchar_node_count */
#include "sound/mhchar_model_create.h"
#include "sound/mhchar_draw_mode_set.h"
#include "sound/mhchar_mat_num_get.h"
#include "sound/mhchar_mdl_prepare.h"
#include "Pl/fn_8027D684.h"                  /* EnableChangeColor, equip_color_index_get, equip_color_rgba_get */
#include "Pl/pl_act.h"                       /* equip_custom_color_rgba_get */
#include "Pl/pl_item_page2_ck.h"
#include "Pl/equip_blade_dye_ck.h"
#include "Pl/pl_equip_file_get.h"
#include "Pl/pl_weapon_alt_form_ck.h"
#include "Pl/armor_color_rgba_get.h"
#include "g3d/fn_8005AA28.h"                 /* res_node_is_valid */
#include "g3d/g3d_anmchr.h"                  /* res_node_get_id */
#include "g3d/res_node_assign.h"
#include "g3d/res_node_mtx_id_get.h"
#include "g3d/res_file_handle_copy.h"
#include "g3d/g3d_resnode.h"                 /* nw4r::g3d::ResNode */
#include "enemy/em007_res_node_name.h"
#include "lobby/lb_player_screen_ok_ck.h"
#include "lobby/res_mat_name_get.h"
#include "unsplit/pl_rig_style.h"
#include "vec3_float_copy.h"
#include "ef/system_lobby_flag_get.h"
#include "Pl/equip_variant_get.h"
#include "Pl/pl_charge_cycle_get.h"
#include "Pl/fn_802840DC.h"                   /* pl_charge_stage2_get */
#include "Pl/pl_yure.h"                      /* YureRec, YurePlayerWork, yure_angles_calc */
#include "enemy/joint_mtx_store.h"
#include "g3d/g3d_calcview.h"                /* mtx34_concat */
#include "g3d/world_mtx_attr_scale_set.h"
#include "g3d/anm_chr_result_scale_get.h"
#include "g3d/anm_chr_result_rot_get.h"
#include "sound/mhchar_wld_mtx_array_get.h"

/* One hair-style row (`pl_rig_hair_style_tbls`): the hair parts shown, the style's kind and the face parts shown.
 * size: 0x8 */
struct PlRigHairStyle {
    /* +0x00 */ u32 parts_0x00;
    /* +0x04 */ u8 kind_0x04;
    /* +0x05 */ u8 face_0x05;
    /* +0x06 */ u8 pad_0x06[2];
};

/* One face-style row (`pl_rig_face_style_tbls`): the face parts shown and the hood flag that hides the brow.
 * size: 0x2 */
struct PlRigFaceStyle {
    /* +0x00 */ u8 parts_0x00;
    /* +0x01 */ u8 hood_0x01;
};

/* One node rule of an attachment kind (`pl_rig_node_rules`): how the callbacks treat the node (1 adjust its animation
 * result, 2 rebuild its matrix from a body joint), its flags (bit 1 keeps the node walk on this node) and the body
 * joint (0xFFFF: the attachment's own).  size: 0x4 */
typedef struct PlRigNodeRule {
    /* +0x00 */ u8 mode_0x00;
    /* +0x01 */ u8 flags_0x01;
    /* +0x02 */ u16 joint_0x02;
} PlRigNodeRule; /* size: 0x4 */

/* One key of a guard-motion key table (`pl_rig_keys_*`): its motion frame (-1.0f ends the table), the root rotation
 * and translation.  size: 0x1C */
typedef struct PlRigKey {
    /* +0x00 */ f32 frame_0x00;
    /* +0x04 */ nw4r::math::VEC3 rot_0x04;
    /* +0x10 */ nw4r::math::VEC3 pos_0x10;
} PlRigKey;

/* One key of a scale key table: its motion frame (-1.0f ends the table), the uniform scale, the lift and the two
 * tilts.  size: 0x14 */
typedef struct PlRigScaleKey {
    /* +0x00 */ f32 frame_0x00;
    /* +0x04 */ f32 scale_0x04;
    /* +0x08 */ f32 lift_0x08;
    /* +0x0C */ f32 tilt_x_0x0C;
    /* +0x10 */ f32 tilt_z_0x10;
} PlRigScaleKey;

/* One key of a tilt key table: its motion frame (-1.0f ends the table), the lift and the two tilts.  size: 0x10 */
typedef struct PlRigTiltKey {
    /* +0x00 */ f32 frame_0x00;
    /* +0x04 */ f32 lift_0x04;
    /* +0x08 */ f32 tilt_x_0x08;
    /* +0x0C */ f32 tilt_z_0x0C;
} PlRigTiltKey;

/* This unit's own functions, declared in address order so an earlier body can call a later one.  All
 * of them are `extern "C"` because the map spells them unmangled (playbook 42). */
extern "C" {
u8 pl_slot_motion_allow_get(_PLW* self, u32 slot);
f32 pl_rig_bank_gain_get(_EQUIP* equip);
f32 pl_rig_key_lerp(_PLW* plw, PlRigKey* keys, PlSeVecPair* out);
PlSeVecPair* pl_rig_guard_pose_get(_PLW* plw, u16 motion, u8 kind, PlSeVecPair* out);
f32 pl_rig_attach_pose_set(PlSeRig* rig, u32 index, u32 kind);
u8 pl_rig_kind0_allow_get(_PLW* self, u16 motion);
u8 pl_rig_kind2_allow_get(_PLW* self, u16 motion);
u8 pl_rig_kind1_allow_get(_PLW* self, u16 motion);
u8 pl_rig_kind3_allow_get(_PLW* self, u16 motion);
u8 pl_rig_kind4_allow_get(_PLW* self, u16 motion);
u8 pl_rig_kind7_allow_get(_PLW* self, u16 motion);
u8 pl_rig_kind8_allow_get(_PLW* self, u16 motion);
PlSeVecPair* pl_rig_lobby_pose_get(_PLW* plw, s32 kind, u8 mode);
PlSeVecPair* pl_rig_pose_get(_PLW* plw, s32 kind, u8 allow);
void pl_rig_attach_motion_pose(PlSeRig* rig, PlSeAttach* attach, u32 kind);
f32 pl_rig_key_get(_PLW* self, f32* keys);
f32 pl_rig_key_pair_get(_PLW* self, f32* keys, f32* a, f32* b);
void pl_rig_key_triple_get(_PLW* self, f32* keys, f32* a, f32* b, f32* c);
void pl_rig_kind4_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion);
void pl_rig_key_scale_lerp(_PLW* plw, PlRigScaleKey* keys, PlSeJointAdj* adj);
void pl_rig_key_tilt_lerp(_PLW* plw, PlRigTiltKey* keys, PlSeJointAdj* adj);
void pl_rig_kind0_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion);
void pl_rig_kind1_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion);
void pl_rig_kind9_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion);
void pl_rig_kind7_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion);
void pl_rig_kind8_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion);
void pl_rig_kind3_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion);
void pl_rig_charge_spin_set(_PLW* plw, PlSeJointAdj* adj, u8 stage);
void pl_rig_kind2_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion);

/* The rig update's `.data`/`.sdata` tables. */
extern u8 pl_rig_bank_kind_tbl[];
extern const f32 pl_rig_bank_gain_tbl[2];
extern PlSeVecPair pl_rig_pose_sheathed[];
extern PlSeVecPair pl_rig_pose_drawn[];
extern PlSeVecPair pl_rig_pose_alt_drawn[];
extern PlSeVecPair pl_rig_pose_carried[];
extern PlSeVecPair pl_rig_pose_alt[];
extern PlSeVecPair pl_rig_pose_special[];
extern PlSeVecPair pl_rig_pose_recital[];
extern PlSeVecPair pl_rig_pose_lobby[];
extern PlSeVecPair pl_rig_pose_lobby_one[];
extern PlRigKey pl_rig_keys_40b[];
extern PlRigKey pl_rig_keys_40c[];
extern PlRigKey pl_rig_keys_410[];
extern PlRigKey pl_rig_keys_40d[];
extern PlRigKey pl_rig_keys_46c[];
extern PlRigKey pl_rig_keys_40e[];
extern PlRigKey pl_rig_keys_43a[];
extern PlRigKey pl_rig_keys_43b[];
extern PlRigKey pl_rig_keys_43c[];
extern PlRigKey pl_rig_keys_43d[];
extern PlRigKey pl_rig_keys_49d[];
extern PlRigKey pl_rig_keys_49e[];
extern PlRigScaleKey pl_rig_k8_139_grip[];
extern PlRigTiltKey pl_rig_k8_139_head[];
extern f32 pl_rig_k8_139_depth[];
extern f32 pl_rig_k8_139_lift[];
extern PlRigScaleKey pl_rig_k8_3ea_grip[];
extern PlRigTiltKey pl_rig_k8_3ea_head[];
extern f32 pl_rig_k8_3ea_lift[];
extern PlRigScaleKey pl_rig_k8_3eb_grip[];
extern PlRigTiltKey pl_rig_k8_3eb_head[];
extern f32 pl_rig_k8_3eb_depth[];
extern f32 pl_rig_k8_3eb_lift[];
extern PlRigScaleKey pl_rig_k8_3eb_grip_pair[];
extern PlRigTiltKey pl_rig_k8_3eb_head_pair[];
extern PlRigScaleKey pl_rig_k8_458_grip[];
extern PlRigTiltKey pl_rig_k8_458_head[];
extern f32 pl_rig_k8_458_lift[];
extern PlRigScaleKey pl_rig_k8_462_grip[];
extern PlRigScaleKey pl_rig_k8_464_grip[];
extern PlRigScaleKey pl_rig_k8_465_grip[];
extern PlRigTiltKey pl_rig_k8_465_head[];
extern PlRigScaleKey pl_rig_k8_474_grip[];
extern PlRigTiltKey pl_rig_k8_474_head[];
extern PlRigScaleKey pl_rig_k8_475_grip[];
extern PlRigTiltKey pl_rig_k8_475_head[];
extern PlRigScaleKey pl_rig_k8_476_grip[];
extern PlRigTiltKey pl_rig_k8_476_head[];
extern PlRigScaleKey pl_rig_k8_477_grip[];
extern PlRigTiltKey pl_rig_k8_477_head[];
extern u8 pl_rig_sway_ranges[][2];
extern PlRigNodeRule* pl_rig_node_rules[];
extern f32 pl_rig_k0_139_roll[];
extern f32 pl_rig_k0_139_scale[];
extern f32 pl_rig_k0_3ea_roll[];
extern f32 pl_rig_k0_3ea_scale[];
extern f32 pl_rig_k0_41b_scale[];
extern f32 pl_rig_k0_450_roll[];
extern f32 pl_rig_k0_450_scale[];
extern f32 pl_rig_k0_482_scale[];
extern f32 pl_rig_k0_3eb_roll[];
extern f32 pl_rig_k0_3eb_scale[];
extern f32 pl_rig_k0_3f1_roll[];
extern f32 pl_rig_k0_3f1_scale[];
extern f32 pl_rig_k0_41c_roll[];
extern f32 pl_rig_k0_41c_scale[];
extern f32 pl_rig_k0_421_roll[];
extern f32 pl_rig_k0_421_scale[];
extern f32 pl_rig_k2_450_scale[];
extern f32 pl_rig_k2_450_lift[];
extern f32 pl_rig_k2_452_scale[];
extern f32 pl_rig_k2_452_lift[];
extern f32 pl_rig_k2_457_scale[];
extern f32 pl_rig_k2_457_lift[];
extern f32 pl_rig_k2_470_scale[];
extern f32 pl_rig_k2_470_lift[];
extern f32 pl_rig_k2_481_scale[];
extern f32 pl_rig_k2_481_lift[];
extern f32 pl_rig_k2_487_scale[];
extern f32 pl_rig_k2_487_lift[];
extern f32 pl_rig_k2_48b_scale[];
extern f32 pl_rig_k2_48b_lift[];
extern f32 pl_rig_k2_48c_scale[];
extern f32 pl_rig_k2_48c_lift[];
extern f32 pl_rig_k3_3ea_scale[];
extern f32 pl_rig_k3_41b_scale[];
extern f32 pl_rig_k3_3f2_scale[];
extern f32 pl_rig_k3_3eb_scale[];
extern f32 pl_rig_k3_3f1_scale[];
extern f32 pl_rig_k3_3fa_scale[];
extern f32 pl_rig_k3_49c_scale[];
extern f32 pl_rig_k7_139_scale[];
extern f32 pl_rig_k7_139_span[];
extern f32 pl_rig_k7_3ea_scale[];
extern f32 pl_rig_k7_3ea_span[];
extern f32 pl_rig_k7_3eb_scale[];
extern f32 pl_rig_k7_3eb_span[];
extern f32 pl_rig_k7_3f1_scale[];
extern f32 pl_rig_k7_3f1_span[];
extern f32 pl_rig_k7_45b_scale[];
extern f32 pl_rig_k7_45b_span[];
extern f32 pl_rig_k9_3ea_scale[];
extern f32 pl_rig_k9_3fa_scale[];
extern f32 pl_rig_k9_46a_scale[];
extern f32 pl_rig_k9_41b_scale[];
extern f32 pl_rig_k9_42e_scale[];
extern f32 pl_rig_k9_3eb_scale[];
extern f32 pl_rig_k9_41c_scale[];
extern f32 pl_rig_k1_3ea_scale[];
extern f32 pl_rig_k1_3eb_scale[];
extern f32 pl_rig_k1_41c_scale[];
extern f32 pl_rig_k1_46a_scale[];
extern f32 pl_rig_k4_3ea_spin[];
extern f32 pl_rig_k4_3ea_scale[];
extern f32 pl_rig_k4_3ea_scale_alt[];
extern f32 pl_rig_k4_40b_scale[];
extern f32 pl_rig_k4_40b_scale_alt[];
extern f32 pl_rig_k4_44d_scale_alt[];
extern f32 pl_rig_k4_44d_scale[];
extern f32 pl_rig_k4_450_scale[];
extern f32 pl_rig_k4_3eb_spin[];
extern f32 pl_rig_k4_3eb_scale[];
extern f32 pl_rig_k4_3f1_spin[];
extern f32 pl_rig_k4_3f1_scale[];
extern f32 pl_rig_k4_40f_spin[];
extern f32 pl_rig_k4_40f_scale[];
extern f32 pl_rig_k4_451_scale_alt[];
extern f32 pl_rig_k4_451_scale[];
}

/* The rig's own `.data`/`.sdata` tables this range reads. */
extern "C" {
extern u8 pl_rig_attach_kind_tbl[];
extern const char* pl_rig_hair_part_names[9];
extern const char* pl_rig_face_part_names[5];
extern u32 pl_rig_model_flags[7];
extern char pl_rig_mat_sym[4];
extern char pl_rig_mat_inn[4];
extern char pl_rig_mat_hair[5];
extern char pl_rig_node_cl[3];
}

/* The rig's callback objects as their classes (the rig's arrays hold them by value). */
static inline LbEquipModelCallbackB* pl_rig_slot_cb(PlSeSlot* slot)
{
    return (LbEquipModelCallbackB*)slot;
}

static inline LbEquipModelCallbackA* pl_rig_attach_cb(PlSeAttach* attach)
{
    return (LbEquipModelCallbackA*)attach;
}

/* A rig model's resource model and scene model (`MHchar` +0x114/+0x118). */
static inline nw4r::g3d::ResMdl* pl_rig_res_mdl(MHchar* model)
{
    return (nw4r::g3d::ResMdl*)&model->field_0x114;
}

static inline nw4r::g3d::ScnMdlSimple* pl_rig_scn_mdl(MHchar* model)
{
    return (nw4r::g3d::ScnMdlSimple*)model->field_0x118;
}

extern "C" {

void pl_rig_joint_adj_reset(PlSeRig* rig, PlSeJointAdj* adj);
const void* res_ofs_ptr_get(const u32* base, s32 offset);
void pl_rig_model_color_index_set(PlSeRig* rig, u16 index, u32 slot);
void pl_rig_model_color_set(PlSeRig* rig, _GXColor* color, u32 slot);
void pl_rig_model_skin_color_set(PlSeRig* rig, _GXColor* color, u32 slot);
void pl_rig_sub_color_apply(PlSeRig* rig, u32 slot);
void pl_rig_hair_color_set(PlSeRig* rig, _GXColor* color);
void pl_rig_hair_parts_map(PlSeRig* rig);
void pl_rig_face_parts_map(PlSeRig* rig);
void pl_rig_hair_parts_apply(PlSeRig* rig);
void pl_rig_hair_parts_show(PlSeRig* rig, u32 mask);
void pl_rig_face_parts_apply(PlSeRig* rig);
void pl_rig_face_parts_show(PlSeRig* rig, u32 mask);
void pl_rig_slot_bind(PlSeRig* rig, PlSeSlot* slot, u32 index);
void pl_rig_attach_bind(PlSeRig* rig, PlSeAttach* attach, u8 kind, MHchar* model);
void pl_rig_model_place(PlSeRig* rig, MHchar* model, _PLW* plw, f32 scale);
u32 pl_rig_screen_ok_ck(_PLW* plw);

/* 0x80222F18 (0x1B0): Resets every model of the rig and its material and node maps, then binds it to `plw`. */
void pl_physics_rebind(PlSeRig* rig, _PLW* plw)
{
    u32 i;
    u32 j;

    for (i = 0; i < 7; i++) {
        mhchar_reset(&rig->models_0x004[i].model);
    }
    for (i = 0; i < 3; i++) {
        mhchar_reset(&rig->sub_0x0A14[i].model);
        rig->sub_kind_0x9CA[i] = 0xFF;
        rig->sub_mat_0xA01[i] = 0xFF;
    }
    for (i = 0; i < 7; i++) {
        rig->slots_0xE40[i].body_0x08 = 0;
        rig->slots_0xE40[i].index_0x0C = i;
        rig->color_mat_0x9CD[i][0] = 0xFF;
        rig->color_mat_0x9CD[i][1] = 0xFF;
        rig->skin_mat_0x9DB[i] = 0xFF;
    }
    rig->hair_mat_0x9E2 = 0xFF;
    memset(rig->hair_part_0x9E3, 0xFF, sizeof(rig->hair_part_0x9E3));
    memset(rig->face_part_0x9F9, 0xFF, sizeof(rig->face_part_0x9F9));
    for (i = 0; i < 3; i++) {
        PlSeAttach* attach = &rig->attachments_0x1230[i];

        attach->models_0x04 = 0;
        attach->joint_0x0C = 0;
        attach->kind_0x10 = pl_rig_attach_kind_tbl[plw->field_0x002];
        for (j = 0; j < 7; j++) {
            pl_rig_joint_adj_reset(rig, &attach->adj_0x014[j]);
        }
    }
    rig->plw = plw;
}

/* 0x802230C8 (0x30): Resets one joint adjustment to unit scale, no rotation, no translation. */
void pl_rig_joint_adj_reset(PlSeRig* rig, PlSeJointAdj* adj)
{
    adj->scale_0x00.x = 1.0f;
    adj->scale_0x00.y = 1.0f;
    adj->scale_0x00.z = 1.0f;
    adj->trans_0x0C.x = 0.0f;
    adj->trans_0x0C.y = 0.0f;
    adj->trans_0x0C.z = 0.0f;
    adj->rot_0x18.x = 0.0f;
    adj->rot_0x18.y = 0.0f;
    adj->rot_0x18.z = 0.0f;
}

/* 0x802230F8 (0xB4): Resets model `slot` and its callback slot and material maps. */
void pl_rig_model_reset(PlSeRig* rig, u32 slot)
{
    mhchar_reset(&rig->models_0x004[slot].model);
    rig->slots_0xE40[slot].body_0x08 = 0;
    rig->slots_0xE40[slot].index_0x0C = slot;
    rig->color_mat_0x9CD[slot][0] = 0xFF;
    rig->color_mat_0x9CD[slot][1] = 0xFF;
    rig->skin_mat_0x9DB[slot] = 0xFF;
    if (slot == 5) {
        memset(rig->hair_part_0x9E3, 0xFF, sizeof(rig->hair_part_0x9E3));
        rig->hair_mat_0x9E2 = 0xFF;
    } else if (slot == 4) {
        memset(rig->face_part_0x9F9, 0xFF, sizeof(rig->face_part_0x9F9));
    }
}

/* 0x802231AC (0xAC): Resets sub-model `slot` and its attachment. */
void pl_rig_sub_reset(PlSeRig* rig, u32 slot)
{
    PlSeAttach* attach = &rig->attachments_0x1230[slot];
    u32 i;

    mhchar_reset(&rig->sub_0x0A14[slot].model);
    rig->sub_kind_0x9CA[slot] = 0xFF;
    rig->sub_mat_0xA01[slot] = 0xFF;
    attach->models_0x04 = 0;
    attach->joint_0x0C = 0;
    attach->kind_0x10 = pl_rig_attach_kind_tbl[rig->plw->field_0x002];
    for (i = 0; i < 7; i++) {
        pl_rig_joint_adj_reset(rig, &attach->adj_0x014[i]);
    }
}

/* 0x80223258 (0xC0): Dyes model `slot` with its equipment piece's colour: the custom colour, the actor's fixed
 * palette entry, or the piece's own palette entry. */
void pl_equip_color_apply(_PLW* self, u32 slot)
{
    _EQUIP* equip = &self->equipA[slot];

    if (EnableChangeColor(equip) == 1) {
        _GXColor color;
        s32 rgba = equip_custom_color_rgba_get(equip);

        color_rgba_copy((u8*)&color, (u8*)&rgba);
        pl_rig_model_color_set((PlSeRig*)self->physics_0x13C, &color, slot);
        return;
    }
    if (self->field_0x25C[6] != 0) {
        pl_rig_model_color_index_set((PlSeRig*)self->physics_0x13C, self->field_0x25C[6], slot);
        return;
    }
    {
        u8 index = equip_color_index_get(equip);

        pl_rig_model_color_index_set((PlSeRig*)self->physics_0x13C, index, slot);
    }
}

/* 0x80223318 (0x28C): Creates model `slot` from `file` and maps its dyed, skin and hair materials and its parts. */
void pl_rig_model_setup(PlSeRig* rig, u32* file, u32 slot, u32 flags)
{
    MHchar* model;
    nw4r::g3d::ResMdl* mdl;
    u8 light;
    u8 n;
    u8 i;

    if ((u8)PlayMode_ck() != 4) {
        u8 mode = GameMode_ck();

        if (mode == 0 || mode == 3) {
            light = 3;
        } else {
            light = rig->plw->chunk_ofs + 3;
        }
    } else {
        light = 3;
    }
    model = &rig->models_0x004[slot].model;
    mhchar_model_create(model, file, flags, light);
    mhchar_draw_mode_set(model, 2);
    if (slot == 0) {
        if (rig->plw->se_name_set == 0) {
            Pl_chr_set_attr(rig->plw, 0x43, 0, 0, 0);
        } else {
            Pl_chr_set_attr(rig->plw, 0x41, 0, 0, 0);
        }
    }
    rig->color_mat_0x9CD[slot][0] = 0xFF;
    rig->color_mat_0x9CD[slot][1] = 0xFF;
    rig->skin_mat_0x9DB[slot] = 0xFF;
    n = 0;
    mdl = pl_rig_res_mdl(&rig->models_0x004[slot].model);
    for (i = 0; i < mhchar_mat_num_get(model); i++) {
        nw4r::g3d::ResMat mat = mdl->GetResMat(i);

        if (memcmp(res_mat_name_get(&mat), pl_rig_mat_sym, 3) == 0) {
            if (n >= 2) {
                break;
            }
            rig->color_mat_0x9CD[slot][n] = i;
            n++;
        } else {
            nw4r::g3d::ResMat mat2 = mdl->GetResMat(i);

            if (memcmp(res_mat_name_get(&mat2), pl_rig_mat_inn, 3) == 0) {
                rig->skin_mat_0x9DB[slot] = i;
            }
        }
    }
    pl_equip_color_apply(rig->plw, slot);
    if (slot == 5) {
        rig->hair_mat_0x9E2 = 0xFF;
        for (i = 0; i < mhchar_mat_num_get(model); i++) {
            nw4r::g3d::ResMat mat = mdl->GetResMat(i);

            if (memcmp(res_mat_name_get(&mat), pl_rig_mat_hair, 4) == 0) {
                rig->hair_mat_0x9E2 = i;
            }
        }
        mhchar_mdl_prepare(mdl);
        pl_rig_hair_color_set(rig, (_GXColor*)rig->plw->field_0x258);
    }
    switch (slot) {
    case 4:
        pl_rig_face_parts_apply(rig);
        pl_rig_hair_parts_apply(rig);
        break;
    case 5:
        pl_rig_hair_parts_map(rig);
        pl_rig_hair_parts_apply(rig);
        pl_rig_face_parts_apply(rig);
        break;
    case 6:
        pl_rig_face_parts_map(rig);
        pl_rig_face_parts_apply(rig);
        break;
    }
}

/* 0x802235A4 (0x3C): The material's name (nw4r's inline `ResMat::GetName`), NULL when unnamed. */
const char* res_mat_name_get(const nw4r::g3d::ResMat* mat)
{
    return (const char*)res_ofs_ptr_get((const u32*)mat, mat->ref().name);
}

/* 0x802235E0 (0x1C): The address `offset` bytes past the block `*base` points at, NULL for a zero offset. */
const void* res_ofs_ptr_get(const u32* base, s32 offset)
{
    u32 addr = *base;

    if (offset == 0) {
        return 0;
    }
    return (const u8*)addr + offset;
}

/* 0x802235FC (0x5C): Dyes model `slot` with palette entry `index`. */
void pl_rig_model_color_index_set(PlSeRig* rig, u16 index, u32 slot)
{
    _GXColor color;
    s32 rgba = equip_color_rgba_get(index);

    color_rgba_copy((u8*)&color, (u8*)&rgba);
    pl_rig_model_color_set(rig, &color, slot);
}

/* 0x80223658 (0xB0): Sets model `slot`'s two dyed materials to `color`. */
void pl_rig_model_color_set(PlSeRig* rig, _GXColor* color, u32 slot)
{
    u8 mat = rig->color_mat_0x9CD[slot][0];

    if (mat != 0xFF) {
        rig->models_0x004[slot].model.setMatColor(mat, GX_COLOR0, *color, false);
        if (rig->color_mat_0x9CD[slot][1] != 0xFF) {
            rig->models_0x004[slot].model.setMatColor(rig->color_mat_0x9CD[slot][1], GX_COLOR0, *color, false);
        }
    }
}

/* 0x80223708 (0x54): Sets model `slot`'s skin material to `color`. */
void pl_rig_model_skin_color_set(PlSeRig* rig, _GXColor* color, u32 slot)
{
    u8 mat = rig->skin_mat_0x9DB[slot];

    if (mat != 0xFF) {
        rig->models_0x004[slot].model.setMatColor(mat, GX_COLOR0, *color, false);
    }
}

/* 0x8022375C (0xD4): Dyes sub-model `slot`: the armour colour while the actor's piece asks for it, else the
 * material's own colour. */
void pl_rig_sub_color_apply(PlSeRig* rig, u32 slot)
{
    if (rig->sub_mat_0xA01[slot] != 0xFF) {
        if (equip_blade_dye_ck(&rig->plw->equipB) != 0) {
            _GXColor color;
            s32 rgba = armor_color_rgba_get();

            color_rgba_copy((u8*)&color, (u8*)&rgba);
            rig->sub_0x0A14[slot].model.setMatColor(rig->sub_mat_0xA01[slot], GX_COLOR0, color, false);
            return;
        }
        rig->sub_0x0A14[slot].model.setMatColor(rig->sub_mat_0xA01[slot], GX_COLOR0, rig->sub_color_0xA08[slot], false);
    }
}

/* 0x80223830 (0x48): Sets the hair model's coloured material to `color`. */
void pl_rig_hair_color_set(PlSeRig* rig, _GXColor* color)
{
    u8 mat = rig->hair_mat_0x9E2;

    if (mat != 0xFF) {
        rig->models_0x004[5].model.setMatColor(mat, GX_COLOR0, *color, false);
    }
}

/* 0x80223878 (0xD0): Maps each node of the hair model to the hair part its name starts with. */
void pl_rig_hair_parts_map(PlSeRig* rig)
{
    MHchar* model = &rig->models_0x004[5].model;
    u32 count;
    u32 node;
    u8 part;

    if (model->field_0x118 == 0) {
        return;
    }
    count = mhchar_node_count(model);
    if (count > 0x16) {
        count = 0x15;
    }
    for (node = 0; node < count; node++) {
        part = 0;
        do {
            nw4r::g3d::ResNode res = pl_rig_res_mdl(model)->GetResNode(node);
            s32 len = strlen(pl_rig_hair_part_names[part]);

            if (memcmp(pl_rig_hair_part_names[part], em007_res_node_name((ResHandle*)&res), len) == 0) {
                rig->hair_part_0x9E3[node] = part;
            }
            part++;
        } while (part < 9);
    }
}

/* 0x80223948 (0xD0): Maps each node of the face model to the face part its name starts with. */
void pl_rig_face_parts_map(PlSeRig* rig)
{
    MHchar* model = &rig->models_0x004[6].model;
    u32 count;
    u32 node;
    u8 part;

    if (model->field_0x118 == 0) {
        return;
    }
    count = mhchar_node_count(model);
    if (count > 8) {
        count = 7;
    }
    for (node = 0; node < count; node++) {
        part = 0;
        do {
            nw4r::g3d::ResNode res = pl_rig_res_mdl(model)->GetResNode(node);
            s32 len = strlen(pl_rig_face_part_names[part]);

            if (memcmp(pl_rig_face_part_names[part], em007_res_node_name((ResHandle*)&res), len - 1) == 0) {
                rig->face_part_0x9F9[node] = part;
            }
            part++;
        } while (part < 5);
    }
}

/* 0x80223A18 (0x2C): Row `index` of hair-style table `table` (8 bytes a row), NULL for -1. */
struct PlRigHairStyle* pl_rig_hair_style_get(u8 table, s32 index)
{
    struct PlRigHairStyle* base = pl_rig_hair_style_tbls[table];

    if (index == -1) {
        return 0;
    }
    return base + index;
}

/* 0x80223A44 (0x2C): Row `index` of face-style table `table` (2 bytes a row), NULL for -1. */
struct PlRigFaceStyle* pl_rig_face_style_get(u8 table, s32 index)
{
    struct PlRigFaceStyle* base = pl_rig_face_style_tbls[table];

    if (index == -1) {
        return 0;
    }
    return base + index;
}

/* 0x80223A70 (0x6C): The style byte of row `index` for set kind `kind` (4 hair, 5 face), 0 otherwise. */
u8 pl_rig_style_byte_get(u8 table, s32 kind, s32 index)
{
    switch (kind) {
    case 5: {
        struct PlRigFaceStyle* face = pl_rig_face_style_get(table, index);

        if (face != 0) {
            return face->parts_0x00;
        }
        break;
    }
    case 4: {
        struct PlRigHairStyle* hair = pl_rig_hair_style_get(table, index);

        if (hair != 0) {
            return hair->kind_0x04;
        }
        break;
    }
    }
    return 0;
}

/* 0x80223ADC (0x58): Shows the hair parts the actor's hair style lists. */
void pl_rig_hair_parts_apply(PlSeRig* rig)
{
    struct PlRigHairStyle* hair = pl_rig_hair_style_get(rig->plw->se_name_set, rig->plw->set_applied[4]);

    pl_rig_hair_parts_show(rig, hair != 0 ? hair->parts_0x00 : -1);
}

/* 0x80223B34 (0x98): Shows each hair node whose part bit is set in `mask`. */
void pl_rig_hair_parts_show(PlSeRig* rig, u32 mask)
{
    MHchar* model = &rig->models_0x004[5].model;
    u32 count;
    u32 node;

    if (model->field_0x118 == 0) {
        return;
    }
    count = mhchar_node_count(model);
    for (node = 0; node < count; node++) {
        u8 part = rig->hair_part_0x9E3[node];

        if (part != 0xFF) {
            model->setVisibility(node, (mask & (1 << part)) != 0);
        }
    }
}

/* 0x80223BCC (0x88): Shows the face parts the actor's hair style lists, without the brow under a hood. */
void pl_rig_face_parts_apply(PlSeRig* rig)
{
    struct PlRigHairStyle* hair = pl_rig_hair_style_get(rig->plw->se_name_set, rig->plw->set_applied[4]);
    struct PlRigFaceStyle* face = pl_rig_face_style_get(rig->plw->se_name_set, rig->plw->set_applied[5]);
    u32 mask;

    if (hair != 0) {
        mask = hair->face_0x05;
    } else {
        mask = -1;
    }
    if (face != 0 && face->hood_0x01 != 0) {
        mask &= ~4;
    }
    pl_rig_face_parts_show(rig, mask);
}

/* 0x80223C54 (0x98): Shows each face node whose part bit is set in `mask`. */
void pl_rig_face_parts_show(PlSeRig* rig, u32 mask)
{
    MHchar* model = &rig->models_0x004[6].model;
    u32 count;
    u32 node;

    if (model->field_0x118 == 0) {
        return;
    }
    count = mhchar_node_count(model);
    for (node = 0; node < count; node++) {
        u8 part = rig->face_part_0x9F9[node];

        if (part != 0xFF) {
            model->setVisibility(node, (mask & (1 << part)) != 0);
        }
    }
}

/* 0x80223CEC (0x168): Loads model `slot` from the actor's equipment file and hooks its world callback. */
void pl_rig_model_load(PlSeRig* rig, u8 slot)
{
    u32 file;
    u32 res;
    u32 arg;
    MHchar* model;

    arg = pl_equip_file_get(rig->plw->chunk_ofs, slot);
    res_file_handle_copy(&res, &arg);
    file = res;
    pl_rig_model_setup(rig, &file, slot, pl_rig_model_flags[slot]);
    model = &rig->models_0x004[slot].model;
    if (model->field_0x118 != 0) {
        model->field_0x40 = 0;
        if (slot != 0) {
            PlSeSlot* cb = &rig->slots_0xE40[slot];

            pl_rig_scn_mdl(model)->SetCalcWorldCallback(pl_rig_slot_cb(cb));
            pl_rig_slot_bind(rig, cb, slot);
            pl_rig_slot_cb(cb)->ApplyTiming(pl_rig_scn_mdl(model), 2, 0);
            return;
        }
        {
            PlSeSlot* cb = &rig->slots_0xE40[slot];

            pl_rig_scn_mdl(model)->SetCalcWorldCallback(pl_rig_slot_cb(cb));
            pl_rig_slot_bind(rig, cb, slot);
            pl_rig_slot_cb(cb)->ApplyTiming(pl_rig_scn_mdl(model), 1, 1);
            pl_rig_slot_cb(cb)->ApplyTiming(pl_rig_scn_mdl(model), 2, 0);
        }
    }
}

/* 0x80223E54 (0x9C): Picks the first sub-model's attachment kind from the actor's weapon type. */
void pl_rig_weapon_kind_set(PlSeRig* rig)
{
    rig->sub_kind_0x9CA[0] = 0xFF;
    rig->sub_kind_0x9CA[1] = 0xFF;
    switch (rig->plw->field_0x002) {
    default:
        rig->sub_kind_0x9CA[0] = 0;
        break;
    case 1:
        rig->sub_kind_0x9CA[0] = 1;
        break;
    case 2:
        rig->sub_kind_0x9CA[0] = 2;
        break;
    case 4:
    case 5:
    case 6:
        rig->sub_kind_0x9CA[0] = 4;
        break;
    case 7:
        rig->sub_kind_0x9CA[0] = 7;
        break;
    case 8:
        rig->sub_kind_0x9CA[0] = 8;
        break;
    case 3:
        rig->sub_kind_0x9CA[0] = 3;
        break;
    }
}

/* 0x80223EF0 (0x3C8): Loads sub-model `slot` from equipment file `file_id`, picks its attachment kind, hooks its
 * world callback and maps its dyed material. */
void pl_rig_weapon_load(PlSeRig* rig, u8 slot, u8 file_id)
{
    u32 res;
    u32 arg;
    u32 flags;
    u8 light;
    u8 kind;
    MHchar* model;
    PlSeAttach* attach;
    u8 i;

    res_file_ctor(&res, 0);
    flags = 0x2C;
    if ((u8)PlayMode_ck() != 4) {
        u8 mode = GameMode_ck();

        if (mode == 0 || mode == 3) {
            light = 3;
        } else {
            light = rig->plw->chunk_ofs + 3;
        }
    } else {
        light = 3;
    }
    switch (rig->plw->field_0x002) {
    default:
        rig->sub_kind_0x9CA[slot] = 0;
        break;
    case 1:
        switch (slot) {
        case 0:
            kind = 1;
            break;
        case 1:
            kind = 9;
            break;
        }
        rig->sub_kind_0x9CA[slot] = kind;
        break;
    case 2:
        rig->sub_kind_0x9CA[slot] = 2;
        break;
    case 4:
    case 5:
    case 6:
        switch (slot) {
        case 0:
            kind = 4;
            break;
        case 1:
            kind = 5;
            break;
        case 2:
            kind = 6;
            flags |= 8;
            break;
        }
        rig->sub_kind_0x9CA[slot] = kind;
        break;
    case 7:
        rig->sub_kind_0x9CA[slot] = 7;
        break;
    case 8:
        rig->sub_kind_0x9CA[slot] = 8;
        break;
    case 3:
        switch (slot) {
        case 0:
            rig->sub_kind_0x9CA[slot] = 3;
            break;
        case 1:
            rig->sub_kind_0x9CA[slot] = 10;
            break;
        }
        break;
    }
    arg = pl_equip_file_get(rig->plw->chunk_ofs, file_id);
    res_file_assign(&res, &arg);
    model = &rig->sub_0x0A14[slot].model;
    mhchar_model_create(model, &res, flags, light);
    rig->sub_0x0A14[slot].model.field_0x40 = 0;
    attach = &rig->attachments_0x1230[slot];
    pl_rig_scn_mdl(&rig->sub_0x0A14[slot].model)->SetCalcWorldCallback(pl_rig_attach_cb(attach));
    pl_rig_attach_bind(rig, attach, rig->sub_kind_0x9CA[slot], model);
    pl_rig_attach_cb(attach)->ApplyTiming(pl_rig_scn_mdl(&rig->sub_0x0A14[slot].model), 1, 0);
    pl_rig_attach_cb(attach)->ApplyTiming(pl_rig_scn_mdl(&rig->sub_0x0A14[slot].model), 2, 0);
    rig->sub_mat_0xA01[slot] = 0xFF;
    for (i = 0; i < mhchar_mat_num_get(model); i++) {
        nw4r::g3d::ResMat mat = pl_rig_res_mdl(&rig->sub_0x0A14[slot].model)->GetResMat(i);

        if (memcmp(res_mat_name_get(&mat), pl_rig_mat_sym, 3) == 0) {
            rig->sub_mat_0xA01[slot] = i;
            model->getMatColor(i, GX_COLOR0, &rig->sub_color_0xA08[slot]);
            break;
        }
    }
    for (i = 0; i < 7; i++) {
        pl_rig_joint_adj_reset(rig, &rig->attachments_0x1230[slot].adj_0x014[i]);
    }
    if (pl_item_page2_ck(rig->plw) != 0 && rig->sub_0x0A14[2].model.field_0x118 != 0) {
        nw4r::g3d::ResNode tmp(0);
        bool show = pl_weapon_alt_form_ck(rig->plw) == 0;
        nw4r::g3d::ResNode sub = pl_rig_res_mdl(&rig->sub_0x0A14[2].model)->GetResNode(2);

        res_node_assign((ResHandle*)&tmp, (ResHandle*)&sub);
        if (res_node_is_valid((ResHandle*)&tmp) != 0) {
            rig->sub_0x0A14[2].model.setVisibility(res_node_get_id((ResHandle*)&tmp), show);
        }
    }
}

/* 0x802242B8 (0x1E0): Re-binds every loaded model's world callback after the actor changed. */
void pl_rig_callbacks_rebind(PlSeRig* rig)
{
    s32 i;

    for (i = 1; i < 7; i++) {
        if (rig->plw->set_applied[i] != -1 && rig->models_0x004[i].model.field_0x118 != 0) {
            pl_rig_slot_bind(rig, &rig->slots_0xE40[i], i);
            pl_rig_slot_cb(&rig->slots_0xE40[i])->ApplyTiming((nw4r::g3d::ScnMdlSimple*)rig->models_0x004[i].model.field_0x118, 2,
                                                         0);
        }
    }
    if (rig->sub_kind_0x9CA[0] != 0xFF && rig->sub_0x0A14[0].model.field_0x118 != 0) {
        pl_rig_attach_bind(rig, &rig->attachments_0x1230[0], rig->sub_kind_0x9CA[0], &rig->sub_0x0A14[0].model);
        pl_rig_attach_cb(&rig->attachments_0x1230[0])->ApplyTiming(pl_rig_scn_mdl(&rig->sub_0x0A14[0].model), 1, 0);
        pl_rig_attach_cb(&rig->attachments_0x1230[0])->ApplyTiming(pl_rig_scn_mdl(&rig->sub_0x0A14[0].model), 2, 0);
    }
    if (rig->sub_kind_0x9CA[1] != 0xFF && rig->sub_0x0A14[1].model.field_0x118 != 0) {
        pl_rig_attach_bind(rig, &rig->attachments_0x1230[1], rig->sub_kind_0x9CA[1], &rig->sub_0x0A14[1].model);
        pl_rig_attach_cb(&rig->attachments_0x1230[1])->ApplyTiming(pl_rig_scn_mdl(&rig->sub_0x0A14[1].model), 1, 0);
        pl_rig_attach_cb(&rig->attachments_0x1230[1])->ApplyTiming(pl_rig_scn_mdl(&rig->sub_0x0A14[1].model), 2, 0);
    }
    if (rig->sub_kind_0x9CA[2] != 0xFF && rig->sub_0x0A14[2].model.field_0x118 != 0) {
        pl_rig_attach_bind(rig, &rig->attachments_0x1230[2], rig->sub_kind_0x9CA[2], &rig->sub_0x0A14[2].model);
        pl_rig_attach_cb(&rig->attachments_0x1230[2])->ApplyTiming(pl_rig_scn_mdl(&rig->sub_0x0A14[2].model), 1, 0);
        pl_rig_attach_cb(&rig->attachments_0x1230[2])->ApplyTiming(pl_rig_scn_mdl(&rig->sub_0x0A14[2].model), 2, 0);
    }
}

/* 0x80224498 (0xB8): Maps node `node` of model `index` onto the body joint of the same name. */
void pl_rig_slot_node_map(PlSeRig* rig, PlSeSlot* slot, u32 index, u32 node)
{
    nw4r::g3d::ResNode res(0);
    nw4r::g3d::ResNode own = pl_rig_res_mdl(&rig->models_0x004[index].model)->GetResNode(node);
    nw4r::g3d::ResNode body = pl_rig_res_mdl(&rig->models_0x004[0].model)->GetResNode(em007_res_node_name((ResHandle*)&own));

    res_node_assign((ResHandle*)&res, (ResHandle*)&body);
    if (res_node_is_valid((ResHandle*)&res) != 0) {
        slot->node_map_0x10[node] = res_node_mtx_id_get((ResHandle*)&res);
    }
}

/* 0x80224550 (0x2A0): Binds callback slot `slot` to model `index`: its root follows the body joint the model kind
 * hangs from, every other node the body joint of its name. */
void pl_rig_slot_bind(PlSeRig* rig, PlSeSlot* slot, u32 index)
{
    nw4r::g3d::ResNode res(0);
    u32 count;
    u32 node;

    slot->body_0x08 = &rig->models_0x004[0].model;
    slot->index_0x0C = index;
    slot->plw_0x04 = rig->plw;
    slot->node_map_0x10[0] = -1;
    slot->node_map_0x10[1] = -1;
    slot->node_map_0x10[2] = -1;
    slot->node_map_0x10[3] = -1;
    slot->node_map_0x10[4] = -1;
    slot->node_map_0x10[5] = -1;
    slot->node_map_0x10[6] = -1;
    slot->node_map_0x10[7] = -1;
    slot->node_map_0x10[8] = -1;
    slot->node_map_0x10[9] = -1;
    slot->node_map_0x10[10] = -1;
    slot->node_map_0x10[11] = -1;
    slot->node_map_0x10[12] = -1;
    slot->node_map_0x10[13] = -1;
    slot->node_map_0x10[14] = -1;
    slot->node_map_0x10[15] = -1;
    slot->node_map_0x10[16] = -1;
    slot->node_map_0x10[17] = -1;
    slot->node_map_0x10[18] = -1;
    slot->node_map_0x10[19] = -1;
    slot->node_map_0x10[20] = -1;
    slot->node_map_0x10[21] = -1;
    slot->node_map_0x10[22] = -1;
    slot->node_map_0x10[23] = -1;
    slot->node_map_0x10[24] = -1;
    slot->node_map_0x10[25] = -1;
    slot->node_map_0x10[26] = -1;
    slot->node_map_0x10[27] = -1;
    slot->node_map_0x10[28] = -1;
    slot->node_map_0x10[29] = -1;
    slot->node_map_0x10[30] = -1;
    slot->node_map_0x10[31] = -1;
    if (index == 0) {
        return;
    }
    switch (index) {
    case 1: {
        nw4r::g3d::ResNode body = pl_rig_res_mdl(&rig->models_0x004[0].model)->GetResNode(3);

        res_node_assign((ResHandle*)&res, (ResHandle*)&body);
        break;
    }
    case 2: {
        nw4r::g3d::ResNode body = pl_rig_res_mdl(&rig->models_0x004[0].model)->GetResNode(1);

        res_node_assign((ResHandle*)&res, (ResHandle*)&body);
        break;
    }
    case 3: {
        nw4r::g3d::ResNode body = pl_rig_res_mdl(&rig->models_0x004[0].model)->GetResNode(0xE);

        res_node_assign((ResHandle*)&res, (ResHandle*)&body);
        break;
    }
    case 4: {
        nw4r::g3d::ResNode body = pl_rig_res_mdl(&rig->models_0x004[0].model)->GetResNode(3);

        res_node_assign((ResHandle*)&res, (ResHandle*)&body);
        break;
    }
    case 5: {
        nw4r::g3d::ResNode body = pl_rig_res_mdl(&rig->models_0x004[0].model)->GetResNode(2);

        res_node_assign((ResHandle*)&res, (ResHandle*)&body);
        break;
    }
    case 6: {
        nw4r::g3d::ResNode body = pl_rig_res_mdl(&rig->models_0x004[0].model)->GetResNode(0xC);

        res_node_assign((ResHandle*)&res, (ResHandle*)&body);
        break;
    }
    }
    if (res_node_is_valid((ResHandle*)&res) != 0) {
        slot->node_map_0x10[0] = res_node_mtx_id_get((ResHandle*)&res);
    }
    {
        nw4r::g3d::ResMdl* mdl = pl_rig_res_mdl(&rig->models_0x004[index].model);

        count = mdl->GetResNodeNumEntries();
        if (index - 4 <= 1) {
            for (node = 1; node < count; node++) {
                nw4r::g3d::ResNode own = mdl->GetResNode(node);
                s32 len = strlen(pl_rig_node_cl);

                if (memcmp(pl_rig_node_cl, em007_res_node_name((ResHandle*)&own), len) != 0) {
                    pl_rig_slot_node_map(rig, slot, index, node);
                }
            }
            return;
        }
        for (node = 1; node < count; node++) {
            pl_rig_slot_node_map(rig, slot, index, node);
        }
    }
}

/* 0x802247F0 (0x30): Binds attachment `attach` of kind `kind` to `model`; kinds 5 and 6 follow the sub-models. */
void pl_rig_attach_bind(PlSeRig* rig, PlSeAttach* attach, u8 kind, MHchar* model)
{
    attach->kind_0x10 = kind;
    attach->model_0x08 = model;
    if ((u32)(kind - 5) <= 1) {
        attach->models_0x04 = rig->sub_0x0A14;
        return;
    }
    attach->models_0x04 = rig->models_0x004;
}

/* 0x80224820 (0x64): Places `model` at the actor's position and rotation with uniform `scale`. */
void pl_rig_model_place(PlSeRig* rig, MHchar* model, _PLW* plw, f32 scale)
{
    model->field_0x28 = plw->param_0x54;
    model->field_0x2C = plw->field_0x058;
    model->field_0x30 = plw->rot_z_0x5C;
    copyVec3(&model->pos_0x04, &plw->vec_0x03C);
    model->scale_0x1C.x = scale;
    model->scale_0x1C.y = scale;
    model->scale_0x1C.z = scale;
}

/* 0x80224884 (0xC0): Whether the player's effects are drawn (0 while the player is hidden, inactive or not yet
 * ready). */
u8 pl_eft_draw_ck(_PLW* plw)
{
    if (plw->field_0x19 > 0) {
        return 0;
    }
    if (plw->field_0x001 == 0) {
        return 0;
    }
    if (((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x34 == 0) {
        return 0;
    }
    if (((PlSeRig*)plw->physics_0x13C)->field_0x9C0[0] == 0) {
        return 0;
    }
    if (Pl_master_ck(plw) == 0 && (u8)GameMode_ck() == 2 && game_ready_ck() == 1 && plw->field_0xB04 == 0) {
        return 0;
    }
    return 1;
}

/* 0x80224944 (0x48): Whether the actor's screen is ready (always outside the lobby). */
u32 pl_rig_screen_ok_ck(_PLW* plw)
{
    switch ((u8)GameMode_ck()) {
    case 2:
        return lb_player_screen_ok_ck(plw);
    }
    return 1;
}

/* 0x8022498C (0x9C): Sets the draw mode of every model of the rig before the update. */
void pl_rig_draw_mode_begin(PlSeRig* rig)
{
    s32 i;

    for (i = 0; i < 7; i++) {
        mhchar_draw_mode_set(&rig->models_0x004[i].model, rig->models_0x004[i].model.field_0x38 | 1);
    }
    for (i = 0; i < 3; i++) {
        mhchar_draw_mode_set(&rig->sub_0x0A14[i].model, rig->sub_0x0A14[i].model.field_0x38 | 1);
    }
}

/* 0x80224A28 (0x9C): Clears the draw mode bit again after the update. */
void pl_rig_draw_mode_end(PlSeRig* rig)
{
    s32 i;

    for (i = 0; i < 7; i++) {
        mhchar_draw_mode_set(&rig->models_0x004[i].model, rig->models_0x004[i].model.field_0x38 & ~1);
    }
    for (i = 0; i < 3; i++) {
        mhchar_draw_mode_set(&rig->sub_0x0A14[i].model, rig->sub_0x0A14[i].model.field_0x38 & ~1);
    }
}

}  /* extern "C" */

/* ------------------------------------------------------------------------------------------------ */
/* 0x80224AC4 */

/* Clears the actor's live flag when any gate says the model must not move, then drives every model
 * record the rig carries: gate byte, model helper, placement and the model's own `move`. */
extern "C" void pl_rig_model_move(PlSeRig* rig)
{
    u32 flag = rig->plw->field_0x001;
    u32 i;
    u32 v;

    if ((u8)GameMode_ck() == 1) {
        if (pl_now_area_ck(rig->plw) == 0) {
            flag = 0;
        }
        if ((u8)PlayMode_ck() == 3) {
            if (pl_camera_near_head_ck(rig->plw) == 1) {
                flag = 0;
            }
        }
    }
    if (rig->plw->field_0x19 > 0) {
        flag = 0;
    }
    if (Pl_master_ck(rig->plw) == 0) {
        if ((u8)GameMode_ck() == 2 && game_ready_ck() == 1 && rig->plw->field_0xB04 == 0) {
            flag = 0;
        }
    }
    if (rig->field_0x9C0[0] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[1] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[2] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[3] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[4] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[5] == 0) {
        flag = 0;
    } else if (rig->field_0x9C0[6] == 0) {
        flag = 0;
    }

    if (Pl_master_ck(rig->plw) == 1) {
        v = split_screen_size_class(rig->plw) | 0x200;
    } else {
        v = 0;
    }

    for (i = 0; i < 7; i++) {
        PlSeModel* m = &rig->models_0x004[i];

        if (m->model.field_0x118 == 0) {
            continue;
        }
        m->model.field_0x34 = flag;
        if (flag != 0) {
            m->model.field_0x34 = rig->field_0x9C0[i];
        }
        pl_equip_color_apply(rig->plw, i);
        if (m->model.field_0x34 != 0) {
            pl_rig_model_skin_color_set(rig, (_GXColor*)rig->plw->field_0x25C, i);
            if (i == 5) {
                pl_rig_hair_color_set(rig, (_GXColor*)rig->plw->field_0x258);
            }
        }
        pl_rig_model_place(rig, &m->model, rig->plw, 1.0f);
        m->model.move((u16)v);
    }

    for (i = 0; i < 3; i++) {
        PlSeModel* m = &rig->sub_0x0A14[i];
        PlSeAttach* a = &rig->attachments_0x1230[i];

        if (m->model.field_0x118 == 0) {
            continue;
        }
        m->model.field_0x34 = flag;
        if (flag != 0) {
            m->model.field_0x34 = rig->field_0x9C7[i];
        }
        if (pl_rig_screen_ok_ck(rig->plw) == 0) {
            m->model.field_0x34 = 0;
        }
        if (rig->plw->field_0x01E != 0) {
            m->model.field_0x34 = 0;
        }
        if (m->model.field_0x34 != 0) {
            pl_rig_sub_color_apply(rig, i);
        }
        pl_rig_model_place(rig, &m->model, rig->plw, pl_rig_attach_pose_set(rig, i, a->kind_0x10));
        m->model.move((u16)v);
        pl_rig_attach_motion_pose(rig, a, a->kind_0x10);
    }

    if (flag != 0) {
        for (i = 0; i < 7; i++) {
            PlSeModel* m = &rig->models_0x004[i];

            if (m->model.field_0x118 != 0 && m->model.field_0x34 != 0) {
                g3d_root_model_bind(pRoot, m->model.field_0x118);
            }
        }
        for (i = 0; i < 3; i++) {
            PlSeModel* m = &rig->sub_0x0A14[i];

            if (m->model.field_0x118 != 0 && m->model.field_0x34 != 0) {
                g3d_root_model_bind(pRoot, m->model.field_0x118);
            }
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80224E28 */

/* Maps one of the actor's motion slots onto the per-slot decision table, then lets the motion number
 * override it: 0 = blocked, 1 = allowed, 2 = the model's own answer. */
extern "C" u8 pl_slot_motion_allow_get(_PLW* self, u32 slot)
{
    u16 motion = Get_motion_no(self);
    u8 flag = 2;

    switch (slot) {
    case 0:
        flag = pl_rig_kind0_allow_get(self, motion);
        break;
    case 1:
        flag = pl_rig_kind1_allow_get(self, motion);
        break;
    case 3:
        flag = pl_rig_kind3_allow_get(self, motion);
        break;
    case 4:
        flag = pl_rig_kind4_allow_get(self, motion);
        break;
    case 7:
        flag = pl_rig_kind7_allow_get(self, motion);
        break;
    case 2:
        flag = pl_rig_kind2_allow_get(self, motion);
        break;
    case 8:
        flag = pl_rig_kind8_allow_get(self, motion);
        break;
    default:
        if (fn_8026A3A0(self) != 0) {
            flag = 1;
        }
        break;
    }

    switch (motion) {
    case 202:
    case 204:
        if (self->field_0x18 != 0) {
            flag = 1;
        } else {
            flag = 2;
        }
        break;
    case 205:
    case 206:
    case 208:
    case 209:
    case 211:
    case 212:
    case 213:
    case 214:
    case 215:
    case 219:
    case 220:
    case 253:
    case 255:
    case 263:
    case 264:
    case 271:
        flag = 2;
        break;
    case 313:
    case 356:
        if (Pl_frame_check_alt(self, 1, lbl_80799CE0, lbl_80799CDC) == 1) {
            flag = 2;
        } else if (Pl_frame_check_alt(self, 1, lbl_80799CE4, lbl_80799CDC) == 1) {
            flag = 1;
        } else {
            flag = 2;
        }
        break;
    case 60:
    case 61:
    case 135:
    case 136:
    case 421:
    case 422:
    case 500:
    case 501:
    case 502:
    case 503:
    case 504:
    case 505:
    case 506:
    case 507:
    case 508:
    case 509:
    case 510:
    case 511:
    case 512:
    case 513:
    case 514:
    case 515:
    case 516:
    case 517:
    case 518:
    case 520:
    case 521:
    case 522:
    case 523:
    case 524:
    case 525:
    case 526:
    case 527:
    case 528:
    case 529:
    case 530:
    case 531:
    case 532:
    case 533:
    case 534:
        flag = 2;
        break;
    default:
        break;
    }

    return flag;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802250E4 */

/* Reads the current SE bank index, then returns that bank's gain from the `.sdata` gain table. */
extern "C" f32 pl_rig_bank_gain_get(_EQUIP* equip)
{
    return pl_rig_bank_gain_tbl[pl_rig_bank_kind_tbl[equip_variant_get(equip)]];
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x8022511C */

/* Interpolates the vector pair of key table `keys` at the actor's motion frame into `out` (the last key's pair once
 * the table's end mark is reached); returns the key's frame. */
extern "C" f32 pl_rig_key_lerp(_PLW* plw, PlRigKey* keys, PlSeVecPair* out)
{
    for (;; keys++) {
        if (-1.0f == keys->frame_0x00) {
            vec3_float_copy(&out->x_0x00, &keys->pos_0x10.x);
            out->x_0x0C = keys->rot_0x04.x;
            out->y_0x10 = keys->rot_0x04.y;
            out->z_0x14 = keys->rot_0x04.z;
            return keys->frame_0x00;
        }
        if (Pl_frame_check_alt(plw, 5, keys->frame_0x00, keys[1].frame_0x00) != 0) {
            f32 t = (pl_rig_get_motion_frame(plw) - keys->frame_0x00) / (keys[1].frame_0x00 - keys->frame_0x00);

            out->x_0x00 = keys->pos_0x10.x + t * (keys[1].pos_0x10.x - keys->pos_0x10.x);
            out->y_0x04 = keys->pos_0x10.y + t * (keys[1].pos_0x10.y - keys->pos_0x10.y);
            out->z_0x08 = keys->pos_0x10.z + t * (keys[1].pos_0x10.z - keys->pos_0x10.z);
            out->x_0x0C = keys->rot_0x04.x + t * (keys[1].rot_0x04.x - keys->rot_0x04.x);
            out->y_0x10 = keys->rot_0x04.y + t * (keys[1].rot_0x04.y - keys->rot_0x04.y);
            out->z_0x14 = keys->rot_0x04.z + t * (keys[1].rot_0x04.z - keys->rot_0x04.z);
            return keys->frame_0x00;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80225280 */

/* The weapon pose of the lance/gunlance guard motions: the motion's key table interpolated into `out`, or the drawn
 * pose of `kind` for every other motion. */
extern "C" PlSeVecPair* pl_rig_guard_pose_get(_PLW* plw, u16 motion, u8 kind, PlSeVecPair* out)
{
    switch (motion) {
    case 0x40B:
    case 0x46B:
        pl_rig_key_lerp(plw, pl_rig_keys_40b, out);
        break;
    case 0x40C:
        pl_rig_key_lerp(plw, pl_rig_keys_40c, out);
        break;
    case 0x410:
        pl_rig_key_lerp(plw, pl_rig_keys_410, out);
        break;
    case 0x40D:
        pl_rig_key_lerp(plw, pl_rig_keys_40d, out);
        break;
    case 0x46C:
        pl_rig_key_lerp(plw, pl_rig_keys_46c, out);
        break;
    case 0x40E:
        pl_rig_key_lerp(plw, pl_rig_keys_40e, out);
        break;
    case 0x43A:
        pl_rig_key_lerp(plw, pl_rig_keys_43a, out);
        break;
    case 0x43B:
        pl_rig_key_lerp(plw, pl_rig_keys_43b, out);
        break;
    case 0x43C:
        pl_rig_key_lerp(plw, pl_rig_keys_43c, out);
        break;
    case 0x43D:
        pl_rig_key_lerp(plw, pl_rig_keys_43d, out);
        break;
    case 0x49D:
        pl_rig_key_lerp(plw, pl_rig_keys_49d, out);
        break;
    case 0x49E:
        pl_rig_key_lerp(plw, pl_rig_keys_49e, out);
        break;
    default:
        out = pl_rig_pose_get(plw, 4, kind);
        break;
    }
    return out;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80225434 */

/* Resets attachment `index`'s joint adjustments and poses its root for kind `kind` from the actor's motion and
 * weapon state; returns the attachment's scale. */
extern "C" f32 pl_rig_attach_pose_set(PlSeRig* rig, u32 index, u32 kind)
{
    _PLW* plw = rig->plw;
    nw4r::math::MTX34 mtx;
    PlSeVecPair keyed;
    nw4r::math::VEC3 unused_a;
    f32 rot[3];
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 trans;
    PlSeJointAdj* adj;
    s32 i;
    u16 motion;
    u8 allow;
    PlSeVecPair* pose;
    f32 scale;
    s32 mode;

    MTX34_ctor(&mtx);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    adj = rig->attachments_0x1230[index].adj_0x014;
    for (i = 0; i < 7; i++) {
        pl_rig_joint_adj_reset(rig, &adj[i]);
    }
    motion = Get_motion_no(plw);
    do {
        switch (kind) {
        case 9:
        case 10:
            allow = 4;
            break;
        case 5:
            rot[0] = 0.0f;
            rot[1] = 20.0f;
            rot[2] = pl_rig_bank_gain_get(&plw->equipB);
            pose = (PlSeVecPair*)rot;
            trans.z = 0.0f;
            trans.y = 0.0f;
            trans.x = 0.0f;
            scale = 1.0f;
            continue;
        case 6:
            return 1.0f;
        default:
            allow = pl_slot_motion_allow_get(plw, kind);
            break;
        }
        if (system_lobby_flag_get() == 2) {
            pose = pl_rig_lobby_pose_get(plw, kind, 0);
        } else if ((motion == 313 || motion == 356) && allow == 1) {
            pose = pl_rig_pose_get(plw, kind, 3);
        } else if ((u32)(plw->field_0x002 - 4) <= 2) {
            if (pl_weapon_alt_form_ck(plw) != 0 &&
                ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 <= 0.0f && allow != 1) {
                pose = pl_rig_guard_pose_get(plw, motion, allow, &keyed);
            } else {
                pose = pl_rig_pose_get(plw, kind, allow);
            }
        } else {
            pose = pl_rig_pose_get(plw, kind, allow);
        }
        vec3_float_copy(&trans.x, &pose->x_0x0C);
        scale = 1.0f;
        switch (allow) {
        case 4:
            mode = 10;
            break;
        case 0:
            mode = 11;
            break;
        case 1:
            mode = 7;
            break;
        default:
            if (kind == 1 || kind == 8) {
                mode = 2;
            } else {
                mode = 3;
            }
            switch (plw->field_0x002) {
            case 4:
            case 5:
            case 6:
                if (pl_weapon_alt_form_ck(plw) != 1) {
                    scale = 0.8f;
                } else {
                    scale = 0.9f;
                }
                break;
            default:
                scale = 0.8f;
                break;
            case 1:
                scale = 1.0f;
                break;
            case 3:
                scale = 0.9f;
                break;
            case 8:
                scale = 0.9f;
                break;
            }
            break;
        }
        rig->attachments_0x1230[index].joint_0x0C = mode;
        rig->attachments_0x1230[index].allow_0x11 = allow;
    } while (0);
    adj->rot_0x18.x = trans.x;
    adj->rot_0x18.y = trans.y;
    adj->rot_0x18.z = trans.z;
    adj->trans_0x0C.x = pose->x_0x00;
    adj->trans_0x0C.y = pose->y_0x04;
    adj->trans_0x0C.z = pose->z_0x08;
    return scale;
}


/* ------------------------------------------------------------------------------------------------ */
/* 0x80225734 */

/* The motion-state gate of kind-0 attachments for motion `motion`: 0 sheathed, 1 drawn, 2 the model's own answer
 * (from the motion frame windows and whether the body leans forward). */
extern "C" u8 pl_rig_kind0_allow_get(_PLW* self, u16 motion)
{
    u8 allow = 2;
    u8 leaning = ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

    switch (motion) {
    case 0x3EA:
        if (Pl_frame_check_alt(self, 1, 14.0f, 0.0f) == 1) {
            allow = 0;
        }
        break;
    case 0x3EB:
        if (Pl_frame_check_alt(self, 1, 36.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 0;
        }
        break;
    case 0x3F1:
        if (Pl_frame_check_alt(self, 1, 42.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 0;
        }
        break;
    case 0x450:
        allow = 0;
        break;
    case 0x46B:
        if (leaning != 0) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 18.0f, 0.0f) == 1) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 4.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 2;
        }
        break;
    case 0x41B:
        if (Pl_frame_check_alt(self, 1, 26.0f, 0.0f) == 1) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 4.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 2;
        }
        break;
    case 0x41C:
        if (leaning != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 62.0f, 0.0f) == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 1;
        }
        break;
    case 0x482:
        if (Pl_frame_check_alt(self, 6, 4.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 2;
        }
        break;
    case 0x421:
        if (leaning != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 48.0f, 0.0f) == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 8.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 1;
        }
        break;
    case 0x48F:
        if (leaning != 0) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 10.0f, 0.0f) == 1) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 2;
        }
        break;
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x41F:
    case 0x4BB:
    case 0x4BC:
    case 0x4BD:
    case 0x4ED:
    case 0x4EF:
        allow = 0;
        break;
    case 0x41E:
        if (leaning != 0 || Pl_frame_check_alt(self, 2, 2.0f, 0.0f) == 1) {
            allow = 1;
        } else {
            allow = 0;
        }
        break;
    case 0x420:
        if (leaning != 0 || Pl_frame_check_alt(self, 2, 16.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 1;
        }
        break;
    case 0x4EE:
        if (leaning != 0 || Pl_frame_check_alt(self, 2, 70.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 1;
        }
        break;
    default:
        allow = (fn_8026A3A0(self) == 0) + 1;
        break;
    }
    return allow;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80225B34 */

/* The motion-state gate of kind-2 attachments (the same scheme). */
extern "C" u8 pl_rig_kind2_allow_get(_PLW* self, u16 motion)
{
    u8 leaning;
    u8 allow = 2;

    leaning = ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

    switch (motion) {
    case 0x3EA:
        if (Pl_frame_check_alt(self, 1, 4.0f, 0.0f) != 0) {
            allow = 1;
        }
        if (leaning == 1) {
            allow = 2;
        }
        break;
    case 0x3EB:
        if (Pl_frame_check_alt(self, 1, 46.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x3F1:
        if (Pl_frame_check_alt(self, 1, 48.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x3F2:
        if (leaning == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x41B:
        if (leaning != 0) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) == 1) {
            allow = 1;
        } else {
            allow = 2;
        }
        break;
    case 0x41C:
        if (leaning != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 50.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x421:
        if (leaning != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 54.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x49A:
        if (leaning != 0) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) == 1) {
            allow = 1;
        } else {
            allow = 2;
        }
        break;
    default:
        if (fn_8026A3A0(self) == 1) {
            allow = 1;
        }
        break;
    }
    return allow;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80225D54 */

/* The motion-state gate of kind-1 attachments. */
extern "C" u8 pl_rig_kind1_allow_get(_PLW* self, u16 motion)
{
    u8 allow = 2;
    u8 leaning = ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

    switch (motion) {
    case 0x406:
    case 0x3FA:
    case 0x3EA:
        if (leaning != 0) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x3EB:
        if (Pl_frame_check_alt(self, 1, 32.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x3F1:
        if (Pl_frame_check_alt(self, 1, 30.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x3F8:
        if (Pl_frame_check_alt(self, 1, 14.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 2;
        }
        break;
    case 0x3F9:
        if (Pl_frame_check_alt(self, 1, 44.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 0;
        }
        break;
    case 0x41B:
        if (leaning != 0) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x41C:
        if (Pl_frame_check_alt(self, 1, 18.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x421:
        if (Pl_frame_check_alt(self, 1, 26.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x488:
        allow = 1;
        break;
    default:
        if (fn_8026A3A0(self) != 0) {
            allow = 1;
        }
        break;
    }
    return allow;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80225F48 */

/* The motion-state gate of kind-3 attachments. */
extern "C" u8 pl_rig_kind3_allow_get(_PLW* self, u16 motion)
{
    u8 allow;
    u8 leaning = ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

    switch (motion) {
    case 0x41B:
    case 0x3EA:
        if (leaning == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x3EB:
        if (Pl_frame_check_alt(self, 1, 62.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x41C:
        if (Pl_frame_check_alt(self, 1, 66.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x421:
    case 0x3F1:
        if (Pl_frame_check_alt(self, 1, 68.0f, 0.0f) == 1) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x46A:
    case 0x42E:
    case 0x3FA:
        if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) == 1) {
            allow = 1;
        } else {
            allow = 2;
        }
        break;
    case 0x49C:
        if (Pl_frame_check_alt(self, 1, 4.0f, 0.0f) == 1) {
            allow = 1;
        } else {
            allow = 2;
        }
        break;
    default:
        allow = (fn_8026A3A0(self) == 0) + 1;
        break;
    }
    return allow;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802260E4 */

/* The motion-state gate of kind-4 attachments. */
extern "C" u8 pl_rig_kind4_allow_get(_PLW* self, u16 motion)
{
    u8 allow = 2;
    u8 leaning = ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

    switch (motion) {
    case 0x49C:
    case 0x46A:
    case 0x41B:
    case 0x3EA:
        if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) != 0) {
            allow = 1;
        } else {
            allow = 2;
        }
        break;
    case 0x421:
    case 0x41C:
    case 0x3EB:
        if (Pl_frame_check_alt(self, 1, 110.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x3F1:
        if (Pl_frame_check_alt(self, 1, 90.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x46C:
    case 0x410:
        if (Pl_frame_check_alt(self, 1, 22.0f, 0.0f) != 0) {
            allow = 1;
        } else {
            allow = 2;
        }
        break;
    case 0x49E:
    case 0x49D:
    case 0x46B:
    case 0x43D:
    case 0x43A:
    case 0x40B:
        if (Pl_frame_check_alt(self, 1, 26.0f, 0.0f) != 0) {
            allow = 1;
        } else {
            allow = 2;
        }
        break;
    case 0x40C:
    case 0x40D:
    case 0x40E:
        if (leaning != 0) {
            allow = 1;
        } else {
            allow = 2;
        }
        break;
    case 0x43B:
    case 0x43C:
        if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    case 0x40F:
        if (Pl_frame_check_alt(self, 1, 82.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = 1;
        }
        break;
    default:
        if (fn_8026A3A0(self) != 0) {
            allow = 1;
        }
        break;
    }
    return allow;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x8022632C */

/* The motion-state gate of kind-7 attachments. */
extern "C" u8 pl_rig_kind7_allow_get(_PLW* self, u16 motion)
{
    u8 leaning;
    u8 allow = 2;

    leaning = ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

    switch (motion) {
    case 0x3EA:
        if (leaning == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 54.0f, 0.0f) != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 14.0f, 0.0f) != 0) {
            allow = 0;
        }
        break;
    case 0x3EB:
        if (Pl_frame_check_alt(self, 1, 50.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = 0;
        }
        break;
    case 0x3F1:
        if (Pl_frame_check_alt(self, 1, 44.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = 0;
        }
        break;
    case 0x41B:
        if (leaning == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 54.0f, 0.0f) != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 14.0f, 0.0f) != 0) {
            allow = 0;
        }
        break;
    case 0x41C:
        if (leaning != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 50.0f, 0.0f) == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 0.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 1;
        }
        break;
    case 0x421:
        if (leaning != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 48.0f, 0.0f) == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 0.0f, 0.0f) == 1) {
            allow = 0;
        } else {
            allow = 1;
        }
        break;
    case 0x45B:
        if (leaning == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 26.0f, 0.0f) != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 6.0f, 0.0f) != 0) {
            allow = 0;
        } else {
            allow = 2;
        }
        break;
    case 0x466:
        if (leaning == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) != 0) {
            allow = 0;
        }
        break;
    case 0x495:
    case 0x467:
        if (Pl_frame_check_alt(self, 1, 128.0f, 0.0f) != 0) {
            allow = 2;
        } else {
            allow = Pl_frame_check_alt(self, 1, 58.0f, 0.0f) == 0;
        }
        if (leaning == 1) {
            allow = 1;
        }
        break;
    case 0x489:
        if (leaning == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 28.0f, 0.0f) != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 6.0f, 0.0f) != 0) {
            allow = 0;
        } else {
            allow = 2;
        }
        break;
    case 0x494:
        if (leaning == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) != 0) {
            allow = 0;
        }
        break;
    default:
        if (fn_8026A3A0(self) != 0) {
            allow = 1;
        }
        break;
    }
    return allow;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80226728 */

/* The motion-state gate of kind-8 attachments. */
extern "C" u8 pl_rig_kind8_allow_get(_PLW* self, u16 motion)
{
    u8 allow = 2;
    u8 leaning = ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

    switch (motion) {
    case 0x41B:
    case 0x3EA:
        if (leaning == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 56.0f, 0.0f) != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 4.0f, 0.0f) != 0) {
            allow = 0;
        }
        break;
    case 0x421:
    case 0x41C:
    case 0x3F1:
    case 0x3EB:
        if (Pl_frame_check_alt(self, 1, 90.0f, 0.0f) != 0) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 32.0f, 0.0f) != 0) {
            allow = 0;
        } else {
            allow = 1;
        }
        break;
    case 0x48A:
    case 0x458:
        if (leaning == 1) {
            allow = 2;
        } else if (Pl_frame_check_alt(self, 1, 30.0f, 0.0f) != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 1, 2.0f, 0.0f) != 0) {
            allow = 0;
        }
        break;
    case 0x494:
    case 0x462:
        if (Pl_frame_check_alt(self, 6, 110.0f, 0.0f) != 0) {
            allow = 1;
        } else if (Pl_frame_check_alt(self, 6, 34.0f, 0.0f) != 0) {
            allow = 0;
        } else {
            allow = 1;
        }
        break;
    case 0x495:
    case 0x463:
        allow = 0;
        break;
    case 0x496:
    case 0x464:
        if (Pl_frame_check_alt(self, 1, 36.0f, 0.0f) != 0) {
            allow = 1;
        } else {
            allow = 0;
        }
        break;
    case 0x465:
        if (Pl_frame_check_alt(self, 1, 50.0f, 0.0f) != 0) {
            allow = 1;
        } else {
            allow = 0;
        }
        break;
    case 0x497:
        if (Pl_frame_check_alt(self, 1, 52.0f, 0.0f) != 0) {
            allow = 1;
        } else {
            allow = 0;
        }
        break;
    default:
        if (fn_8026A3A0(self) == 1) {
            allow = 1;
        }
        break;
    }
    return allow;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80226A0C */

/* Returns the 24-byte vector pair the motion layer uses for `mode`: 0 indexes the per-index table,
 * 1 is the single shared pair, anything else is null. */
extern "C" PlSeVecPair* pl_rig_lobby_pose_get(_PLW* plw, s32 kind, u8 mode)
{
    switch (mode) {
    case 0:
        return &pl_rig_pose_lobby[kind];
    case 1:
        return pl_rig_pose_lobby_one;
    default:
        return NULL;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80226A4C */

/* Sets the joint adjustment `adj` to the lobby pose of `kind` (`mode` 0 or 1). */
extern "C" void pl_rig_lobby_pose_apply(PlSeJointAdj* adj, _PLW* plw, s32 kind, u8 mode)
{
    PlSeVecPair* pose;

    switch (mode) {
    case 0:
        pose = pl_rig_lobby_pose_get(plw, kind, mode);
        break;
    case 1:
        pose = pl_rig_lobby_pose_get(plw, kind, mode);
        break;
    default:
        return;
    }
    vec3_float_copy(&adj->trans_0x0C.x, &pose->x_0x00);
    vec3_float_copy(&adj->rot_0x18.x, &pose->x_0x0C);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80226AE0 */

/* The weapon pose of `kind` for the motion state `allow` (0 sheathed, 1 drawn, 2 carried, 3 special), with the
 * switch-axe/gunlance alternate forms and the hunting-horn recital motions. */
extern "C" PlSeVecPair* pl_rig_pose_get(_PLW* plw, s32 kind, u8 allow)
{
    switch (plw->field_0x002) {
    case 4:
    case 5:
    case 6: {
        u8 alt = pl_weapon_alt_form_ck(plw);

        if ((s32)allow == 1) {
            return &pl_rig_pose_alt_drawn[alt];
        }
        return &pl_rig_pose_alt[alt];
    }
    case 8:
        switch (allow) {
        default: {
            u16 motion = Get_motion_no(plw);

            if ((u32)(motion - 0x462) <= 3 || (u32)(motion - 0x494) <= 3) {
                return pl_rig_pose_recital;
            }
            return &pl_rig_pose_sheathed[kind];
        }
        case 1:
            return &pl_rig_pose_drawn[kind];
        case 2:
            return &pl_rig_pose_carried[kind];
        case 3:
            return &pl_rig_pose_special[kind];
        }
    default:
        switch (allow) {
        case 3:
            return &pl_rig_pose_special[kind];
        default:
            return &pl_rig_pose_sheathed[kind];
        case 1:
            return &pl_rig_pose_drawn[kind];
        case 2:
            return &pl_rig_pose_carried[kind];
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80226C70 */

/* Sets the joint adjustment `adj` to the weapon pose of `kind` for `allow`, or the alternate form's. */
extern "C" void pl_rig_pose_apply(PlSeJointAdj* adj, _PLW* plw, s32 kind, u8 allow)
{
    PlSeVecPair* pose;

    if (pl_item_page2_ck(plw) == 0) {
        switch (allow) {
        default:
            pose = &pl_rig_pose_sheathed[kind];
            break;
        case 1:
            pose = &pl_rig_pose_drawn[kind];
            break;
        case 2:
            pose = &pl_rig_pose_carried[kind];
            break;
        case 3:
            pose = &pl_rig_pose_special[kind];
            break;
        }
    } else {
        u8 alt = pl_weapon_alt_form_ck(plw);

        if ((s32)allow == 1) {
            pose = &pl_rig_pose_alt_drawn[alt];
        } else {
            pose = &pl_rig_pose_alt[alt];
        }
    }
    vec3_float_copy(&adj->trans_0x0C.x, &pose->x_0x00);
    vec3_float_copy(&adj->rot_0x18.x, &pose->x_0x0C);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80226D90 */

/* Runs attachment `attach`'s per-kind motion pose for the actor's current motion. */
extern "C" void pl_rig_attach_motion_pose(PlSeRig* rig, PlSeAttach* attach, u32 kind)
{
    _PLW* plw = rig->plw;
    u16 motion = Get_motion_no(plw);

    switch (kind) {
    case 0:
        pl_rig_kind0_pose(rig, plw, attach, motion);
        break;
    case 1:
        pl_rig_kind1_pose(rig, plw, attach, motion);
        break;
    case 9:
        pl_rig_kind9_pose(rig, plw, attach, motion);
        break;
    case 7:
        pl_rig_kind7_pose(rig, plw, attach, motion);
        break;
    case 4:
        pl_rig_kind4_pose(rig, plw, attach, motion);
        break;
    case 8:
        pl_rig_kind8_pose(rig, plw, attach, motion);
        break;
    case 3:
        pl_rig_kind3_pose(rig, plw, attach, motion);
        break;
    case 2:
        pl_rig_kind2_pose(rig, plw, attach, motion);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80226EC4 */

/* Raises the kind-4 attachment: its spin (unless in the alternate form) and its stretched scale. */
static inline void pl_rig_kind4_raise(PlSeAttach* attach, u8 alt)
{
    if (alt == 0) {
        attach->adj_0x014[2].rot_0x18.x = 180.0f;
    }
    attach->adj_0x014[3].scale_0x00.x = 1.0f;
    attach->adj_0x014[3].scale_0x00.y = 0.1f;
    attach->adj_0x014[3].scale_0x00.z = 2.5f;
}

/* Lowers the kind-4 attachment: the half-raised scale while the actor holds it up, else unit scale. */
static inline void pl_rig_kind4_lower(_PLW* plw, PlSeAttach* attach)
{
    if (plw->field_0x274 != 0) {
        attach->adj_0x014[3].scale_0x00.x = 1.0f;
        attach->adj_0x014[3].scale_0x00.y = 0.75f;
        attach->adj_0x014[3].scale_0x00.z = 2.5f;
    } else {
        attach->adj_0x014[3].scale_0x00.z = 1.0f;
        attach->adj_0x014[3].scale_0x00.y = 1.0f;
        attach->adj_0x014[3].scale_0x00.x = 1.0f;
    }
}

/* Poses the kind-4 attachment for motion `motion` from its key tables (outside the lobby), then mirrors its scale
 * onto the next joint. */
extern "C" void pl_rig_kind4_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 unused_c;
    u8 alt;
    s32 tail = 0;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    alt = pl_weapon_alt_form_ck(plw);
    if (system_lobby_flag_get() != 2) {
        u8 leaning = ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

        switch (motion) {
        case 0x49C:
        case 0x46A:
        case 0x41B:
        case 0x3EA:
            if (leaning == 0) {
                if (alt == 0) {
                    attach->adj_0x014[2].rot_0x18.x = pl_rig_key_get(plw, pl_rig_k4_3ea_spin);
                }
                if (plw->field_0x269 != 0) {
                    pl_rig_key_pair_get(plw, pl_rig_k4_3ea_scale_alt, &attach->adj_0x014[3].scale_0x00.z,
                                &attach->adj_0x014[3].scale_0x00.y);
                } else {
                    pl_rig_key_pair_get(plw, pl_rig_k4_3ea_scale, &attach->adj_0x014[3].scale_0x00.z,
                                &attach->adj_0x014[3].scale_0x00.y);
                }
            } else {
                tail = 1;
            }
            break;
        case 0x421:
        case 0x41C:
        case 0x3EB:
            if (leaning == 0) {
                if (alt == 0) {
                    attach->adj_0x014[2].rot_0x18.x = pl_rig_key_get(plw, pl_rig_k4_3eb_spin);
                }
                pl_rig_key_pair_get(plw, pl_rig_k4_3eb_scale, &attach->adj_0x014[3].scale_0x00.z,
                            &attach->adj_0x014[3].scale_0x00.y);
            } else {
                tail = 2;
            }
            break;
        case 0x3F1:
            if (leaning == 0) {
                pl_rig_key_pair_get(plw, pl_rig_k4_3f1_scale, &attach->adj_0x014[3].scale_0x00.z,
                            &attach->adj_0x014[3].scale_0x00.y);
                if (alt == 0) {
                    attach->adj_0x014[2].rot_0x18.x = pl_rig_key_get(plw, pl_rig_k4_3f1_spin);
                }
            } else {
                tail = 2;
            }
            break;
        case 0x49E:
        case 0x49D:
        case 0x46C:
        case 0x46B:
        case 0x43D:
        case 0x43A:
        case 0x410:
        case 0x40B:
            if (leaning == 0) {
                if (plw->field_0x269 != 0) {
                    pl_rig_key_pair_get(plw, pl_rig_k4_40b_scale_alt, &attach->adj_0x014[3].scale_0x00.z,
                                &attach->adj_0x014[3].scale_0x00.y);
                } else {
                    pl_rig_key_pair_get(plw, pl_rig_k4_40b_scale, &attach->adj_0x014[3].scale_0x00.z,
                                &attach->adj_0x014[3].scale_0x00.y);
                }
            } else {
                tail = 1;
            }
            break;
        case 0x485:
        case 0x47E:
        case 0x454:
        case 0x44D:
            if (leaning == 0) {
                if (plw->field_0x269 != 0) {
                    pl_rig_key_pair_get(plw, pl_rig_k4_44d_scale_alt, &attach->adj_0x014[3].scale_0x00.z,
                                &attach->adj_0x014[3].scale_0x00.y);
                } else {
                    pl_rig_key_pair_get(plw, pl_rig_k4_44d_scale, &attach->adj_0x014[3].scale_0x00.z,
                                &attach->adj_0x014[3].scale_0x00.y);
                }
            } else {
                tail = 2;
            }
            break;
        case 0x481:
        case 0x450:
            if (leaning == 0) {
                pl_rig_key_pair_get(plw, pl_rig_k4_450_scale, &attach->adj_0x014[3].scale_0x00.z,
                            &attach->adj_0x014[3].scale_0x00.y);
            } else {
                tail = 2;
            }
            break;
        case 0x482:
        case 0x451:
            if (leaning == 0) {
                if (plw->field_0x269 != 0) {
                    pl_rig_key_pair_get(plw, pl_rig_k4_451_scale_alt, &attach->adj_0x014[3].scale_0x00.z,
                                &attach->adj_0x014[3].scale_0x00.y);
                } else {
                    pl_rig_key_pair_get(plw, pl_rig_k4_451_scale, &attach->adj_0x014[3].scale_0x00.z,
                                &attach->adj_0x014[3].scale_0x00.y);
                }
            } else {
                tail = 2;
            }
            break;
        case 0x40F:
            if (leaning == 0) {
                pl_rig_key_pair_get(plw, pl_rig_k4_40f_scale, &attach->adj_0x014[3].scale_0x00.z,
                            &attach->adj_0x014[3].scale_0x00.y);
                if (alt == 0) {
                    attach->adj_0x014[2].rot_0x18.x = pl_rig_key_get(plw, pl_rig_k4_40f_spin);
                }
            } else {
                tail = 2;
            }
            break;
        default:
            if (fn_8026A3A0(plw) == 0 || attach->allow_0x11 == 2) {
                tail = 1;
            } else {
                tail = 2;
            }
            break;
        }
        if (tail == 1) {
            pl_rig_kind4_raise(attach, alt);
        } else if (tail == 2) {
            pl_rig_kind4_lower(plw, attach);
        }
    }
    attach->adj_0x014[4].scale_0x00.x = attach->adj_0x014[3].scale_0x00.y;
    attach->adj_0x014[4].scale_0x00.y = attach->adj_0x014[3].scale_0x00.x;
    attach->adj_0x014[4].scale_0x00.z = attach->adj_0x014[3].scale_0x00.z;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80227354 */

/* Poses the kind-0 attachment for motion `motion`: its third joint's roll and fourth joint's scale from the motion's
 * key tables, the charge scale of its sixth joint, or the resting pose; the fifth joint's depth follows the fourth. */
extern "C" void pl_rig_kind0_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 unused_c;
    nw4r::math::VEC3* scale;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    scale = &attach->adj_0x014[3].scale_0x00;
    if (system_lobby_flag_get() == 2) {
        scale->x = 2.6f;
        scale->y = 1.0f;
        scale->z = 2.5f;
    } else {
        u8 leaning;
        s32 charge = 0;

        if (fn_8026A3A0(plw) != 0 && attach->allow_0x11 != 2) {
            scale->x = 2.6f;
            scale->y = 1.0f;
            scale->z = 2.5f;
        }
        leaning = ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;
        switch (motion) {
        case 0x164:
        case 0x139:
            if (leaning == 0) {
                attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_139_roll);
                pl_rig_key_pair_get(plw, pl_rig_k0_139_scale, &scale->x, &scale->z);
            } else {
                attach->adj_0x014[2].trans_0x0C.z = 100.0f;
                scale->x = 1.0f;
                scale->y = 1.0f;
                scale->z = 1.0f;
            }
            break;
        case 0x41B:
        case 0x3EA:
            if (leaning == 0) {
                if (motion == 0x3EA) {
                    attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_3ea_roll);
                    pl_rig_key_pair_get(plw, pl_rig_k0_3ea_scale, &scale->x, &scale->z);
                } else {
                    attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_3ea_roll);
                    pl_rig_key_pair_get(plw, pl_rig_k0_41b_scale, &scale->x, &scale->z);
                }
            } else {
                attach->adj_0x014[2].trans_0x0C.z = 100.0f;
                scale->x = 1.0f;
                scale->y = 1.0f;
                scale->z = 1.0f;
            }
            break;
        case 0x46B:
        case 0x450:
            if (leaning == 0) {
                attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_450_roll);
                pl_rig_key_pair_get(plw, pl_rig_k0_450_scale, &scale->x, &scale->z);
            } else {
                attach->adj_0x014[2].trans_0x0C.z = 100.0f;
                scale->x = 1.0f;
                scale->y = 1.0f;
                scale->z = 1.0f;
            }
            break;
        case 0x482:
        case 0x48F:
            if (leaning == 0) {
                attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_450_roll);
                pl_rig_key_pair_get(plw, pl_rig_k0_482_scale, &scale->x, &scale->z);
            } else {
                attach->adj_0x014[2].trans_0x0C.z = 100.0f;
                scale->x = 1.0f;
                scale->y = 1.0f;
                scale->z = 1.0f;
            }
            break;
        case 0x421:
        case 0x41C:
        case 0x3F1:
        case 0x3EB:
            if (leaning == 0) {
                switch (motion) {
                case 0x3EB:
                    attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_3eb_roll);
                    pl_rig_key_pair_get(plw, pl_rig_k0_3eb_scale, &scale->x, &scale->z);
                    break;
                case 0x3F1:
                    attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_3f1_roll);
                    pl_rig_key_pair_get(plw, pl_rig_k0_3f1_scale, &scale->x, &scale->z);
                    break;
                case 0x41C:
                    attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_41c_roll);
                    pl_rig_key_pair_get(plw, pl_rig_k0_41c_scale, &scale->x, &scale->z);
                    break;
                case 0x421:
                    attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k0_421_roll);
                    pl_rig_key_pair_get(plw, pl_rig_k0_421_scale, &scale->x, &scale->z);
                    break;
                }
            } else {
                attach->adj_0x014[2].trans_0x0C.z = 0.0f;
                scale->x = 2.6f;
                scale->y = 1.0f;
                scale->z = 2.5f;
            }
            break;
        case 0x468:
            if (leaning != 0 || Pl_frame_check_alt(plw, 2, 50.0f, 0.0f) == 1) {
                charge = 1;
            }
            break;
        case 0x46F:
            if (leaning != 0 || Pl_frame_check_alt(plw, 2, 56.0f, 0.0f) == 1) {
                charge = 1;
            }
            break;
        case 0x48E:
            if (leaning != 0 || Pl_frame_check_alt(plw, 2, 54.0f, 0.0f) == 1) {
                charge = 1;
            }
            break;
        case 0x494:
            if (leaning != 0 || Pl_frame_check_alt(plw, 2, 70.0f, 0.0f) == 1) {
                charge = 1;
            }
            break;
        case 0x492:
        case 0x47E:
        case 0x46D:
        case 0x44D:
            charge = 1;
            break;
        default:
            if (fn_8026A3A0(plw) == 0 || attach->allow_0x11 == 2) {
                attach->adj_0x014[2].trans_0x0C.z = 100.0f;
            }
            break;
        }
        if (charge != 0) {
            switch (pl_charge_stage_get(plw)) {
            case 0:
                attach->adj_0x014[5].scale_0x00.x = 1.0f;
                break;
            case 1:
                attach->adj_0x014[5].scale_0x00.x = 1.5f;
                break;
            case 2:
                attach->adj_0x014[5].scale_0x00.x = 2.2f;
                break;
            default:
                attach->adj_0x014[5].scale_0x00.x = 3.0f;
                break;
            }
        }
    }
    attach->adj_0x014[4].scale_0x00.z = 1.0f + 1.2f * (scale->z - 1.0f);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80227900 */

/* Poses the kind-1 attachment for motion `motion`: its third joint's depth scale from the motion's key table, or the
 * resting/stretched scale. */
extern "C" void pl_rig_kind1_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 unused_c;
    u8 leaning;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    if (system_lobby_flag_get() == 2) {
        attach->adj_0x014[2].scale_0x00.z = 3.0f;
        return;
    }
    leaning = ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;
    switch (motion) {
    case 0x42E:
    case 0x3FA:
    case 0x41B:
    case 0x3EA:
        if (leaning == 0) {
            attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k1_3ea_scale);
            return;
        }
        attach->adj_0x014[2].scale_0x00.z = 1.0f;
        return;
    case 0x421:
    case 0x3F1:
    case 0x3EB:
        if (leaning == 0) {
            attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k1_3eb_scale);
            return;
        }
        attach->adj_0x014[2].scale_0x00.z = 3.0f;
        return;
    case 0x41C:
        if (leaning == 0) {
            attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k1_41c_scale);
            return;
        }
        attach->adj_0x014[2].scale_0x00.z = 3.0f;
        return;
    case 0x46A:
    case 0x488:
        if (leaning == 0) {
            attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k1_46a_scale);
            return;
        }
        attach->adj_0x014[2].scale_0x00.z = 1.0f;
        return;
    default:
        if (fn_8026A3A0(plw) == 0 || attach->allow_0x11 == 2) {
            attach->adj_0x014[2].scale_0x00.z = 1.0f;
            return;
        }
        attach->adj_0x014[2].scale_0x00.z = 3.0f;
        return;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80227AF0 */

/* Poses the kind-9 attachment for motion `motion`: its third joint's scale from the motion's key table, unit scale
 * or the stretched scale. */
extern "C" void pl_rig_kind9_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 unused_c;
    nw4r::math::VEC3* scale;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    scale = &attach->adj_0x014[2].scale_0x00;
    if (system_lobby_flag_get() != 2) {
        u8 leaning = ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

        switch (motion) {
        case 0x3EA:
            if (leaning == 0) {
                pl_rig_key_triple_get(plw, pl_rig_k9_3ea_scale, &scale->x, &scale->y, &scale->z);
            }
            return;
        case 0x3FA:
            if (leaning == 0) {
                pl_rig_key_triple_get(plw, pl_rig_k9_3fa_scale, &scale->x, &scale->y, &scale->z);
            }
            return;
        case 0x46A:
        case 0x488:
            if (leaning == 0) {
                pl_rig_key_triple_get(plw, pl_rig_k9_46a_scale, &scale->x, &scale->y, &scale->z);
            }
            return;
        case 0x41B:
            if (leaning == 0) {
                pl_rig_key_triple_get(plw, pl_rig_k9_41b_scale, &scale->x, &scale->y, &scale->z);
            }
            return;
        case 0x42E:
            if (leaning == 0) {
                pl_rig_key_triple_get(plw, pl_rig_k9_42e_scale, &scale->x, &scale->y, &scale->z);
            }
            return;
        case 0x421:
        case 0x3F1:
        case 0x3EB:
            if (leaning == 0) {
                pl_rig_key_triple_get(plw, pl_rig_k9_3eb_scale, &scale->x, &scale->y, &scale->z);
                return;
            }
            scale->x = 1.5f;
            scale->y = 4.0f;
            scale->z = 1.5f;
            return;
        case 0x41C:
            if (leaning == 0) {
                pl_rig_key_triple_get(plw, pl_rig_k9_41c_scale, &scale->x, &scale->y, &scale->z);
                return;
            }
            scale->x = 1.5f;
            scale->y = 4.0f;
            scale->z = 1.5f;
            return;
        default:
            if (fn_8026A3A0(plw) == 0 || attach->allow_0x11 == 2) {
                scale->x = 1.0f;
                scale->y = 1.0f;
                scale->z = 1.0f;
                return;
            }
            break;
        }
    }
    scale->x = 1.5f;
    scale->y = 4.0f;
    scale->z = 1.5f;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80227D8C */

/* Poses the kind-7 attachment for motion `motion`: its root pose, the visibility of its seventh node, the stagger
 * scale and the span its key tables give (variant 9 widens its fifth joint with the span). */
extern "C" void pl_rig_kind7_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 unused_c;
    PlSeJointAdj* root;
    s32 variant;
    f32 span;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    root = &attach->adj_0x014[5];
    if (system_lobby_flag_get() == 2) {
        pl_rig_lobby_pose_apply(root, plw, 7, 1);
        attach->adj_0x014[4].scale_0x00.x = 1.8f;
        attach->adj_0x014[4].scale_0x00.z = 2.0f;
    } else {
        pl_rig_pose_apply(root, plw, 7, 2);
    }
    variant = equip_variant_get(&plw->equipB);
    if (pl_rig_res_mdl(attach->model_0x08)->GetResNodeNumEntries() > 6) {
        if (attach->allow_0x11 == 2 && system_lobby_flag_get() != 2) {
            attach->model_0x08->setVisibility(6, false);
        } else {
            attach->model_0x08->setVisibility(6, true);
        }
    }
    if (system_lobby_flag_get() == 2) {
        span = 1.0f;
    } else {
        u8 leaning;

        if (attach->allow_0x11 != 2) {
            root->scale_0x00.z = 0.8f;
            root->scale_0x00.y = 0.8f;
            root->scale_0x00.x = 0.8f;
        }
        leaning = ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;
        if (fn_8026A3A0(plw) == 0 || attach->allow_0x11 == 2) {
            attach->adj_0x014[3].scale_0x00.x = 1.0f;
            attach->adj_0x014[3].scale_0x00.z = 1.0f;
            attach->adj_0x014[4].scale_0x00.x = 1.0f;
            attach->adj_0x014[4].scale_0x00.z = 1.0f;
        } else {
            switch ((u8)pl_stagger_level_get(plw)) {
            default:
                attach->adj_0x014[3].scale_0x00.x = 1.0f;
                attach->adj_0x014[3].scale_0x00.z = 1.0f;
                break;
            case 1:
                attach->adj_0x014[3].scale_0x00.x = 1.1f;
                attach->adj_0x014[3].scale_0x00.z = 1.1f;
                break;
            case 2:
                attach->adj_0x014[3].scale_0x00.x = 1.3f;
                attach->adj_0x014[3].scale_0x00.z = 1.3f;
                break;
            case 3:
                attach->adj_0x014[3].scale_0x00.x = 1.5f;
                attach->adj_0x014[3].scale_0x00.z = 1.5f;
                break;
            }
            attach->adj_0x014[4].scale_0x00.x = 1.8f;
            attach->adj_0x014[4].scale_0x00.z = 2.0f;
        }
        span = 0.75f;
        switch (motion) {
        case 0x164:
        case 0x139:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k7_139_scale);
                span = pl_rig_key_get(plw, pl_rig_k7_139_span);
            } else {
                attach->adj_0x014[2].scale_0x00.z = 0.75f;
            }
            break;
        case 0x41B:
        case 0x3EA:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k7_3ea_scale);
                span = pl_rig_key_get(plw, pl_rig_k7_3ea_span);
            } else {
                attach->adj_0x014[2].scale_0x00.z = 0.75f;
            }
            break;
        case 0x421:
        case 0x41C:
        case 0x3F1:
        case 0x3EB:
            if (leaning == 0) {
                if (motion == 0x3EB || motion == 0x41C) {
                    attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k7_3eb_scale);
                    span = pl_rig_key_get(plw, pl_rig_k7_3eb_span);
                } else {
                    attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k7_3f1_scale);
                    span = pl_rig_key_get(plw, pl_rig_k7_3f1_span);
                }
            } else {
                span = 1.0f;
                attach->adj_0x014[2].scale_0x00.z = span;
            }
            break;
        case 0x45B:
        case 0x489:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k7_45b_scale);
                span = pl_rig_key_get(plw, pl_rig_k7_45b_span);
            } else {
                attach->adj_0x014[2].scale_0x00.z = 0.75f;
            }
            break;
        default:
            if (fn_8026A3A0(plw) == 0 || attach->allow_0x11 == 2) {
                span = 0.75f;
                attach->adj_0x014[2].scale_0x00.z = span;
            } else {
                span = 1.0f;
                attach->adj_0x014[2].scale_0x00.z = span;
            }
            break;
        }
    }
    if (variant == 9) {
        attach->adj_0x014[4].scale_0x00.x = 0.8f + 4.0f * (span - 0.75f);
        attach->adj_0x014[4].scale_0x00.z = 2.0f;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80228190 */

/* Interpolates the scale, lift and tilt of key table `keys` at the actor's motion frame into joint adjustment `adj`
 * (the last key's once the table's end mark is reached); the scale is uniform across x and z. */
extern "C" void pl_rig_key_scale_lerp(_PLW* plw, PlRigScaleKey* keys, PlSeJointAdj* adj)
{
    for (;; keys++) {
        if (-1.0f == keys->frame_0x00) {
            adj->scale_0x00.x = keys->scale_0x04;
            adj->rot_0x18.y = keys->lift_0x08;
            adj->trans_0x0C.x = keys->tilt_x_0x0C;
            adj->trans_0x0C.z = keys->tilt_z_0x10;
            break;
        }
        if (Pl_frame_check_alt(plw, 5, keys->frame_0x00, keys[1].frame_0x00) != 0) {
            f32 t = (pl_rig_get_motion_frame(plw) - keys->frame_0x00) / (keys[1].frame_0x00 - keys->frame_0x00);

            adj->scale_0x00.x = keys->scale_0x04 + t * (keys[1].scale_0x04 - keys->scale_0x04);
            adj->rot_0x18.y = keys->lift_0x08 + t * (keys[1].lift_0x08 - keys->lift_0x08);
            adj->trans_0x0C.x = keys->tilt_x_0x0C + t * (keys[1].tilt_x_0x0C - keys->tilt_x_0x0C);
            adj->trans_0x0C.z = keys->tilt_z_0x10 + t * (keys[1].tilt_z_0x10 - keys->tilt_z_0x10);
            break;
        }
    }
    adj->scale_0x00.z = adj->scale_0x00.x;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802282C0 */

/* Interpolates the lift and tilt of key table `keys` at the actor's motion frame into joint adjustment `adj`. */
extern "C" void pl_rig_key_tilt_lerp(_PLW* plw, PlRigTiltKey* keys, PlSeJointAdj* adj)
{
    for (;; keys++) {
        if (-1.0f == keys->frame_0x00) {
            adj->rot_0x18.y = keys->lift_0x04;
            adj->trans_0x0C.x = keys->tilt_x_0x08;
            adj->trans_0x0C.z = keys->tilt_z_0x0C;
            return;
        }
        if (Pl_frame_check_alt(plw, 5, keys->frame_0x00, keys[1].frame_0x00) != 0) {
            f32 t = (pl_rig_get_motion_frame(plw) - keys->frame_0x00) / (keys[1].frame_0x00 - keys->frame_0x00);

            adj->rot_0x18.y = keys->lift_0x04 + t * (keys[1].lift_0x04 - keys->lift_0x04);
            adj->trans_0x0C.x = keys->tilt_x_0x08 + t * (keys[1].tilt_x_0x08 - keys->tilt_x_0x08);
            adj->trans_0x0C.z = keys->tilt_z_0x0C + t * (keys[1].tilt_z_0x0C - keys->tilt_z_0x0C);
            return;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802283C8 */

/* Poses the kind-8 attachment for motion `motion` from its scale, tilt and depth key tables, or one of its fixed
 * poses: resting, held up, or played. */
extern "C" void pl_rig_kind8_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 unused_c;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    if (system_lobby_flag_get() != 2) {
        PlSeJointAdj* grip = &attach->adj_0x014[3];
        PlSeJointAdj* head = &attach->adj_0x014[4];
        u8 leaning = ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;
        s32 pose = 0;

        switch (motion) {
        case 0x164:
        case 0x139:
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_139_grip, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_139_head, head);
                attach->adj_0x014[5].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k8_139_depth);
                attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k8_139_lift);
                return;
            }
            pose = 1;
            break;
        case 0x41B:
        case 0x3EA:
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_3ea_grip, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_3ea_head, head);
                attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k8_3ea_lift);
                return;
            }
            pose = 1;
            break;
        case 0x421:
        case 0x41C:
        case 0x3F1:
        case 0x3EB:
            if (plw->field_0x5C6 == 0) {
                if (leaning == 0) {
                    pl_rig_key_scale_lerp(plw, pl_rig_k8_3eb_grip, grip);
                    pl_rig_key_tilt_lerp(plw, pl_rig_k8_3eb_head, head);
                    attach->adj_0x014[5].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k8_3eb_depth);
                    attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k8_3eb_lift);
                }
                return;
            }
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_3eb_grip_pair, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_3eb_head_pair, head);
                attach->adj_0x014[5].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k8_3eb_depth);
                attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k8_3eb_lift);
                return;
            }
            pose = 2;
            break;
        case 0x48A:
        case 0x458:
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_458_grip, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_458_head, head);
                attach->adj_0x014[2].trans_0x0C.z = pl_rig_key_get(plw, pl_rig_k8_458_lift);
                return;
            }
            pose = 1;
            break;
        case 0x494:
        case 0x462:
            if (leaning != 0) {
                if (Pl_act_ck(plw, 4, 25) == 1 || Pl_act_ck(plw, 4, 75) == 1) {
                    pose = 3;
                } else {
                    pose = 2;
                }
                break;
            }
            pl_rig_key_scale_lerp(plw, pl_rig_k8_462_grip, grip);
            head->rot_0x18.y = -180.0f;
            head->trans_0x0C.x = 0.0f;
            head->trans_0x0C.y = 0.0f;
            head->trans_0x0C.z = -20.0f;
            attach->adj_0x014[5].scale_0x00.z = 3.0f;
            return;
        case 0x496:
        case 0x464:
            pl_rig_key_scale_lerp(plw, pl_rig_k8_464_grip, grip);
            pose = 4;
            break;
        case 0x497:
        case 0x465:
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_465_grip, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_465_head, head);
                return;
            }
            pose = 3;
            break;
        case 0x495:
        case 0x463:
            pose = 3;
            break;
        case 0x4A6:
        case 0x474:
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_474_grip, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_474_head, head);
            }
            return;
        case 0x4A7:
        case 0x475:
            if (Pl_act_ck(plw, 4, 70) != 0 || Pl_act_ck(plw, 4, 71) != 0) {
                return;
            }
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_475_grip, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_475_head, head);
                return;
            }
            pose = 2;
            break;
        case 0x4A8:
        case 0x476:
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_476_grip, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_476_head, head);
            }
            return;
        case 0x4A9:
        case 0x477:
            if (leaning == 0) {
                pl_rig_key_scale_lerp(plw, pl_rig_k8_477_grip, grip);
                pl_rig_key_tilt_lerp(plw, pl_rig_k8_477_head, head);
                return;
            }
            pose = 2;
            break;
        default:
            if (fn_8026A3A0(plw) == 0 || attach->allow_0x11 == 2) {
                pose = 1;
            } else if (plw->field_0x5C6 == 1) {
                pose = 2;
            }
            break;
        }
        switch (pose) {
        case 1:
            attach->adj_0x014[2].trans_0x0C.z = 100.0f;
            head->rot_0x18.y = -10.0f;
            head->trans_0x0C.x = 5.0f;
            head->trans_0x0C.y = 0.0f;
            head->trans_0x0C.z = -53.0f;
            grip->scale_0x00.x = 0.9f;
            grip->scale_0x00.y = 1.0f;
            grip->scale_0x00.z = 0.9f;
            grip->trans_0x0C.x = 8.0f;
            grip->trans_0x0C.y = 0.0f;
            grip->trans_0x0C.z = -133.0f;
            break;
        case 2:
            head->rot_0x18.y = -180.0f;
            head->trans_0x0C.x = 0.0f;
            head->trans_0x0C.y = 0.0f;
            head->trans_0x0C.z = -20.0f;
            grip->scale_0x00.x = 0.9f;
            grip->scale_0x00.y = 1.0f;
            grip->scale_0x00.z = 0.9f;
            grip->trans_0x0C.x = 8.0f;
            grip->trans_0x0C.y = 0.0f;
            grip->trans_0x0C.z = -133.0f;
            attach->adj_0x014[5].scale_0x00.z = 3.0f;
            break;
        case 3:
            grip->rot_0x18.y = -27.0f;
            grip->scale_0x00.x = 0.9f;
            grip->scale_0x00.y = 1.0f;
            grip->scale_0x00.z = 0.9f;
            grip->trans_0x0C.x = 6.0f;
            grip->trans_0x0C.y = 0.0f;
            grip->trans_0x0C.z = -141.0f;
            /* fall through */
        case 4:
            head->rot_0x18.y = -180.0f;
            head->trans_0x0C.x = 0.0f;
            head->trans_0x0C.y = 0.0f;
            head->trans_0x0C.z = -20.0f;
            attach->adj_0x014[5].scale_0x00.z = 3.0f;
            break;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802289F8 */

/* Poses the kind-3 attachment for motion `motion`: its third joint's depth scale from the motion's key table, or the
 * spin the actor's frame counter turns for the spinning motions. */
extern "C" void pl_rig_kind3_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 unused_c;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    if (system_lobby_flag_get() != 2) {
        u8 leaning = ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 > 0.0f;

        switch (motion) {
        case 0x3EA:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k3_3ea_scale);
                return;
            }
            attach->adj_0x014[2].scale_0x00.z = 0.45f;
            return;
        case 0x41B:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k3_41b_scale);
                return;
            }
            attach->adj_0x014[2].scale_0x00.z = 0.45f;
            return;
        case 0x3F2:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k3_3f2_scale);
                return;
            }
            attach->adj_0x014[2].scale_0x00.z = 0.45f;
            return;
        case 0x41C:
        case 0x3EB:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k3_3eb_scale);
                return;
            }
            attach->adj_0x014[2].scale_0x00.z = 1.0f;
            return;
        case 0x421:
        case 0x3F1:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k3_3f1_scale);
                return;
            }
            attach->adj_0x014[2].scale_0x00.z = 1.0f;
            return;
        case 0x480:
        case 0x44E:
            attach->adj_0x014[3].rot_0x18.z = 360.0f * (f32)(s32)((plw->frame_0x020 & 0xF) << 12) / 65536.0f;
            return;
        case 0x48A:
        case 0x458:
            if (Pl_frame_check_alt(plw, 2, 50.0f, 0.0f) == 1) {
                attach->adj_0x014[3].rot_0x18.z = 360.0f * (f32)(s32)((plw->frame_0x020 & 7) << 13) / 65536.0f;
                return;
            }
            if (Pl_frame_check_alt(plw, 2, 90.0f, 0.0f) == 1) {
                attach->adj_0x014[3].rot_0x18.z =
                    360.0f * (f32)(s32)((plw->frame_0x020 & 0x1F) << 11) / 65536.0f;
                return;
            }
            break;
        case 0x42E:
        case 0x3FA:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k3_3fa_scale);
                return;
            }
            attach->adj_0x014[2].scale_0x00.z = 0.45f;
            return;
        case 0x49C:
            if (leaning == 0) {
                attach->adj_0x014[2].scale_0x00.z = pl_rig_key_get(plw, pl_rig_k3_49c_scale);
                return;
            }
            attach->adj_0x014[2].scale_0x00.z = 0.45f;
            return;
        default:
            if (fn_8026A3A0(plw) == 0 || attach->allow_0x11 == 2) {
                attach->adj_0x014[2].scale_0x00.x = 1.0f;
                attach->adj_0x014[2].scale_0x00.y = 1.0f;
                attach->adj_0x014[2].scale_0x00.z = 0.45f;
            }
            break;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80228D88 */

/* Spins joint adjustment `adj` with the actor's frame counter at the rate of charge stage `stage` (0xFF reads the
 * actor's own stage). */
extern "C" void pl_rig_charge_spin_set(_PLW* plw, PlSeJointAdj* adj, u8 stage)
{
    if (stage == 0xFF) {
        switch (pl_charge_stage2_get(plw)) {
        case 0:
            stage = 0;
            break;
        case 1:
            stage = 1;
            break;
        default:
            stage = 2;
            break;
        }
    }
    switch (stage) {
    case 1:
        adj->rot_0x18.x = 360.0f * (f32)(s32)((plw->frame_0x020 & 0x1F) << 11) / 65536.0f;
        break;
    case 2:
        adj->rot_0x18.x = 360.0f * (f32)(s32)((plw->frame_0x020 & 0xF) << 12) / 65536.0f;
        break;
    case 3:
        adj->rot_0x18.x = 360.0f * (f32)(s32)((plw->frame_0x020 & 7) << 13) / 65536.0f;
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80228ED4 */

/* Poses the kind-2 attachment for motion `motion`: its third joint's scale and the fourth joint's lift from the
 * motion's key tables, the charge spin and cycle, or the charged stretch. */
extern "C" void pl_rig_kind2_pose(PlSeRig* rig, _PLW* plw, PlSeAttach* attach, u16 motion)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::VEC3 unused_c;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    if (system_lobby_flag_get() != 2) {
        PlSeJointAdj* spin = &attach->adj_0x014[4];
        f32 lift = 0.0f;
        u8 leaning = ((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model.field_0x44 > lift;
        f32 scale = 1.0f;
        s32 charged = 0;

        switch (motion) {
        case 0x3F3:
        case 0x3F4:
        case 0x3F5:
        case 0x424:
        case 0x425:
        case 0x426:
        case 0x42C:
        case 0x42D:
            break;
        case 0x46B:
        case 0x46D:
        case 0x483:
        case 0x484:
            pl_rig_charge_spin_set(plw, spin, 0xFF);
            if (pl_charge_stage2_get(plw) <= 2) {
                if (pl_charge_stage2_get(plw) == 2) {
                    switch (plw->charge_gauge_0x310 % pl_charge_cycle_get(plw)) {
                    case 0:
                        scale = 1.0f;
                        lift = 0.0f;
                        break;
                    case 1:
                        scale = 3.2f;
                        lift = 8.0f;
                        break;
                    case 2:
                        scale = 2.9f;
                        lift = -85.0f;
                        break;
                    case 3:
                        scale = 3.0f;
                        lift = -58.0f;
                        break;
                    default:
                        charged = 1;
                        break;
                    }
                }
            } else {
                charged = 1;
            }
            break;
        case 0x450:
            if (leaning == 0) {
                scale = pl_rig_key_get(plw, pl_rig_k2_450_scale);
                lift = pl_rig_key_get(plw, pl_rig_k2_450_lift);
            } else {
                charged = 1;
            }
            break;
        case 0x452:
            if (leaning == 0) {
                scale = pl_rig_key_get(plw, pl_rig_k2_452_scale);
                lift = pl_rig_key_get(plw, pl_rig_k2_452_lift);
            } else {
                charged = 1;
            }
            break;
        case 0x457:
            if (leaning == 0) {
                scale = pl_rig_key_get(plw, pl_rig_k2_457_scale);
                lift = pl_rig_key_get(plw, pl_rig_k2_457_lift);
            } else {
                charged = 1;
            }
            break;
        case 0x470:
            if (leaning == 0) {
                scale = pl_rig_key_get(plw, pl_rig_k2_470_scale);
                lift = pl_rig_key_get(plw, pl_rig_k2_470_lift);
            } else {
                charged = 1;
            }
            break;
        case 0x481:
            if (leaning == 0) {
                scale = pl_rig_key_get(plw, pl_rig_k2_481_scale);
                lift = pl_rig_key_get(plw, pl_rig_k2_481_lift);
            } else {
                charged = 1;
            }
            break;
        case 0x487:
            if (leaning == 0) {
                scale = pl_rig_key_get(plw, pl_rig_k2_487_scale);
                lift = pl_rig_key_get(plw, pl_rig_k2_487_lift);
            } else {
                charged = 1;
            }
            break;
        case 0x48B:
            if (leaning == 0) {
                scale = pl_rig_key_get(plw, pl_rig_k2_48b_scale);
                lift = pl_rig_key_get(plw, pl_rig_k2_48b_lift);
            } else {
                charged = 1;
            }
            break;
        case 0x48C:
            if (leaning == 0) {
                scale = pl_rig_key_get(plw, pl_rig_k2_48c_scale);
                lift = pl_rig_key_get(plw, pl_rig_k2_48c_lift);
            } else {
                charged = 1;
            }
            break;
        default:
            charged = 1;
            break;
        }
        if (charged != 0 && pl_charge_stage2_get(plw) >= 2) {
            scale = 3.0f;
            lift = -60.0f;
        }
        attach->adj_0x014[2].scale_0x00.x = scale;
        attach->adj_0x014[2].scale_0x00.y = scale;
        attach->adj_0x014[2].scale_0x00.z = scale;
        attach->adj_0x014[3].rot_0x18.y = lift;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802292D0 */

/* Returns the animation key frame at the motion layer's current frame. */
extern "C" f32 pl_rig_key_get(_PLW* self, f32* out)
{
    return getKeyData(out, pl_rig_get_motion_frame(self));
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229304 */

/* Reads three key-frame channels at the motion layer's current frame. */
extern "C" f32 pl_rig_key_pair_get(_PLW* self, f32* a, f32* b, f32* c)
{
    return anim_key_curve_eval(a, b, c, pl_rig_get_motion_frame(self));
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229358 */

/* Reads four key-frame channels at the motion layer's current frame. */
extern "C" void pl_rig_key_triple_get(_PLW* self, f32* a, f32* b, f32* c, f32* d)
{
    getKeyData3(a, pl_rig_get_motion_frame(self), b, c, d);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802293BC */

/* Enables the model's world callback at `timing`; timings 1 and 2 also set the node it runs at. */
void LbEquipModelCallbackB::ApplyTiming(nw4r::g3d::ScnMdlSimple* track, s32 mode, u32 arg)
{
    switch (mode) {
    case 1:
    case 2:
        track->EnableScnMdlCallbackTiming((nw4r::g3d::ScnObj::Timing)mode);
        track->SetCalcWorldNodeID(arg);
        break;
    case 4:
        track->EnableScnMdlCallbackTiming((nw4r::g3d::ScnObj::Timing)mode);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229430 */

/* The same timing hook for the other callback class. */
void LbEquipModelCallbackA::ApplyTiming(nw4r::g3d::ScnMdlSimple* track, s32 mode, u32 arg)
{
    switch (mode) {
    case 1:
    case 2:
        track->EnableScnMdlCallbackTiming((nw4r::g3d::ScnObj::Timing)mode);
        track->SetCalcWorldNodeID(arg);
        break;
    case 4:
        track->EnableScnMdlCallbackTiming((nw4r::g3d::ScnObj::Timing)mode);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802294A4 */

/* Before a node's matrix: zeroes the root's translation and turns the spine nodes (2, 3, 12, 13) about y by a share
 * of the actor's body lean. */
void LbEquipModelCallbackB::ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                                          nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::MTX33 rot;
    nw4r::math::VEC3 trans;
    f32 angle;

    MTX34_ctor(&mtx);
    MTX33_ctor(&rot);
    VEC3_ctor(&trans);
    switch (pFuncObj->GetCallbackNodeID()) {
    case 0:
        trans.x = 0.0f;
        trans.y = 0.0f;
        trans.z = 0.0f;
        anm_chr_result_trans_set((ResAnmChrObj*)pResult, &trans.x);
        return;
    case 2:
        angle = 0.3f * (360.0f * (f32)(s16)plw_0x04->field_0x0B2 / 65536.0f);
        break;
    case 3:
        angle = 0.3f * (360.0f * (f32)(s16)plw_0x04->field_0x0B2 / 65536.0f);
        break;
    case 12:
        angle = 0.3f * (360.0f * (f32)(s16)plw_0x04->field_0x0B2 / 65536.0f);
        break;
    case 13:
        angle = 0.1f * (360.0f * (f32)(s16)plw_0x04->field_0x0B2 / 65536.0f);
        break;
    default:
        return;
    }
    anm_chr_result_mtx_get((ResAnmChrObj*)pResult, (f32*)&mtx);
    rotMatrixY((u16)(s32)(0.5f + 65536.0f * angle / 360.0f), &mtx);
    anm_chr_result_mtx_set((ResAnmChrObj*)pResult, (f32*)&mtx);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229698 */

/* After a node's matrix: pins it to its body joint, moves the walk to the next node and, on the hair and cloth
 * models, rotates the node of a seated sway chain. */
void LbEquipModelCallbackB::ExecCallbackB(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                                          nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
    u32 node = pFuncObj->GetCallbackNodeID();
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    nw4r::math::MTX34 turn;
    nw4r::math::MTX34 mtx;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    MTX34_ctor(&turn);
    MTX34_ctor(&mtx);
    if (node_map_0x10[node] != 0xFFFFFFFF) {
        joint_mtx_store((MtxHolder*)pManip,
                        &mhchar_wld_mtx_array_get((nw4r::g3d::ScnMdlSimple*)body_0x08->field_0x118)[node_map_0x10[node]]);
    }
    pFuncObj->SetCallbackNodeID(pFuncObj->GetCallbackNodeID() + 1);
    if ((index_0x0C == 5 || index_0x0C == 4) && node != 0) {
        u8* range = pl_rig_sway_ranges[index_0x0C];
        u8 i;

        YureRec* rec;
        u32 side;

        for (i = range[0];; i++) {
            if (i >= range[1]) {
                return;
            }
            rec = &((YurePlayerWork*)plw_0x04)->yure_0x674[i];
            if (rec->active_0x00 != 0) {
                side = 0;
                if (node == rec->joint_0x10[0]) {
                    break;
                }
                side = 1;
                if (node == rec->joint_0x10[1]) {
                    break;
                }
            }
        }
        {
            rec->is_enemy_0x06 = 1;
            if (rec->state_0x03 != 0) {
                if ((s32)rec->shape_0x01 == 1) {
                    joint_mtx_load((MtxHolder*)pManip, &mtx);
                    mtx34_identity(&turn);
                    rotMatrixX(rec->pitch_0x78, &turn);
                    mtx34_concat_assign(&mtx, &turn);
                    joint_mtx_store((MtxHolder*)pManip, &mtx);
                    return;
                }
                joint_mtx_load((MtxHolder*)pManip, &mtx);
                yure_angles_calc(rec, &mtx, &rec->pitch_0x78, &rec->roll_0x80, side);
                yure_joint_apply(rec, (MtxHolder*)pManip);
            }
            return;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229868 */

/* Finds the first entry of `rows` after `index` whose gate byte is set and hands it to the track
 * update; stops at the table's seventh entry. */
extern "C" void pl_rig_node_next_set(u32 index, PlRigNodeRule* rows, nw4r::g3d::FuncObjCalcWorld* arg)
{
    u32 i;

    for (i = index + 1; i < 7; i++) {
        if (rows[i].mode_0x00 != 0) {
            arg->SetCallbackNodeID(i);
            return;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x802298AC */

/* Before a node's matrix: applies the node's joint adjustment to its animation result (scale, rotation and
 * translation) and moves the walk to the next ruled node. */
void LbEquipModelCallbackA::ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                                          nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
    nw4r::math::VEC3 v;
    u32 node;
    PlRigNodeRule* rules;
    PlRigNodeRule* rule;
    PlSeJointAdj* adj;

    VEC3_ctor(&v);
    node = pFuncObj->GetCallbackNodeID();
    rules = pl_rig_node_rules[kind_0x10];
    rule = &rules[node];
    adj = &adj_0x014[node];
    if (rule->mode_0x00 == 1) {
        anm_chr_result_scale_get((ResAnmChrObj*)pResult, &v.x);
        v.x *= adj->scale_0x00.x;
        v.y *= adj->scale_0x00.y;
        v.z *= adj->scale_0x00.z;
        anm_chr_result_scale_set((ResAnmChrObj*)pResult, &v.x);
        anm_chr_result_rot_get((ResAnmChrObj*)pResult, &v.x);
        v.x += adj->rot_0x18.x;
        v.y += adj->rot_0x18.y;
        v.z += adj->rot_0x18.z;
        anm_chr_result_rot_set((ResAnmChrObj*)pResult, &v.x);
        anm_chr_result_trans_get((ResAnmChrObj*)pResult, &v.x);
        v.x += adj->trans_0x0C.x;
        v.y += adj->trans_0x0C.y;
        v.z += adj->trans_0x0C.z;
        anm_chr_result_trans_set((ResAnmChrObj*)pResult, &v.x);
    }
    if ((rule->flags_0x01 & 2) == 0) {
        pl_rig_node_next_set(node, rules, pFuncObj);
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229A20 */

/* After a node's matrix: rebuilds a joint-ruled node's matrix from its adjustment and its body joint (the lobby uses
 * the body root and unit scale) and moves the walk to the next ruled node. */
void LbEquipModelCallbackA::ExecCallbackB(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                                          nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
    u32 node = pFuncObj->GetCallbackNodeID();
    nw4r::math::MTX34 mtx;
    nw4r::math::MTX34 joint_mtx;
    nw4r::math::VEC3 trans;
    PlRigNodeRule* rules;
    PlRigNodeRule* rule;

    MTX34_ctor(&mtx);
    MTX34_ctor(&joint_mtx);
    VEC3_ctor(&trans);
    rules = pl_rig_node_rules[kind_0x10];
    rule = &rules[node];
    if (rule->mode_0x00 == 2) {
        PlSeJointAdj* adj = &adj_0x014[node];

        mtx34_identity(&mtx);
        mtx34_rot_xyz_deg((f32*)&mtx, adj->rot_0x18.x, adj->rot_0x18.y, adj->rot_0x18.z);
        if (system_lobby_flag_get() != 2) {
            u16 joint;

            trans.x = adj->trans_0x0C.x;
            trans.y = adj->trans_0x0C.y;
            trans.z = adj->trans_0x0C.z;
            mtx34_trans_apply((Mtx34*)&mtx, (Vec3*)&trans, &mtx);
            joint = rule->joint_0x02;
            if (joint == 0xFFFF) {
                joint = joint_0x0C;
            }
            mhchar_joint_mtx_get(&models_0x04->model, joint, (Mtx34*)&joint_mtx);
            mtx34_concat((Mtx34*)&mtx, (Mtx34*)&joint_mtx, (Mtx34*)&mtx);
            world_mtx_attr_scale_set((VtxAttrRef*)pManip, adj->scale_0x00.x * model_0x08->scale_0x1C.x,
                                     adj->scale_0x00.y * model_0x08->scale_0x1C.y,
                                     adj->scale_0x00.z * model_0x08->scale_0x1C.z);
        } else {
            u16 joint;

            trans.x = adj->trans_0x0C.x;
            trans.y = adj->trans_0x0C.y;
            trans.z = adj->trans_0x0C.z;
            mtx34_trans_apply((Mtx34*)&mtx, (Vec3*)&trans, &mtx);
            joint = rule->joint_0x02;
            if (joint == 0xFFFF || kind_0x10 == 7) {
                joint = 0;
            }
            mhchar_joint_mtx_get(&models_0x04->model, joint, (Mtx34*)&joint_mtx);
            mtx34_concat((Mtx34*)&mtx, (Mtx34*)&joint_mtx, (Mtx34*)&mtx);
            world_mtx_attr_scale_set((VtxAttrRef*)pManip, 1.0f, 1.0f, 1.0f);
        }
        joint_mtx_store((MtxHolder*)pManip, &mtx);
    }
    pl_rig_node_next_set(node, rules, pFuncObj);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229CB4 */

/* Picks the four SE frame codes out of the actor's equipment decorations and stores them on the
 * actor's SE work, collapsing duplicates to zero. */
extern "C" void fn_80229CB4(_PLW* self)
{
    s32 n0 = pl_equip_model_id_get(self, &self->equipA[3]);
    s32 n1 = pl_equip_model_id_get(self, &self->equipA[2]);
    s32 n2 = pl_equip_model_id_get(self, &self->equipA[0]);
    s32 n3 = pl_equip_model_id_get(self, &self->equipA[1]);
    u8 v0;
    u8 v1;
    u8 v2;
    u8 v3;

    if (n0 > 0) {
        v0 = lbl_807913D0[self->se_name_set][n0];
    } else {
        v0 = 0;
    }
    if (n1 > 0) {
        v1 = lbl_807913C8[self->se_name_set][n1];
    } else {
        v1 = 0;
    }
    if (n2 > 0) {
        v2 = lbl_807913C0[self->se_name_set][n2];
    } else {
        v2 = 0;
    }
    if (n3 > 0) {
        v3 = lbl_807913D8[self->se_name_set][n3];
    } else {
        v3 = 0;
    }

    if (v0 == v1) {
        v1 = 0;
    }
    if (v0 == v2) {
        v2 = 0;
    }
    if (v1 == v2) {
        v2 = 0;
    }

    self->field_0xAF4->field_0x0A3D = v0;
    self->field_0xAF4->field_0x0A3E = v1;
    self->field_0xAF4->field_0x0A3F = v2;
    self->field_0xAF4->field_0x0A40 = v3;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229E10 */

/* Arms one SE frame request for each of the actor's three frame codes. */
extern "C" void se_req_frame_set_parts3(_se_w* work, s32 code, s32 val)
{
    if (work->field_0x0A3D != 0) {
        se_req_frame_set(work, code, work->field_0x0A3D, 2, val | 0x1000000);
    }
    if (work->field_0x0A3E != 0) {
        se_req_frame_set(work, code, work->field_0x0A3E, 2, val | 0x1000000);
    }
    if (work->field_0x0A3F != 0) {
        se_req_frame_set(work, code, work->field_0x0A3F, 2, val | 0x1000000);
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229EA8 */

/* Arms the actor's fourth SE frame request. */
extern "C" void se_req_frame_set_part4(_se_w* work, s32 code, s32 x, s32 y)
{
    u32 v = work->field_0x0A40;

    if (v == 0) {
        return;
    }
    se_req_frame_set(work, code, v, x, y | 0x1000000);
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80229BFC / 0x80229C58 */

/* 0x80229BFC (0x5C): Destroys the callback through the world-callback base (called out of line). */
#pragma dont_inline on
LbEquipModelCallbackA::~LbEquipModelCallbackA()
{
}

/* 0x80229C58 (0x5C): Destroys the other callback class the same way. */
LbEquipModelCallbackB::~LbEquipModelCallbackB()
{
}
#pragma dont_inline reset
#pragma peephole reset
#pragma fp_contract reset
