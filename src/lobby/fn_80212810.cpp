/* lobby/fn_80212810.cpp - the lobby's item and equipment UI layer: the sort keys and refusal bits of the smithy's make
 *   and upgrade rows, the menu list/choice box and help panels, the text and prompt helpers, the item and equipment
 *   cells, the detail windows, the prices, and the equipment comparison list's row builder.
 * RANGE. .text 0x80212810-0x80219260 (105 functions, defined in address order); .data 0x805B97BC-0x805B9DDC, .sdata
 *   0x80791DC0-0x80791EB0, .sdata2 0x80799C18-0x80799C40, extab, extabindex.  The `.data`/`.sdata` tables and the five
 *   jump tables are emitted in retail order by defining each table before its first user; no seam inside the range
 *   (the data runs in function order, no vtable, no repeated pool value).
 * FLAGS. `cflags_lobby`; file-scope `#pragma peephole off` (the kept `clrlwi` + `slwi` index scales and unfused narrow
 *   compares), `pool_data off` (each table addressed by its own `lis`/`addi`), `fp_contract off` (the price tables'
 *   `fmuls` + `fadds`) and `optimization_level 4` (the switches' range-first `subi` + `cmplwi` tests, playbook 108).
 * NAMES. No `__FILE__` string covers the range; the file keeps the map's stem (request lobby-80212810#1 proposes
 *   `lobby/lb_item_ui`, GUESS, after `lb_pane_ui`/`lb_companion_ui`/`lb_quest_ui`).  `LbPutAnaPageArrow` is the map's real
 *   name.  GUESS (from each body and its callers): every other `lb_*` name, `draw_font_idx_num`,
 *   `equip_sell_price_get`, and the data `lb_*_ids`/`lb_*_tbl`/`lb_*_layout` (each named for the sprites or values it
 *   holds and the function that reads it).  The foreign names this unit proposed are in the name registry.
 * RESIDUALS.
 *   - `lb_scroll_rows_build`: retail stores `right_rows_0x1D[2]`/`[3]` of the group-2 rows through index registers
 *     (`li r3,2` + `add`); row-indexed spellings fold to constant offsets.  The rest is register colouring.
 *   - `.data` is 4 bytes long: MWCC 8-aligns the 64-byte `lb_equip_kind_rank_tbl` (any object whose size is a multiple of
 *     8) where retail has it 4-aligned at +0x44, so every later table sits 4 bytes late; an `s32[16]`, a struct array,
 *     a struct wrapper, an unsized array, `__attribute__((aligned(4)))` and a function-local static all align it to 8.
 *   - `lb_seisan_materials_get`: retail computes `bonus * 2` and the `gun_level_bonus_tbl` address before clearing
 *     `out`, ours in the loop preheader; a clearing loop and other clear spellings do not move them.
 *   - `lb_list_dialog_draw`: retail keeps `x = frame + 20` unextended (`addi r25,r3,0x14`); the call result
 *     assigned to the `s16` is extended first.
 *   - `lb_kakou_weapon_cost_flags_get`, `lb_deco_row_flags_get`: retail loads the table row pointer straight into
 *     the saved register (`lwzx r30` + `add r30,r30`).
 *   - `lb_sprite_pulse_draw`, `lb_count_window_draw`: `mullw` operand order; both source orders give ours.
 *   - register colouring or scheduling only: `lb_text_center_print` (retail loads `spr.width` before `spr.pos.x`),
 *     `lb_list_draw_ex`, `lb_item_window_draw`, `lb_seisan_row_flags_get`, `lb_kakou_weapon_flags_get`,
 *     `lb_kakou_icon_build`, `lb_recipe_material_flags_get`, `lb_gun_recipe_material_flags_get`,
 *     `lb_item_cell_draw_layout`, `lb_kakou_source_draw`, `lb_item_need_list_draw`, `lb_equip_row_next`,
 *     `lb_scene_npc_refresh` (retail steps the NPC pointer before the counter).
 *   - this unit's views of foreign records (`LbArmorView`, `LbArmorExView`, `LbBladeView`, `LbGunView` over the Pl rows,
 *     `LbSmithRow`/`LbSmithGunRow`/`LbSmithKakouRow` over `lobby/lb_screen_step.cpp`'s recipes, `LbShopGoodsRow` over
 *     `lobby/fn_801E0ADC.cpp`'s `LbIdRow6`, `LbUserNames` over the user data) name bytes their owners leave as padding or
 *     define in their `.cpp`; they fold into the owners' headers when those are touched.
 *   - `item_species_name_get` returns its name in r3 while its owner declares it `void`; called through a typed view.
 * SHAPES. A `case` group's body order follows the source order while its range tests come first (lb_kakou_source_draw);
 *   early `return 0`s are `break`s to one shared `return 0` (lb_equip_kind_ck, lb_equip_wearer_ck); a pointer to the
 *   previous row taken before the `-1` test (the row walkers); flags built with `|=` from a zeroed byte; a kept `bool`
 *   for the values passed to `GetMenuFontColor`; a `continue`/`break` switch instead of retail's goto-shaped material
 *   filter (lb_seisan_materials_get).
 */
#include "types.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off
#pragma optimization_level 4

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/printf.h"
#include "hud/layout.h"
#include "hud/draw_font_align.h"
#include "hud/put_itemicon.h"
#include "hud/draw_monstericon_rec.h"
#include "hud/cockpit.h"
#include "hud/cockpit_blink_get.h"
#include "font/flfnt.h"
#include "font/font_print_plain.h"
#include "menu/menu_message.h"
#include "menu/put_frame_dialog_title.h"
#include "menu/put_menu_cursor.h"
#include "menu/get_pop_dat_ptr.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800E3CBC.h"
#include "mh3_pad/system_w.h"
#include "lobby/lobby_w.h"
#include "lobby/LbStr.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_list.h"
#include "lobby/LbPutAnaPageArrow.h"
#include "lobby/lb_frame_draw_at.h"
#include "lobby/lb_font_print.h"
#include "fn_80047398/lobby_world_block.h"
#include "quest/quest_types.h"
#include "menu/menu_item.h"
#include "menu/menu_row.h"
#include "menu/place_rec_make_set.h"
#include "menu/skill_table_page_count.h"
#include "menu/menu_infomation.h"
#include "userdata_item.h"
#include "userdata_item/userdata_box_capacity.h"
#include "userdata_item/userdata_equip_box_count.h"
#include "Pl/fn_8027D684.h"
#include "Pl/pl_status_sheet_get.h"
#include "Pl/equip_slot_rec_find.h"
#include "Pl/equip_armor_ex_rec_find.h"
#include "Pl/equip_armor_ex_rec_get.h"
#include "Pl/equip_gun_level_bonus_get.h"
#include "Pl/equip_blade_rec_find.h"
#include "Pl/equip_weapon_rec_find.h"
#include "Pl/equip_weapon_recs_find.h"
#include "ef/eft052.h"
#include "ef/eft052_page_counts_get.h"
#include "camera/camera_lobby_disarm.h"
#include "lobby/lb_npc.h"
#include "lobby/lb_npc_talk_select.h"
#include "lobby/lb_goods_tier_get.h"
#include "lobby/lb_tr_flag_get.h"
#include "lobby/equip_sell_price_get.h"
#include "lobby/seisan_data.h"
#include "lobby/lb_shop_row_unlocked_ck.h"
#include "Pl/gun_level_bonus_tbl.h"
#include "Pl/equip_defense_get.h"
#include "Pl/equip_weapon_rec_get.h"
#include "Pl/item_species_class_get.h"
#include "Pl/equip_slot_rec_get.h"
#include "Pl/equip_deco_count.h"
#include "Pl/item_kind_name_get.h"
#include "userdata_item/item_slots_room_get.h"
#include "userdata_equip_flag_ck.h"
#include "userdata_progress_flag_ck.h"
#include "fn_8004CAD8.h"
#include "ef/eft052_page_put.h"

/* This unit's views of the Pl equipment rows, for the bytes `Pl/fn_8027D684.h` leaves as padding.
 * The armour row (`PlSlotRec`, `equip_slot_rec_find`): its slot byte, the wear flags and the five stats.
 * size: 0x18 */
typedef struct LbArmorView {
    /* +0x00 */ u8 slots_0x00;
    /* +0x01 */ u8 pad_0x01[5];
    /* +0x06 */ u8 wear_flags_0x06;
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ s8 defense_0x08;
    /* +0x09 */ s8 resist_0x09;
    /* +0x0A */ s8 resist_0x0A;
    /* +0x0B */ s8 resist_0x0B;
    /* +0x0C */ s8 resist_0x0C;
    /* +0x0D */ u8 pad_0x0D[0xB];
} LbArmorView; /* size: 0x18 */

/* The armour's second row (`equip_armor_ex_rec_find`): its rarity, the price and the upgrade materials.
 * size: 0x1C */
typedef struct LbArmorExView {
    /* +0x00 */ u16 rare_0x00;
    /* +0x02 */ u8 pad_0x02[6];
    /* +0x08 */ s32 price_0x08;
    /* +0x0C */ LbItemCount materials_0x0C[4];
} LbArmorExView; /* size: 0x1C */

/* The blade row (`PlSkillRec`): the price, the attack, the two element/status pairs, the unlock condition and the
 * rarity.  size: 0x18 */
typedef struct LbBladeView {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ s32 price_0x04;
    /* +0x08 */ u8 pad_0x08[2];
    /* +0x0A */ u16 attack_0x0A;
    /* +0x0C */ u8 pad_0x0C[2];
    /* +0x0E */ u8 element_a_type_0x0E;
    /* +0x0F */ s8 element_a_value_0x0F;
    /* +0x10 */ u8 element_b_type_0x10;
    /* +0x11 */ s8 element_b_value_0x11;
    /* +0x12 */ u8 pad_0x12[2];
    /* +0x14 */ u8 unlock_0x14;
    /* +0x15 */ u8 rare_0x15;
    /* +0x16 */ u8 pad_0x16[2];
} LbBladeView; /* size: 0x18 */

/* The bowgun part row (`PlWeaponRec`): its bonus row, the attack, the price, the rarity and the unlock condition.
 * size: 0x24 */
typedef struct LbGunView {
    /* +0x00 */ u8 pad_0x00[3];
    /* +0x03 */ u8 bonus_row_0x03;
    /* +0x04 */ u16 attack_0x04;
    /* +0x06 */ u8 pad_0x06[6];
    /* +0x0C */ s32 price_0x0C;
    /* +0x10 */ u8 pad_0x10[3];
    /* +0x13 */ u8 rare_0x13;
    /* +0x14 */ u8 pad_0x14[2];
    /* +0x16 */ u8 unlock_0x16;
    /* +0x17 */ u8 pad_0x17[0xD];
} LbGunView; /* size: 0x24 */

/* One make recipe row of `seisan_data`/`deco_sei_tbl_ptr`: the lock byte, the result item, the four materials and
 * their unlock conditions.  size: 0x18 */
typedef struct LbSmithRow {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 item_0x02;
    /* +0x04 */ LbItemCount materials_0x04[4];
    /* +0x14 */ u8 cond_0x14[4];
} LbSmithRow; /* size: 0x18 */

/* One gun-part make row of `seisan_data` (kinds 11..13): six materials and their level/unlock nibbles.
 * size: 0x24 */
typedef struct LbSmithGunRow {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 item_0x02;
    /* +0x04 */ LbItemCount materials_0x04[6];
    /* +0x1C */ u8 cond_0x1C[6];
    /* +0x22 */ u8 pad_0x22[2];
} LbSmithGunRow; /* size: 0x24 */

/* One weapon upgrade row of `kakou_data`: three materials and the four pieces it upgrades into.  size: 0x18 */
typedef struct LbSmithKakouRow {
    /* +0x00 */ LbItemCount materials_0x00[3];
    /* +0x0C */ u16 next_0x0C[4];
    /* +0x14 */ u8 pad_0x14[4];
} LbSmithKakouRow; /* size: 0x18 */

/* One row of a shop's goods table (terminated by 0xFF in both halves): the item, its stock key and price key.
 * size: 0x6 */
typedef struct LbShopGoodsRow {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ u16 stock_0x02;
    /* +0x04 */ u16 price_0x04;
} LbShopGoodsRow; /* size: 0x6 */

/* One `_tr_flag` record (`lb_tr_flag_get`): the goods id and its name.  size: 0xC */
struct LbTrFlag {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ s8* name_0x04;
    /* +0x08 */ u8 pad_0x08[4];
}; /* size: 0xC */

/* The 0x80-byte equipment comparison list (`lb_scroll_init` clears it): its kind and mode, the two pages' modes and
 * equipment groups, the left and right column kind of each row, the compared and the base pieces with their bowgun
 * parts, and the player work the page counts read.  size: 0x80 */
typedef struct LbScrollWork {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 mode_0x02;
    /* +0x03 */ u8 paged_0x03;
    /* +0x04 */ u8 left_mode_0x04;
    /* +0x05 */ u8 right_mode_0x05;
    /* +0x06 */ u8 unused_0x06[2];
    /* +0x08 */ s16 cursor_0x08;
    /* +0x0A */ s16 count_0x0A;
    /* +0x0C */ u16 value_0x0C;
    /* +0x0E */ u16 value_0x0E;
    /* +0x10 */ u8 flags_0x10;
    /* +0x11 */ u8 left_group_0x11;
    /* +0x12 */ u8 right_group_0x12;
    /* +0x13 */ u8 left_rows_0x13[10];
    /* +0x1D */ u8 right_rows_0x1D[10];
    /* +0x27 */ u8 unused_0x27[5];
    /* +0x2C */ s32 value_0x2C;
    /* +0x30 */ s32 value_0x30;
    /* +0x34 */ _PLW* owner_0x34;
    /* +0x38 */ _EQUIP cmp_0x38;
    /* +0x44 */ _EQUIP cmp_parts_0x44[2];
    /* +0x5C */ _EQUIP base_0x5C;
    /* +0x68 */ _EQUIP base_parts_0x68[2];
} LbScrollWork; /* size: 0x80 */

/* `item_species_name_get` returns the species' name in r3; its owner declares it `void`. */
#define item_species_name_str ((s8* (*)(u16))item_species_name_get)

/* One row of a list's help table (`LbList::help_tbl_0x10`): the refusal line's string and the row it goes on.
 * size: 0x4 */
typedef struct LbHelpPair {
    /* +0x00 */ s16 str_0x00;
    /* +0x02 */ s16 row_0x02;
} LbHelpPair;

/* The user-data bytes `lb_speaker_name_get` reads: the three Felyne name ids and the Poogie name id.  size: 0x483B
 * (a view: the block is 0x6000 bytes) */
typedef struct LbUserNames {
    /* +0x0000 */ u8 pad_0x0000[0x3F95];
    /* +0x3F95 */ u8 cat_name_0x3F95[3];
    /* +0x3F98 */ u8 pad_0x3F98[0x483A - 0x3F98];
    /* +0x483A */ u8 pig_name_0x483A;
} LbUserNames;

extern "C" {

/* Defined further down. */
s32 lb_sort_key_base_get(s32 sort);
s32 lb_armor_upgrade_price_get(_EQUIP* equip, u8 level);
s32 lb_gun_upgrade_price_get(_EQUIP* equip, u8 level);
s32 lb_seisan_price_get(u8 kind, u16 id);
s32 lb_row_init(_EQUIP* row, u8 kind, u16 item);
u16 lb_seisan_row_flags_get(u16 kind, u16 idx, u8 level);
void lb_list_row_draw(s8* text, s32 sel, _mh_ivec2_* pos, u32 color, u8 kind);
u16 lb_kakou_armor_flags_get(u8 kind, u16 item, u8 level);
u16 lb_kakou_gun_flags_get(u8 kind, u16 item, u8 level);
u16 lb_kakou_weapon_flags_get(u8 kind, u16 idx, _EQUIP* equip);
s32 lb_scene_npc_refresh(s32 on);
void lb_item_detail_draw(u16 panel, u16 item, const u16* ids, u16 box_mark, u16 pouch_mark);
s32 lb_panel_msg_draw(s32 panel, s16 str);
void lb_panel_line_draw(u16 panel, s16 str, s16 color, s32 row);
s32 lb_help_text_put(u16 panel, s8* text);

}



extern "C" {

s32 lb_sort_filter_name_get(s32 id)
{
    s32 value;

    switch ((u8)id) {
    case 0:
    default:
        value = 249;
        break;
    case 1:
        value = 250;
        break;
    case 2:
        value = 251;
        break;
    case 3:
        value = 252;
        break;
    case 4:
        value = 253;
        break;
    case 5:
        value = 254;
        break;
    case 6:
        value = 255;
        break;
    case 7:
        value = 256;
        break;
    case 8:
        value = 257;
        break;
    case 9:
        value = 269;
        break;
    case 10:
        value = 270;
        break;
    case 11:
        value = 235;
        break;
    case 12:
        value = 236;
        break;
    case 13:
        value = 237;
        break;
    }
    return (s32)LbStr(0, (u16)value);
}

/* The weapon class each sort-filter kind 1..8 compares against an equipment record's class byte. */
u8 lb_equip_kind_class_tbl[12] = { 0, 1, 2, 5, 3, 4, 1, 2, 3 };
s32 lb_equip_kind_ck(u8 kind, s32 arg, s32 sel)
{
    LbBladeView* equip;
    PlSlotRec* flags;

    switch (kind) {
    case 0:
        return 1;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        if ((u8)equip_kind_table_class((u8)arg) == 1 && (u8)arg != 11) {
            equip = (LbBladeView*)equip_blade_rec_find((u8)arg, (u16)sel);
            if (lb_equip_kind_class_tbl[kind] == equip->element_a_type_0x0E && equip->element_a_value_0x0F > 0) {
                return 1;
            }
        }
        break;
    case 6:
    case 7:
    case 8:
        if ((u8)equip_kind_table_class((u8)arg) == 1 && (u8)arg != 11) {
            equip = (LbBladeView*)equip_blade_rec_find((u8)arg, (u16)sel);
            if (lb_equip_kind_class_tbl[kind] == equip->element_b_type_0x10 && equip->element_b_value_0x11 > 0) {
                return 1;
            }
        }
        break;
    case 9:
        flags = equip_slot_rec_find((u8)arg, (u16)sel);
        if ((flags->wear_flags & 4) != 0) {
            return 1;
        }
        break;
    case 10:
        flags = equip_slot_rec_find((u8)arg, (u16)sel);
        if ((flags->wear_flags & 8) != 0) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 lb_deco_class_ck(s32 kind, s32 arg)
{
    switch ((u8)kind) {
    case 0:
        return 1;
    case 11:
        if (item_species_class_get((u16)arg) == 1) {
            return 1;
        }
        break;
    case 12:
        if (item_species_class_get((u16)arg) == 2) {
            return 1;
        }
        break;
    case 13:
        if (item_species_class_get((u16)arg) == 3) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x80212AD0 (0x4C): the key a sort order `sort` starts its walk above (1000 for the stat orders, else 0). */
s32 lb_sort_key_base_get(s32 sort)
{
    sort = (u8)sort;
    switch (sort) {
    case 0:
    case 16:
        return 0;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        return 1000;
    case 17:
        return 0;
    default:
        return 0;
    }
}




/* 0x80212B1C (0x320): the sort key of item `id` of equipment kind `kind` under sort order `sort` (an armour stat, an
 * element or status value, the attack, or the negated rarity for 16), 0 when the order does not apply. */
s32 lb_item_sort_key_get(u8 kind, u8 sort, u16 id)
{
    LbBladeView* blade;
    LbGunView* gun;
    u8 want;

    switch ((u8)equip_kind_table_class(kind)) {
    case 0: {
        LbArmorView* armor;
        if (sort == 16) {
            return -((LbArmorExView*)equip_armor_ex_rec_find(kind, id))->rare_0x00;
        }
        armor = (LbArmorView*)equip_slot_rec_find(kind, id);
        switch (sort) {
        case 0:
            return 0;
        case 6:
            return armor->slots_0x00;
        case 1:
            return armor->defense_0x08;
        case 2:
            return armor->resist_0x09;
        case 3:
            return armor->resist_0x0B;
        case 4:
            return armor->resist_0x0A;
        case 5:
            return armor->resist_0x0C;
        }
        break;
    }
    case 1:
        equip_weapon_recs_find(kind, id, (PlSkillRec**)&blade, (PlWeaponRec**)&gun);
        if (blade != NULL) {
            switch (sort) {
            case 0:
                return 0;
            case 12:
                return blade->attack_0x0A;
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                switch (sort) {
                case 7:
                    want = 1;
                    break;
                case 8:
                    want = 2;
                    break;
                case 9:
                    want = 5;
                    break;
                case 10:
                    want = 3;
                    break;
                case 11:
                    want = 4;
                    break;
                }
                if (blade->element_a_type_0x0E == want) {
                    return blade->element_a_value_0x0F > 0 ? blade->element_a_value_0x0F : 0;
                }
                return 0;
            case 13:
            case 14:
            case 15:
                switch (sort) {
                case 13:
                    want = 1;
                    break;
                case 14:
                    want = 2;
                    break;
                case 15:
                    want = 3;
                    break;
                }
                if (blade->element_b_type_0x10 == want) {
                    return blade->element_b_value_0x11 > 0 ? blade->element_b_value_0x11 : 0;
                }
                return 0;
            case 16:
                return -blade->rare_0x15;
            }
        }
        if (gun != NULL) {
            switch (sort) {
            case 0:
                return 0;
            case 12:
                return gun->attack_0x04;
            case 16:
                return -gun->rare_0x13;
            }
        }
        break;
    case 2:
        equip_weapon_recs_find(kind, id, (PlSkillRec**)&blade, (PlWeaponRec**)&gun);
        if (gun != NULL) {
            switch (sort) {
            case 0:
                return 0;
            case 12:
                return gun->attack_0x04;
            case 16:
                return -gun->rare_0x13;
            }
        }
        break;
    }
    return 0;
}

s32 lb_shop_row_next(u8 kind, u8 sort, s32 prev, LbShopGoodsRow* table)
{
    s32 best_key;
    s32 best;
    s32 base;
    s32 i;
    s32 key;
    LbShopGoodsRow* prev_row;

    best_key = 0;
    best = -1;
    prev_row = &table[prev];
    if (prev == -1) {
        base = lb_sort_key_base_get(sort);
    } else {
        base = lb_item_sort_key_get(kind, sort, prev_row->item_0x00);
    }
    for (i = 0; table->stock_0x02 != 0xFF && table->price_0x04 != 0xFF; i++, table++) {
        if ((u32)lb_shop_row_unlocked_ck((LbIdRow6*)table) == 1) {
            key = lb_item_sort_key_get(kind, sort, table->item_0x00);
            if ((base > key || (base == key && prev < i)) &&
                (best == -1 || key > best_key || (key == best_key && i < best))) {
                best = i;
                best_key = key;
            }
        }
    }
    return best;
}

/* 0x80212F34 (0xC8): takes make row `idx` (item `item`) as the next pick when it is not locked and its key follows
 * `base`/`prev` closer than the pick so far in `best`/`best_key`. */
void lb_seisan_row_pick(u8 kind, u8 sort, s32 prev, s32 idx, u16 item, s32 base, s32* best, s32* best_key)
{
    s32 key;
    s32 open;

    open = lb_seisan_row_flags_get(kind, idx, 1) != 0x10;
    if (open != 0) {
        key = lb_item_sort_key_get(kind, sort, item);
        if ((base > key || (base == key && prev < idx)) &&
            (*best == -1 || key > *best_key || (key == *best_key && idx < *best))) {
            *best = idx;
            *best_key = key;
        }
    }
}

s32 lb_seisan_row_next(u8 kind, u8 sort, s32 prev, LbSmithRow* table)
{
    s32 best_key;
    s32 best;
    s32 base;
    s32 i;
    LbSmithRow* row;
    LbSmithRow* prev_row;

    best_key = 0;
    best = -1;
    prev_row = &table[prev];
    i = 0;
    row = table;
    if (prev == -1) {
        base = lb_sort_key_base_get(sort);
    } else {
        base = lb_item_sort_key_get(kind, sort, prev_row->item_0x02);
    }
    for (; row->item_0x02 != 0xFFFF; i++, row++) {
        lb_seisan_row_pick(kind, sort, prev, i, row->item_0x02, base, &best, &best_key);
    }
    return best;
}

s32 lb_seisan_gun_row_next(u8 kind, u8 sort, s32 prev, LbSmithGunRow* table)
{
    s32 best_key;
    s32 best;
    s32 base;
    s32 i;
    LbSmithGunRow* row;
    LbSmithGunRow* prev_row;

    best_key = 0;
    best = -1;
    prev_row = &table[prev];
    i = 0;
    row = table;
    if (prev == -1) {
        base = lb_sort_key_base_get(sort);
    } else {
        base = lb_item_sort_key_get(kind, sort, prev_row->item_0x02);
    }
    for (; row->item_0x02 != 0xFFFF; i++, row++) {
        lb_seisan_row_pick(kind, sort, prev, i, row->item_0x02, base, &best, &best_key);
    }
    return best;
}

/* 0x8021317C (0x94): whether armour `id` of kind `kind` can be worn by `wearer` (0 any, 1 a blademaster, else a
 * gunner); weapons pass only for any wearer. */
s32 lb_equip_wearer_ck(u8 kind, u16 id, u8 wearer)
{
    PlSlotRec* rec;

    if (wearer != 0) {
        switch (kind) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            rec = equip_slot_rec_find(kind, id);
            if (wearer == 1) {
                if ((rec->wear_flags & 4) != 0) {
                    return 1;
                }
            } else if ((rec->wear_flags & 8) != 0) {
                return 1;
            }
            break;
        }
        return 0;
    }
    return 1;
}

/* The sort rank of each equipment kind (0..15) in the equipment box. */
s32 lb_equip_kind_rank_tbl[16] = { 0x0F000000, 0x0A000000, 0x0B000000, 0x0C000000, 0x0D000000, 0x09000000,
                                   0x0E000000, 0x00000000, 0x01000000, 0x02000000, 0x03000000, 0x06000000,
                                   0x07000000, 0x08000000, 0x05000000, 0x04000000 };

/* 0x80213210 (0x80): the box sort penalty of equipment `id` of kind `kind`: its kind rank plus bits for a kind outside
 * `kinds`, a kind outside `picked` and a piece `wearer` cannot wear. */
s32 lb_equip_sort_penalty_get(u8 kind, u16 id, s32 kinds, s32 picked, u8 wearer)
{
    s32 penalty;
    s32 bit;

    penalty = lb_equip_kind_rank_tbl[kind];
    bit = 1 << kind;
    if ((kinds & bit) == 0) {
        penalty |= 0x20000000;
    }
    if ((picked & bit) == 0) {
        penalty |= 0x10000000;
    }
    if (lb_equip_wearer_ck(kind, id, wearer) == 0) {
        penalty |= 0x40000000;
    }
    return penalty;
}

/* 0x80213290 (0xE4): the sort key of the box piece `equip` under sort order `sort` (17 the decoration rarity, 12 the
 * attack with a bowgun's upgrade bonus, 6 an armour's defence). */
s32 lb_equip_sort_key_get(_EQUIP* equip, u8 sort)
{
    switch (sort) {
    case 17:
        if (equip->kind == 6) {
            return Get_equip_rare(equip);
        }
        return 0;
    case 12:
        if (equip->kind - 11U <= 2) {
            return equip_gun_level_bonus_get(equip);
        }
        return lb_item_sort_key_get(equip->kind, sort, equip->item_id);
    case 6:
        if ((u8)equip_kind_table_class(equip->kind) == 0) {
            return (s16)equip_defense_get(equip);
        }
        return lb_item_sort_key_get(equip->kind, sort, equip->item_id);
    default:
        return lb_item_sort_key_get(equip->kind, sort, equip->item_id);
    }
}

/* 0x80213374 (0x170): the index of the box piece of `rows` (`count` of them) that follows piece `prev` (-1: the first)
 * in sort order `sort`, the kind rank and the penalties first, or -1. */
s32 lb_equip_row_next(u8 sort, s32 prev, _EQUIP* rows, u16 count, s32 kinds, s32 picked, u8 wearer)
{
    s32 base;
    s32 best;
    s32 key;
    s32 best_key;
    _EQUIP* row;
    s32 i;

    best_key = 0x80000000;
    best = -1;
    if (prev == -1) {
        base = lb_sort_key_base_get(sort);
        if (sort != 16) {
            base <<= 8;
        }
    } else {
        row = &rows[prev];
        base = lb_equip_sort_key_get(row, sort);
        if (sort != 16) {
            base <<= 8;
            base += lb_equip_sort_key_get(row, 16);
        }
        base -= lb_equip_sort_penalty_get(row->kind, row->item_id, kinds, picked, wearer);
    }
    for (i = 0; i < count; i++, rows++) {
        key = lb_equip_sort_key_get(rows, sort);
        if (sort != 16) {
            key <<= 8;
            key += lb_equip_sort_key_get(rows, 16);
        }
        key -= lb_equip_sort_penalty_get(rows->kind, rows->item_id, kinds, picked, wearer);
        if ((base > key || (base == key && prev < i)) &&
            (best == -1 || key > best_key || (key == best_key && i < best))) {
            best = i;
            best_key = key;
        }
    }
    return best;
}

/* 0x802134E4 (0x110): whether the unlock condition `cond` of a recipe is met by the user data `user` (1/2 always,
 * 3 a hunter rank, 4/5 the online game or a story flag, 6..8 a story flag). */
u32 lb_recipe_cond_ck(Q_UserData* user, s16 cond)
{
    s32 ok;

    ok = 0;
    switch (cond) {
    case 1:
    case 2:
        ok = 1;
        break;
    case 3:
        if (user->hunter_rank_0x3DE4 != 0) {
            ok = 1;
        }
        break;
    case 4:
        if (game_ready_ck() == 1) {
            ok = 1;
        } else if (userdata_progress_flag_ck(0x2F) == 1) {
            ok = 1;
        }
        break;
    case 5:
        if (game_ready_ck() == 1) {
            ok = 1;
        } else if (userdata_progress_flag_ck(0x32) == 1) {
            ok = 1;
        }
        break;
    case 6:
        if (userdata_progress_flag_ck(6) == 1) {
            ok = 1;
        }
        break;
    case 7:
        if (userdata_progress_flag_ck(7) == 1) {
            ok = 1;
        }
        break;
    case 8:
        if (userdata_progress_flag_ck(8) == 1) {
            ok = 1;
        }
        break;
    }
    return ok != 0;
}

/* 0x802135F4 (0x14C): the refusal bits of make row `row`'s four materials (8 one is short, 0x10 the first is locked)
 * for the user data `user`; `level` nonzero skips the held counts. */
u16 lb_recipe_material_flags_get(Q_UserData* user, LbSmithRow* row, u8 level)
{
    u16 flags;
    s32 i;
    s32 have;

    flags = 0;
    for (i = 0; i < 4; i++) {
        if (row->materials_0x04[i].item_0x00 != 0 && row->materials_0x04[i].count_0x02 > 0) {
            u8 boxed = userdata_box_item_ck((LbItemDb*)user, row->materials_0x04[i].item_0x00) == 1;
            if (level == 0) {
                have = userdata_item_count_total(row->materials_0x04[i].item_0x00, lobby_world_block);
            }
            if (row->kind_0x00 == 0 && i == 0) {
                switch (row->cond_0x14[i]) {
                case 2:
                case 3:
                    if (boxed == 0) {
                        flags |= 0x10;
                    }
                    break;
                case 4:
                case 5:
                    if (game_ready_ck() == 1 && boxed == 0) {
                        flags |= 0x10;
                    }
                    break;
                }
                if (lb_recipe_cond_ck(user, row->cond_0x14[i]) == 0) {
                    flags |= 0x10;
                }
            }
            if (level == 0 && have < row->materials_0x04[i].count_0x02) {
                flags |= 8;
            }
        }
    }
    return flags;
}

/* 0x80213740 (0x130): the same bits for a gun-part make row (its conditions in the low nibbles, the high nibbles 3..7
 * marking the level-only materials it skips). */
u16 lb_gun_recipe_material_flags_get(Q_UserData* user, LbSmithGunRow* row, u8 level)
{
    u16 flags;
    s32 i;
    s32 have;

    flags = 0;
    for (i = 0; i < 4; i++) {
        if (((row->cond_0x1C[i] & 0xF0) >> 4) - 3 > 4U && row->materials_0x04[i].item_0x00 != 0 &&
            row->materials_0x04[i].count_0x02 > 0) {
            u8 boxed = userdata_box_item_ck((LbItemDb*)user, row->materials_0x04[i].item_0x00) == 1;
            if (level == 0) {
                have = userdata_item_count_total(row->materials_0x04[i].item_0x00, lobby_world_block);
            }
            if (row->kind_0x00 == 0 && i == 0) {
                s32 unlock = row->cond_0x1C[i] & 0xF;
                if (unlock - 2 <= 1U && boxed == 0) {
                    flags |= 0x10;
                }
                if (lb_recipe_cond_ck(user, unlock) == 0) {
                    flags |= 0x10;
                }
            }
            if (level == 0 && have < row->materials_0x04[i].count_0x02) {
                flags |= 8;
            }
        }
    }
    return flags;
}

u16 lb_seisan_row_flags_get(u16 kind, u16 idx, u8 level)
{
    PlWeaponRec* gun;
    PlSkillRec* blade;
    u16 item;
    Q_UserData* user;
    u8 locked;
    u16 flags;
    LbSmithRow* row;
    u32 gun_kind;

    user = (Q_UserData*)lobby_world_block;
    flags = 0;
    gun_kind = kind - 11;
    if (gun_kind <= 2) {
        row = (LbSmithRow*)&((LbSmithGunRow*)seisan_data[kind])[idx];
        locked = row->kind_0x00;
        item = row->item_0x02;
    } else {
        row = &((LbSmithRow*)seisan_data[kind])[idx];
        locked = row->kind_0x00;
        item = row->item_0x02;
        if ((u8)equip_kind_table_class(kind) == 0) {
            PlSlotRec* rec = equip_slot_rec_find(kind, item);
            if (rec != NULL) {
                if (user->hunter_sex_0x0000 == 0) {
                    if ((rec->wear_flags & 1) == 0) {
                        flags |= 0x10;
                    }
                } else if ((rec->wear_flags & 2) == 0) {
                    flags |= 0x10;
                }
            }
        }
    }
    if (item == 0) {
        flags |= 0x10;
    }
    if (locked == 0 && (u8)equip_kind_table_class(kind) - 1 <= 1U) {
        u8 cond;
        equip_weapon_recs_find(kind, item, &blade, &gun);
        if (blade != NULL) {
            cond = ((LbBladeView*)blade)->unlock_0x14;
        } else {
            cond = ((LbGunView*)gun)->unlock_0x16;
        }
        if (lb_recipe_cond_ck(user, cond) == 0) {
            flags |= 0x10;
        }
    }
    if (gun_kind <= 2) {
        flags |= lb_gun_recipe_material_flags_get(user, (LbSmithGunRow*)row, level);
    } else {
        flags |= lb_recipe_material_flags_get(user, row, level);
    }
    if (level != 0) {
        return flags;
    }
    if (lb_seisan_price_get(kind, item) > user->zenny_0x18) {
        flags |= 4;
    }
    if (userdata_equip_box_free_count() == 0) {
        flags |= 2;
    }
    return flags;
}

u16 lb_deco_row_flags_get(u16 idx)
{
    Q_UserData* user;
    LbSmithRow* row;
    u16 flags;
    s16 pouch;
    s16 box;

    user = (Q_UserData*)lobby_world_block;
    row = &((LbSmithRow*)deco_sei_tbl_ptr)[idx];
    flags = lb_recipe_material_flags_get(user, row, 0);
    if (eft052_item_half_get(row->item_0x02) > user->zenny_0x18) {
        flags |= 4;
    }
    pouch = eft052_page_count_ck(row->item_0x02, 1);
    if (pouch < 0) {
        pouch = 0;
    }
    box = eft052_page_count_ck(row->item_0x02, 0);
    if (box < 0) {
        box = 0;
    }
    if (pouch + box == 0) {
        flags |= 2;
    }
    return flags;
}

void lb_kakou_materials_get(u8 kind, u16 item, u8 level, LbItemCount* out)
{
    _EQUIP equip;
    LbArmorExView* rec;

    rec = (LbArmorExView*)equip_armor_ex_rec_find(kind, item);
    out[0].item_0x00 = 0;
    out[0].count_0x02 = 0;
    out[1].item_0x00 = 0;
    out[1].count_0x02 = 0;
    out[2].item_0x00 = 0;
    out[2].count_0x02 = 0;
    out[3].item_0x00 = 0;
    out[3].count_0x02 = 0;
    lb_row_init(&equip, kind, item);
    switch (equip_level_stage_get(&equip, level)) {
    case 0:
        out[0].item_0x00 = 0x147;
        out[0].count_0x02 = 1;
        break;
    case 1:
        out[0].item_0x00 = 0x148;
        out[0].count_0x02 = 1;
        break;
    case 2:
        out[0].item_0x00 = 0x1BC;
        out[0].count_0x02 = 1;
        break;
    case 3:
        out[0].item_0x00 = 0x1BD;
        out[0].count_0x02 = 1;
        break;
    case 4:
        out[0].item_0x00 = 0x1BE;
        out[0].count_0x02 = 1;
        break;
    case 5:
        out[0].item_0x00 = rec->materials_0x0C[0].item_0x00;
        out[0].count_0x02 = rec->materials_0x0C[0].count_0x02;
        out[1].item_0x00 = rec->materials_0x0C[1].item_0x00;
        out[1].count_0x02 = rec->materials_0x0C[1].count_0x02;
        out[2].item_0x00 = rec->materials_0x0C[2].item_0x00;
        out[2].count_0x02 = rec->materials_0x0C[2].count_0x02;
        out[3].item_0x00 = rec->materials_0x0C[3].item_0x00;
        out[3].count_0x02 = rec->materials_0x0C[3].count_0x02;
        break;
    }
}

u16 lb_kakou_armor_flags_get(u8 kind, u16 item, u8 level)
{
    Q_UserData* user;
    LbItemCount mats[4];
    _EQUIP equip;
    s32 i;
    u16 flags;
    LbItemCount* mat;

    user = (Q_UserData*)lobby_world_block;
    flags = 0;
    lb_row_init(&equip, kind, item);
    if (lb_armor_upgrade_price_get(&equip, level) > user->zenny_0x18) {
        flags |= 4;
    }
    lb_kakou_materials_get(kind, item, level, mats);
    for (i = 0; i < 4; i++) {
        if (mats[i].item_0x00 != 0 && mats[i].count_0x02 > 0 &&
            userdata_item_count_total(mats[i].item_0x00, lobby_world_block) < mats[i].count_0x02) {
            flags |= 8;
        }
    }
    return flags;
}

void lb_seisan_materials_get(u8 kind, u16 item, u8 level, LbItemCount* out)
{
    LbGunView* gun;
    LbSmithGunRow* row;
    s32 bonus;
    s16 i;
    s16 n;

    gun = (LbGunView*)equip_weapon_rec_find(11, item);
    for (row = (LbSmithGunRow*)seisan_data[kind]; row->item_0x02 != 0xFFFF; row++) {
        if (row->item_0x02 == item) {
            break;
        }
    }
    bonus = gun->bonus_row_0x03 * 7;
    out[0].item_0x00 = 0;
    out[0].count_0x02 = 0;
    out[1].item_0x00 = 0;
    out[1].count_0x02 = 0;
    out[2].item_0x00 = 0;
    out[2].count_0x02 = 0;
    out[3].item_0x00 = 0;
    out[3].count_0x02 = 0;
    n = 0;
    for (i = 0; i < 6; i++) {
        switch ((row->cond_0x1C[i] & 0xF0) >> 4) {
        case 0:
            if (level != 0) {
                continue;
            }
            break;
        case 1:
            if (level == 0) {
                break;
            }
            if (gun_level_bonus_tbl[bonus] != 0) {
                continue;
            }
            if (level < 4) {
                break;
            }
            continue;
        case 2:
            if (level == 0 || gun_level_bonus_tbl[bonus] != 0 || level > 3) {
                break;
            }
            continue;
        case 3:
            if (level != 0 && (gun_level_bonus_tbl[bonus] != 0 || level > 2)) {
                break;
            }
            continue;
        case 4:
            if (level != 0 && (gun_level_bonus_tbl[bonus] != 0 || level >= 6) &&
                (gun_level_bonus_tbl[bonus] != 1 || level >= 3)) {
                break;
            }
            continue;
        case 5:
            if (level != 0 && gun_level_bonus_tbl[bonus] == 0 && level < 4) {
                break;
            }
            continue;
        case 6:
            if (level != 0 && (gun_level_bonus_tbl[bonus] != 0 || level > 3)) {
                break;
            }
            continue;
        case 7:
            if (level != 0) {
                break;
            }
            continue;
        }
        if (n <= 3) {
            out->item_0x00 = row->materials_0x04[i].item_0x00;
            out->count_0x02 = row->materials_0x04[i].count_0x02;
            out++;
            n++;
        }
    }
}

u16 lb_kakou_gun_flags_get(u8 kind, u16 item, u8 level)
{
    Q_UserData* user;
    LbItemCount mats[4];
    _EQUIP equip;
    s32 i;
    u16 flags;
    LbItemCount* mat;

    user = (Q_UserData*)lobby_world_block;
    flags = 0;
    lb_row_init(&equip, kind, item);
    if (lb_gun_upgrade_price_get(&equip, level) > user->zenny_0x18) {
        flags |= 4;
    }
    lb_seisan_materials_get(kind, item, level, mats);
    for (i = 0; i < 4; i++) {
        if (mats[i].item_0x00 != 0 && userdata_item_count_total(mats[i].item_0x00, lobby_world_block) < mats[i].count_0x02) {
            flags |= 8;
        }
    }
    return flags;
}

/* 0x80214078 (0xBC): the money (4) and material (8) refusal bits of the weapon upgrade recipe `idx` of kind `kind`. */
u16 lb_kakou_weapon_cost_flags_get(u8 kind, u16 idx)
{
    Q_UserData* user;
    LbItemCount* mat;
    u16 flags;
    s32 i;

    user = (Q_UserData*)lobby_world_block;
    flags = 0;
    mat = ((LbSmithKakouRow*)kakou_data[kind])[idx].materials_0x00;
    if (lb_seisan_price_get(kind, idx) > user->zenny_0x18) {
        flags |= 4;
    }
    for (i = 0; i < 3; i++, mat++) {
        if (mat->item_0x00 != 0 && mat->count_0x02 > 0 &&
            userdata_item_count_total(mat->item_0x00, lobby_world_block) < mat->count_0x02) {
            flags |= 8;
        }
    }
    return flags;
}

u16 lb_kakou_weapon_flags_get(u8 kind, u16 idx, _EQUIP* equip)
{
    LbItemCount species[3];
    u16 decos[3];
    u16 flags;
    s32 count;
    s32 room;
    s32 kinds;
    s32 i;

    kinds = 0;
    flags = lb_kakou_weapon_cost_flags_get(kind, idx);
    count = equip_deco_count(equip);
    room = (u16)item_slots_free_count(((Q_UserData*)lobby_world_block)->box_0x180, userdata_box_capacity(lobby_world_block));
    if (room < count) {
        u16* deco;
        LbItemCount* entry;
        species[0].item_0x00 = 0;
        species[0].count_0x02 = 0;
        species[1].item_0x00 = 0;
        species[1].count_0x02 = 0;
        species[2].item_0x00 = 0;
        species[2].count_0x02 = 0;
        equip_deco_ids_get(equip, decos, 1);
        for (i = 0, deco = decos, entry = species; i < count; i++, deco++, entry++) {
            u16 id = get_item_species_data(*deco)->item_id;
            s32 found = 0;
            s32 j;
            for (j = 0; j < i; j++) {
                if (id == species[j].item_0x00) {
                    species[j].count_0x02++;
                    found = 1;
                    break;
                }
            }
            if (found == 0) {
                entry->item_0x00 = id;
                entry->count_0x02 = 1;
                kinds++;
            }
        }
        if (room < kinds) {
            s32 full = room * 99;
            for (i = 0, entry = species; i < kinds; i++, entry++) {
                if (item_slots_room_get(entry->item_0x00, ((Q_UserData*)lobby_world_block)->box_0x180,
                                        userdata_box_capacity(lobby_world_block)) - full < entry->count_0x02) {
                    room--;
                }
                if ((s32)room < 0) {
                    flags |= 0x100;
                    break;
                }
            }
        }
    }
    return flags;
}

/* 0x802142D8 (0x2CC): builds the upgrade results of the box piece `equip` into `out` (four records) and their prices
 * into `prices`; returns the refusal state (1 no upgrade, 2 a gun part, 4 a gunner piece, 8 no path, 0x40 the cap). */
s32 lb_kakou_icon_build(_EQUIP* equip, s32 wearer, _EQUIP* out, s32* prices)
{
    Q_UserData* user;
    s32 state;
    s32 i;
    _EQUIP* rec;
    s32* price;

    user = (Q_UserData*)lobby_world_block;
    state = 0;
    for (i = 0, rec = out, price = prices; i < 4; i++, rec++, price++) {
        memset(rec, 0, sizeof(_EQUIP));
        *price = 0;
    }
    switch ((u8)equip_kind_table_class(equip->kind)) {
    case 0:
        if (wearer == 0) {
            state |= 1;
        } else if (equip_armor_ex_rec_get(equip) == NULL) {
            state |= 1;
        } else if (equip->deco_count >= equip_level_cap_get(equip)) {
            state |= 0x40;
        } else {
            out->kind = equip->kind;
            out->item_id = equip->item_id;
            out->deco_count = equip->deco_count + 1;
            out->deco_level = equip->deco_level;
            out->skill_id[0] = equip->skill_id[0];
            out->skill_id[1] = equip->skill_id[1];
            out->skill_id[2] = equip->skill_id[2];
            *prices = lb_armor_upgrade_price_get(out, 0xFF);
        }
        break;
    case 1:
        if (wearer == 1) {
            state |= 4;
        } else {
            switch (equip->kind) {
            case 11:
            if (equip->deco_count >= gun_frame_level_cap_get(equip)) {
                state |= 0x40;
            } else {
                out->kind = equip->kind;
                out->item_id = equip->item_id;
                out->deco_count = equip->deco_count + 1;
                out->deco_level = equip->deco_level;
                out->skill_id[0] = equip->skill_id[0];
                out->skill_id[1] = equip->skill_id[1];
                out->skill_id[2] = equip->skill_id[2];
                *prices = lb_gun_upgrade_price_get(out, 0xFF);
            }
            break;
            default: {
            LbSmithKakouRow* path = &((LbSmithKakouRow*)kakou_data[equip->kind])[equip->item_id];
            u16 found = 0;
            u16* next = path->next_0x0C;
            for (i = 0; i < 4; i++, next++) {
                u16 id = *next;
                if (id != 0 &&
                    lb_recipe_cond_ck(user, ((LbBladeView*)equip_blade_rec_find(equip->kind, id))->unlock_0x14) == 1) {
                    found++;
                    out->kind = equip->kind;
                    out->item_id = id;
                    out->deco_count = 0;
                    out->deco_level = 0;
                    out->skill_id[0] = 0;
                    out->skill_id[1] = 0;
                    out->skill_id[2] = 0;
                    *prices = lb_seisan_price_get(equip->kind, id);
                    out++;
                    prices++;
                    if (userdata_equip_flag_ck(equip->kind, id, 1) == 0) {
                        userdata_equip_flag_set(equip->kind, id, 1);
                        userdata_equip_flag_set(equip->kind, id, 2);
                    }
                }
            }
            if (found == 0) {
                state |= 8;
            }
            break;
            }
            }
        }
        break;
    case 2:
        state |= 2;
        break;
    }
    return state;
}

/* ------------------------------------------------------------------------------------------------ */
/* The menu list, choice box and help panel helpers. */

/* 0x802145A4 (0xB0): sizes a table-built list from its longest row string and its row count. */
void lb_list_size_calc(LbList* list)
{
    s16 i;
    s32 width;

    width = 0;
    for (i = 0; i < list->count_0x06; i++) {
        s32 len = flfntStrLen((char*)LbStr(0, (u16)(list->str_base_0x0A + i)));
        if (width < len) {
            width = len;
        }
    }
    list->width_0x1C = (width >> 1) * 18 + 60;
    if (list->width_0x1C < 200) {
        list->width_0x1C = 200;
    }
    list->height_0x1E = i * 26 + 40;
}

void lb_list_init(LbList* list, s16 count, u8 kind, s16 str_base, s16 help_str, u16 off_mask, s32 se_ok, u32 flags,
                  u16 blink_mask)
{
    memset(list, 0, sizeof(LbList));
    list->count_0x06 = count;
    list->kind_0x08 = kind;
    list->str_base_0x0A = (s16)str_base;
    list->help_str_0x0C = help_str;
    list->help_tbl_0x10 = NULL;
    list->off_mask_0x02 = off_mask;
    list->blink_mask_0x04 = blink_mask;
    list->se_ok_0x14 = se_ok;
    list->flags_0x18 = flags;
    if (kind == 0xFF) {
        lb_list_size_calc(list);
    }
}

void lb_choice_init(LbList* list, const LbListTmpl* tmpl, u16 off_mask, u16 blink_mask)
{
    memset(list, 0, sizeof(LbList));
    list->count_0x06 = tmpl->count_0x00;
    list->kind_0x08 = tmpl->kind_0x02;
    list->str_base_0x0A = tmpl->str_base_0x04;
    list->help_str_0x0C = tmpl->help_str_0x06;
    list->help_tbl_0x10 = tmpl->help_tbl_0x08;
    list->off_mask_0x02 = off_mask;
    list->blink_mask_0x04 = blink_mask;
    list->se_ok_0x14 = tmpl->se_ok_0x0C;
    list->flags_0x18 = tmpl->flags_0x10;
    if (list->kind_0x08 == 0xFF) {
        lb_list_size_calc(list);
    }
}

s32 lb_choice_step(LbList* list)
{
    s32 result;

    result = 0;
    if (lb_cmd_pressed_ck(0x10) != 0) {
        if ((list->off_mask_0x02 & (1 << list->cursor_0x00)) == 0) {
            result = 1;
            if (list->se_ok_0x14 != -1) {
                sysSE_req(list->se_ok_0x14);
            }
        } else {
            sysSE_req(2);
        }
    } else if (lb_cmd_pressed_ck(0x20) != 0) {
        result = 2;
        sysSE_req(1);
    } else if (lb_cmd_repeat_ck(3) != 0) {
        list->cursor_0x00 = menu_cursor_step_open_last(list->cursor_0x00, list->count_0x06, lb_cmd_repeat_get(), 1, 2,
                                                       list->flags_0x18);
    }
    return result;
}

/* 0x8021487C (0xCC): puts the blinking marker beside row sprite `row` of panel `panel`. */
void lb_row_blink_mark_draw(u16 panel, u16 row)
{
    _mh_ivec2_ pos;
    _SPR_DATA_* spr;
    s16 x;
    s16 y;

    get_lsp_data(panel, &pos);
    spr = get_lsp_data(row, NULL);
    x = (f32)(pos.x + spr->pos.x) - 24.0f;
    y = (f32)(pos.y + spr->pos.y) - 3.0f;
    put_itemicon(x, y, 30, 0xDC1E1EFF, 0x73);
}

void lb_list_draw_ex(LbList* list, u16 panel, const u16* sprites, const u16* rows, u16 help_panel, u8 flag,
                     const u16* strs);

void lb_choice_draw(LbList* list, u16 panel, const u16* sprites, const u16* rows, u16 help_panel, u8 flag)
{
    lb_list_draw_ex(list, panel, sprites, rows, help_panel, flag, NULL);
}

/* 0x8021495C (0x1D4): draws `list`'s rows on panel `panel`, their strings from `strs` or the list's string run, and
 * the cursor row's help line on `help_panel`. */
void lb_list_draw_ex(LbList* list, u16 panel, const u16* sprites, const u16* rows, u16 help_panel, u8 flag,
                     const u16* strs)
{
    _mh_ivec2_ pos;
    bool active;
    s32 bit;
    s32 i;
    u32 color;
    s32 sel;
    const u16* str;

    get_lsp_data(panel, &pos);
    draw_sprite_anim_ary(sprites, list->count_0x06, &pos);
    active = flag == 0;
    for (i = 0, str = strs; i < list->count_0x06; i++) {
        bool cur;
        if (list->cursor_0x00 == i) {
            sel = active;
            cur = 1;
        } else {
            sel = 0;
            cur = 0;
        }
        bit = 1 << i;
        color = GetMenuFontColor((list->off_mask_0x02 & bit) == 0, cur, active, 0);
        lb_frame_draw_at(panel, *rows,
                         (s8*)LbStr(0, strs == NULL ? (u16)(list->str_base_0x0A + i) : *str), sel, color,
                         list->kind_0x08);
        if ((list->blink_mask_0x04 & bit) != 0 && system_w.field_0x0c % 48 < 24) {
            lb_row_blink_mark_draw(panel, *rows);
        }
        str++;
        rows++;
    }
    if (list->help_str_0x0C != -1) {
        lb_panel_msg_draw(help_panel, (s16)(list->help_str_0x0C + list->cursor_0x00));
        if (list->help_tbl_0x10 != NULL && (list->off_mask_0x02 & (1 << list->cursor_0x00)) != 0 &&
            ((LbHelpPair*)list->help_tbl_0x10)[list->cursor_0x00].str_0x00 != -1) {
            lb_panel_line_draw(help_panel, ((LbHelpPair*)list->help_tbl_0x10)[list->cursor_0x00].str_0x00, 2,
                               ((LbHelpPair*)list->help_tbl_0x10)[list->cursor_0x00].row_0x02);
        }
    }
}

/* 0x80214B30 (0x1D0): draws `list` as a titled dialog at (`x`, `y`), or at the screen's top right for (0, 0). */
void lb_list_dialog_draw(LbList* list, s16 x, s16 y, s8* title, bool active)
{
    s32 shadow_x;
    s32 text_x;
    s16 i;
    bool cur;
    u32 color;

    if (x == 0 && y == 0) {
        x = 640 - (list->width_0x1C + 15);
        y = 64;
    }
    x = put_frame_dialog_title(x, y, list->width_0x1C, list->height_0x1E, 0, 0, 0, title, 4);
    y += 20;
    x += 20;
    shadow_x = x + 12;
    text_x = x + 10;
    for (i = 0; i < list->count_0x06; i++) {
        put_frame_separator(x, y, list->width_0x1C - 40, 26, 0);
        if (list->cursor_0x00 == i) {
            cur = 1;
            put_frame_cursor_bar(x, y, list->width_0x1C - 40, 26, 0, 0);
        } else {
            cur = 0;
        }
        color = GetMenuFontColor((list->off_mask_0x02 & (1 << i)) == 0, cur, active, 0);
        font_set_size(18, 18);
        if (cur == 1) {
            font_print_ex(shadow_x, y + 4, 1, (s8*)LbStr(0, (u16)(list->str_base_0x0A + i)));
        }
        flfntSetColor(color);
        font_locate(text_x, y + 3);
        font_print((s8*)LbStr(0, (u16)(list->str_base_0x0A + i)));
        y += 26;
    }
}

/* The count window's frame, digits and arrow sprites. */
u16 lb_count_frame_ids[11] = { 0x14D3, 0x14D4, 0x14CB, 0x14CC, 0x14CD, 0x14CE, 0x14CF, 0x14D0, 0x14D1, 0x14D2, 0xFFFF };
u16 lb_count_digit_ids[8] = { 0x14DC, 0x14DD, 0x14DE, 0x14D6, 0x14D7, 0x14DF, 0x14E0, 0xFFFF };
u16 lb_count_arrow_ids[4] = { 0x14DB, 0x14DA, 0x14D8, 0x14D9 };

void lb_count_window_draw(s32 panel, s32 sub_panel, s32 value, s32 max, s32 price, s32 flag, s32 moved)
{
    _mh_ivec2_ pos;
    char buf[16];
    _SPR_DATA_ spr;
    u16 lit;

    lit = 0;
    get_lsp_data((u16)panel, &pos);
    draw_sprite_ary(lb_count_frame_ids, &pos);
    get_lsp_data((u16)sub_panel, &pos);
    draw_sprite_ary(lb_count_digit_ids, &pos);
    if (chk_pointer() == 0) {
        if ((s16)value > 1) {
            spr_data_copy(&spr, get_lsp_data(lb_count_arrow_ids[0], NULL));
            if (((u16)moved & 2) != 0) {
                spr.color = 0xFAE6BFFF;
                lit = 1;
            } else if (((u16)moved & 4) != 0) {
                lit = 1;
            }
            draw_sprite(spr, &pos);
        }
        if ((s16)value < (s16)max) {
            spr_data_copy(&spr, get_lsp_data(lb_count_arrow_ids[1], NULL));
            if (((u16)moved & 1) != 0) {
                spr.color = 0xFAE6BFFF;
                lit = 1;
            } else if (((u16)moved & 8) != 0) {
                lit = 1;
            }
            draw_sprite(spr, &pos);
        }
    }
    sprintf(buf, "%d", (s16)value);
    draw_font_anim_idx(lb_count_arrow_ids[2], lit, (s8*)buf, 1, &pos);
    if (lb_count_arrow_ids[3] != 0xFFFF) {
        if ((u8)flag == 0) {
            sprintf(buf, "%dz", (s16)value * price);
        } else {
            sprintf(buf, "%dpt", (s16)value * price);
        }
        draw_font_idx(lb_count_arrow_ids[3], (s8*)buf, 2, &pos);
    }
}

s32 lb_panel_msg_draw(s32 panel, s16 str)
{
    return lb_help_text_put((u16)panel, (s8*)LbStr(3, (u16)str));
}

s32 lb_help_text_put(u16 panel, s8* text)
{
    _mh_ivec2_ pos;
    _SPR_DATA_* spr;

    spr = get_lsp_data(panel, &pos);
    draw_window_frame(pos.x, pos.y, spr->width, spr->height, 0x1E1C0DB2);
    font_set_size(18, 18);
    font_print_ex(pos.x + 20, pos.y + 16, 0, text);
}

s32 lb_help_num_put(s32 panel, s32 str, s32 value)
{
    _mh_ivec2_ pos;
    _SPR_DATA_* spr;

    spr = get_lsp_data((u16)panel, &pos);
    draw_window_frame(pos.x, pos.y, spr->width, spr->height, 0x1E1C0DB2);
    font_set_size(18, 18);
    font_print_ex(pos.x + 20, pos.y + 16, 0, (s8*)LbStr(3, (u16)str), value);
}

void lb_panel_str_print(u16 panel, s16 str, s16 color)
{
    _mh_ivec2_ pos;

    get_lsp_data(panel, &pos);
    font_set_size(18, 18);
    font_print_ex(pos.x + 20, pos.y + 36, color, (s8*)LbStr(3, (u16)str));
}

void lb_panel_line_draw(u16 panel, s16 str, s16 color, s32 row)
{
    _mh_ivec2_ pos;

    get_lsp_data(panel, &pos);
    font_set_size(18, 18);
    font_print_ex(pos.x + 20, pos.y + (row * 20 + 16), color, (s8*)LbStr(3, (u16)str));
}

/* The yes/no box's frame and its two cursor rows. */
u16 lb_yes_no_frame_ids[16] = { 0x1882, 0x187A, 0x187B, 0x187C, 0x187D, 0x187E, 0x187F, 0x1880,
                                0x1881, 0x1885, 0x1886, 0x1887, 0x1888, 0x1889, 0x188A, 0xFFFF };
u16 lb_yes_cursor_ids[5] = { 0x188E, 0x188B, 0x188C, 0x188D, 0xFFFF };
u16 lb_no_cursor_ids[5] = { 0x1892, 0x188F, 0x1890, 0x1891, 0xFFFF };

void lb_panel_yes_no_draw(u16 panel, s32 sel)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    bool yes;
    bool no;

    get_lsp_data(panel, &pos);
    draw_sprite_ary(lb_yes_no_frame_ids, &pos);
    if (sel == 0) {
        yes = 1;
        no = 0;
    } else {
        yes = 0;
        no = 1;
    }
    spr_data_copy(&spr, get_lsp_data(0x1883, NULL));
    spr.color = GetMenuFontColor(1, yes, 1, 0);
    draw_font(spr, (s8*)LbStr(1, 0x2D), 1, &pos);
    spr_data_copy(&spr, get_lsp_data(0x1884, NULL));
    spr.color = GetMenuFontColor(1, no, 1, 0);
    draw_font(spr, (s8*)LbStr(1, 0x2E), 1, &pos);
    put_menu_cursor(sel == 0 ? lb_yes_cursor_ids : lb_no_cursor_ids, 0, &pos);
}

/* Prints the number `value` with the layout sprite `id`'s font. */
void draw_font_idx_num(u16 id, s32 value, u32 flags, const _mh_ivec2_* pos)
{
    char buf[32];

    sprintf(buf, "%d", value);
    draw_font_idx(id, (s8*)buf, flags, pos);
}

/* 0x80215310 (0x6C): prints the number `value` with sprite record `spr`'s font. */
void lb_sprite_num_draw(const _SPR_DATA_* spr, s32 value, u32 flags, const _mh_ivec2_* pos)
{
    char buf[32];

    sprintf(buf, "%d", value);
    draw_font(*spr, (s8*)buf, flags, pos);
}

void lb_font_print(s16 x, s16 y, s8* str, u32 color, u32 flags)
{
    if ((flags & 4) != 0) {
        font_print_ex(x + 2, y + 1, 1, str);
    }
    flfntSetColor(color);
    font_locate(x, y);
    font_print(str);
}

/* 0x80215414 (0x94): puts `str` unformatted at (`x`, `y`) in `color`, shadowed when `flags` has bit 2. */
void lb_font_puts(s16 x, s16 y, s8* str, u32 color, u32 flags)
{
    if ((flags & 4) != 0) {
        font_print_plain(x + 2, y + 1, 1, (const char*)str);
    }
    flfntSetColor(color);
    font_locate(x, y);
    flfntFontPuts((char*)str);
}

/* 0x802154A8 (0xB4): prints `str` centred on `cx` with its middle at `cy`, at font size `size`. */
void lb_font_center_print(s16 cx, s32 cy, s8* str, s16 size, u32 color, u32 flags)
{
    s16 x;

    cy -= size >> 1;
    x = menu_text_center_x((char*)str, cx, size);
    font_set_size(size, size);
    if ((flags & 4) != 0) {
        font_print_ex(x + 2, cy + 1, 1, str);
    }
    flfntSetColor(color);
    font_locate(x, cy);
    font_print(str);
}

/* 0x8021555C (0x100): prints the text block `str` centred in layout sprite `id` moved by `ofs`, in palette colour
 * `color` or in `rgba` when it is nonzero. */
void lb_text_center_print(u16 id, s8* str, s16 color, s16 size, const _mh_ivec2_* ofs, u32 rgba)
{
    _SPR_DATA_ spr;
    s16 lines;
    s16 x;
    s16 y;

    font_set_size(size, size);
    spr_data_copy(&spr, get_lsp_data(id, NULL));
    x = menu_text_block_center_x((char*)str, (s16)(spr.pos.x + ((spr.width >> 1) + ofs->x)), size, &lines);
    y = spr.pos.y + ((spr.height >> 1) + ofs->y);
    y -= (s16)(lines * (size >> 1));
    if (rgba == 0) {
        font_color_set(color);
    } else {
        flfntSetColor(rgba);
    }
    font_locate(x, y);
    font_print(str);
}

/* The decide prompt's frame and arrow sprites. */
u16 lb_prompt_frame_ids[7] = { 0x1827, 0x1828, 0x1829, 0x182A, 0x182B, 0x182C, 0xFFFF };
u16 lb_prompt_arrow_ids[7] = { 0x182E, 0x182D, 0x1831, 0x1832, 0x182F, 0x1830, 0xFFFF };

s32 lb_cursor_prompt_draw(u8 kind, _mh_ivec2_* pos)
{
    u16 frame;
    s32 mode;
    u16 anim;

    mode = kind & 0x7F;
    if (mode == 1 && chk_pointer() == 0) {
        frame = (u32)lobby_w.counter_0x030 % 40 + 10;
        anim = 1;
    } else if (mode == 0) {
        frame = 2;
        anim = 2;
    } else {
        frame = 0;
        anim = 0;
    }
    draw_sprite_anim_ary(lb_prompt_frame_ids, anim, pos);
    draw_sprite_anim_idx(0x1825, frame, pos);
    if ((kind & 0x80) == 0) {
        draw_sprite_anim_ary(lb_prompt_arrow_ids, anim, pos);
    }
}

/* The cancel prompt's frame and arrow sprites. */
u16 lb_cancel_frame_ids[7] = { 0x1838, 0x1839, 0x183A, 0x183B, 0x183C, 0x183D, 0xFFFF };
u16 lb_cancel_arrow_ids[5] = { 0x183E, 0x183F, 0x1840, 0x1841, 0xFFFF };

/* 0x80215754 (0xF8): draws the cancel prompt `kind` at `pos` (the decide prompt's twin). */
void lb_cancel_prompt_draw(u8 kind, _mh_ivec2_* pos)
{
    u16 frame;
    s32 mode;
    u16 anim;

    mode = kind & 0x7F;
    if (mode == 1 && chk_pointer() == 0) {
        frame = (u32)lobby_w.counter_0x030 % 40 + 10;
        anim = 1;
    } else if (mode == 0) {
        frame = 2;
        anim = 2;
    } else {
        frame = 0;
        anim = 0;
    }
    draw_sprite_anim_ary(lb_cancel_frame_ids, anim, pos);
    draw_sprite_anim_idx(0x1834, frame, pos);
    if ((kind & 0x80) == 0) {
        draw_sprite_anim_ary(lb_cancel_arrow_ids, anim, pos);
    }
}

void lb_sprite_pulse_draw(const u16* ids, const _mh_ivec2_* pos)
{
    _SPR_DATA_ spr;
    u16 alpha;

    alpha = (s32)(120.0f * cockpit_blink_get()) + 136;
    for (; *ids != 0xFFFF; ids++) {
        set_blendmode(4, 1, 1);
        sprite_frame_apply(&spr, *ids, 0, NULL);
        alpha = (u8)spr.color * alpha >> 8;
        spr.color = (spr.color & 0xFFFFFF00) + alpha;
        draw_sprite(spr, pos);
    }
    set_blendmode(4, 5, 1);
}

s32 lb_button_prompt_draw(s32 kind, _mh_ivec2_* pos)
{
    if ((u32)chk_pointer() != 1) {
        if (get_option_cfg(7) == 0) {
            draw_sprite_anim_idx(0x1821, (u16)kind, pos);
        } else {
            draw_sprite_anim_idx(0x1822, (u16)kind, pos);
        }
        if (kind == 0) {
            set_blendmode(4, 1, 1);
            draw_sprite_anim_idx(0x1823, (u32)lobby_w.counter_0x030 % 30, pos);
            set_blendmode(4, 5, 1);
        }
    }
}

/* The page arrows' sprites. */
u16 lb_panel_arrow_ids[10] = { 0x184B, 0x1849, 0x184A, 0x1843, 0x1846, 0x1845, 0x1844, 0x1848, 0x1847, 0xFFFF };

void lb_page_arrow_draw(u16 panel, s16 page, s16 pages, u16 x, s32 active)
{
    _mh_ivec2_ pos;
    u8 flags;

    flags = 1;
    if (active == 0) {
        flags |= 0x80;
    }
    get_lsp_data(panel, &pos);
    PutPageArrow(lb_panel_arrow_ids, page, pages, x, &pos, flags);
}


void lb_page_arrow_put(s16 x, s16 y, s16 page, s16 pages, u16 moved, s32 active)
{
    _mh_ivec2_ pos;
    u8 flags;

    flags = 1;
    pos.x = x;
    pos.y = y;
    if (active == 0) {
        flags |= 0x80;
    }
    PutPageArrow(lb_panel_arrow_ids, page, pages, moved, &pos, flags);
}

/* Puts the stick page arrows with the held flag set. */
void lb_ana_page_arrow_put(s32 lsp, s32 kind, s32 page, s32 pages, s32 moved, _mh_ivec2_* pos, bool active)
{
    LbPutAnaPageArrow((u16)lsp, (u16)kind, (s16)page, (s16)pages, (u16)moved, pos, active, true);
}

/* The stick page arrows' sprites, their frame and the stick icon. */
u16 lb_ana_arrow_ids[5] = { 0x184D, 0x184F, 0x1851, 0x1852, 0x1853 };
u16 lb_ana_arrow_frame_ids[4] = { 0x1854, 0x1855, 0x1856, 0xFFFF };
u16 lb_ana_stick_ids[2] = { 0x184E, 0xFFFF };

} /* extern "C" */

void LbPutAnaPageArrow(u16 lsp, u16 kind, s16 page, s16 pages, u16 moved, _mh_ivec2_* pos, bool active, bool flag)
{
    _mh_ivec2_ at;
    u16 bits;
    u8 mode;

    bits = 0;
    mode = 0;
    if (active == 0) {
        mode |= 0x80;
    }
    if (lsp == 0) {
        at.x = 0;
        at.y = 0;
    } else {
        get_lsp_data(lsp, &at);
    }
    if (pos != NULL) {
        at.x += pos->x;
        at.y += pos->y;
    }
    if (flag == 1) {
        draw_sprite_anim_ary(lb_ana_arrow_frame_ids, kind, &at);
    }
    if ((moved & 0x800) != 0) {
        bits |= 4;
    }
    if ((moved & 0x400) != 0) {
        bits |= 8;
    }
    PutPageArrow(lb_ana_arrow_ids, page, pages, bits, &at, mode);
    if (chk_pointer() == 0) {
        draw_sprite_ary(lb_ana_stick_ids, &at);
    }
}

extern "C" {

/* The resource points window's frame. */
u16 lb_points_frame_ids[4] = { 0x0192, 0x0193, 0x0194, 0xFFFF };

void lb_points_draw(u16 panel)
{
    _mh_ivec2_ pos;
    char buf[16];

    get_lsp_data(panel, &pos);
    draw_sprite_ary(lb_points_frame_ids, &pos);
    sprintf(buf, "%d", ((Q_UserData*)lobby_world_block)->points_0x3F04);
    draw_font_idx(400, (s8*)buf, 2, &pos);
    sprintf(buf, "%s", LbStr(1, 0x2C));
    draw_font_idx(401, (s8*)buf, 0, &pos);
}

/* The list rows' frame and cursor sprites, one pair per row kind (0..18). */
u16 lb_row0_frame_ids[5] = { 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0xFFFF };
u16 lb_row0_cursor_ids[5] = { 0x00DD, 0x00DA, 0x00DB, 0x00DC, 0xFFFF };
u16 lb_row1_frame_ids[5] = { 0x00E0, 0x00E1, 0x00E2, 0x00E3, 0xFFFF };
u16 lb_row1_cursor_ids[5] = { 0x00E7, 0x00E4, 0x00E5, 0x00E6, 0xFFFF };
u16 lb_row2_frame_ids[5] = { 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0xFFFF };
u16 lb_row2_cursor_ids[5] = { 0x00F1, 0x00EE, 0x00EF, 0x00F0, 0xFFFF };
u16 lb_row3_frame_ids[4] = { 0x00F4, 0x00F5, 0x00F6, 0xFFFF };
u16 lb_row3_cursor_ids[5] = { 0x00FA, 0x00F7, 0x00F8, 0x00F9, 0xFFFF };
u16 lb_row4_frame_ids[2] = { 0x00BD, 0xFFFF };
u16 lb_row4_cursor_ids[5] = { 0x00C1, 0x00BE, 0x00BF, 0x00C0, 0xFFFF };
u16 lb_row5_frame_ids[3] = { 0x00C4, 0x00C5, 0xFFFF };
u16 lb_row5_cursor_ids[5] = { 0x00C9, 0x00C6, 0x00C7, 0x00C8, 0xFFFF };
u16 lb_row6_frame_ids[5] = { 0x00CC, 0x00CD, 0x00CE, 0x00CF, 0xFFFF };
u16 lb_row6_cursor_ids[5] = { 0x00D3, 0x00D0, 0x00D1, 0x00D2, 0xFFFF };
u16 lb_row7_frame_ids[5] = { 0x1EC0, 0x1EC1, 0x1EC2, 0x1EC3, 0xFFFF };
u16 lb_row7_cursor_ids[5] = { 0x1EC7, 0x1EC4, 0x1EC5, 0x1EC6, 0xFFFF };
u16 lb_row8_frame_ids[4] = { 0x1C42, 0x1C43, 0x1C44, 0xFFFF };
u16 lb_row8_cursor_ids[5] = { 0x1C48, 0x1C45, 0x1C46, 0x1C47, 0xFFFF };
u16 lb_row9_frame_ids[4] = { 0x1C4B, 0x1C4C, 0x1C4D, 0xFFFF };
u16 lb_row9_cursor_ids[5] = { 0x1C51, 0x1C4E, 0x1C4F, 0x1C50, 0xFFFF };
u16 lb_row10_frame_ids[4] = { 0x1ED1, 0x1ED2, 0x1ED3, 0xFFFF };
u16 lb_row10_cursor_ids[5] = { 0x1ECD, 0x1ECA, 0x1ECB, 0x1ECC, 0xFFFF };
u16 lb_row16_frame_ids[9] = { 0x2241, 0x2242, 0x2243, 0x2244, 0x2245, 0x2246, 0x2247, 0x2248, 0xFFFF };
u16 lb_row16_cursor_ids[5] = { 0x224F, 0x224C, 0x224D, 0x224E, 0xFFFF };
u16 lb_row11_frame_ids[4] = { 0x0ABA, 0x0ABB, 0x0ABC, 0xFFFF };
u16 lb_row11_cursor_ids[5] = { 0x0AC0, 0x0ABD, 0x0ABE, 0x0ABF, 0xFFFF };
u16 lb_row12_frame_ids[6] = { 0x1E63, 0x1E64, 0x1E65, 0x1E66, 0x1E67, 0xFFFF };
u16 lb_row12_cursor_ids[5] = { 0x1E6B, 0x1E68, 0x1E69, 0x1E6A, 0xFFFF };
u16 lb_row13_frame_ids[5] = { 0x0172, 0x0173, 0x0174, 0x0175, 0xFFFF };
u16 lb_row13_cursor_ids[5] = { 0x0179, 0x0176, 0x0177, 0x0178, 0xFFFF };
u16 lb_row14_frame_ids[5] = { 0x254A, 0x254B, 0x254C, 0x254D, 0xFFFF };
u16 lb_row14_cursor_ids[5] = { 0x2551, 0x254E, 0x254F, 0x2550, 0xFFFF };
u16 lb_row15_frame_ids[5] = { 0x0124, 0x0125, 0x0126, 0x0127, 0xFFFF };
u16 lb_row15_cursor_ids[5] = { 0x012B, 0x0128, 0x0129, 0x012A, 0xFFFF };

u16* lb_row_frame_tbl[19] = {
    lb_row0_frame_ids,  lb_row1_frame_ids,  lb_row2_frame_ids,  lb_row3_frame_ids,  lb_row4_frame_ids,
    lb_row5_frame_ids,  lb_row6_frame_ids,  lb_row7_frame_ids,  lb_row8_frame_ids,  lb_row9_frame_ids,
    lb_row10_frame_ids, lb_row11_frame_ids, lb_row12_frame_ids, lb_row13_frame_ids, lb_row14_frame_ids,
    lb_row15_frame_ids, lb_row16_frame_ids, lb_row16_frame_ids, lb_row16_frame_ids,
};
u16* lb_row_cursor_tbl[19] = {
    lb_row0_cursor_ids,  lb_row1_cursor_ids,  lb_row2_cursor_ids,  lb_row3_cursor_ids,  lb_row4_cursor_ids,
    lb_row5_cursor_ids,  lb_row6_cursor_ids,  lb_row7_cursor_ids,  lb_row8_cursor_ids,  lb_row9_cursor_ids,
    lb_row10_cursor_ids, lb_row11_cursor_ids, lb_row12_cursor_ids, lb_row13_cursor_ids, lb_row14_cursor_ids,
    lb_row15_cursor_ids, lb_row16_cursor_ids, lb_row16_cursor_ids, lb_row16_cursor_ids,
};
u16 lb_row_font_ids[19] = { 0x00D5, 0x00DF, 0x00E9, 0x00F3, 0x00BC, 0x00C3, 0x00CB, 0x1EBF, 0x1C41, 0x1C4A,
                            0x1EC9, 0x0AB9, 0x1E61, 0x0171, 0x2549, 0x0123, 0x224B, 0x224B, 0x224B };

void lb_list_row_draw(s8* text, s32 sel, _mh_ivec2_* pos, u32 color, u8 kind)
{
    _SPR_DATA_ spr;
    u16 flags;

    flags = 0;
    draw_sprite_ary(lb_row_frame_tbl[kind], pos);
    if (kind == 17) {
        draw_sprite_idx(0x2249, pos);
    } else if (kind == 18) {
        draw_sprite_idx(0x224A, pos);
    }
    if (sel != 0) {
        put_menu_cursor(lb_row_cursor_tbl[kind], 0, pos);
        flags = 4;
    }
    if (color != 0) {
        spr_data_copy(&spr, get_lsp_data(lb_row_font_ids[kind], NULL));
        spr.color = color;
        switch (kind) {
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 8:
        case 9:
        case 11:
            flags |= 1;
            break;
        }
        draw_font_align(&spr, text, flags, pos);
    } else {
        draw_font_align_idx(0xD5, text, flags, pos);
    }
}

void lb_frame_draw_at(u16 panel, u16 sub_panel, s8* text, s32 sel, u32 color, u8 mode)
{
    _mh_ivec2_ at;
    _mh_ivec2_ pos;
    _SPR_DATA_* spr;

    get_lsp_data(panel, &pos);
    spr = get_lsp_data(sub_panel, NULL);
    at.x = pos.x + spr->pos.x;
    at.y = pos.y + spr->pos.y;
    lb_list_row_draw(text, sel, &at, color, mode);
}

/* The four hunter-group colours. */
u32 lb_group_colors[4] = { 0xC4330CFF, 0xFFD900FF, 0x99DF0CFF, 0x61CDEBFF };

/* 0x80215E6C (0x18): returns hunter group `id`'s colour. */
u32 lb_group_color_get(u8 id)
{
    return lb_group_colors[id];
}

/* 0x80215E84 (0x108): draws the hunter-group row `id` (0x7F: none) with its colour badge and name. */
void lb_group_row_draw(s32 id, bool enabled, bool sel, bool active, bool marked, u16 anim, _mh_ivec2_* pos)
{
    _SPR_DATA_ spr;
    s32 flags;
    u8 group;
    u16 frame;

    flags = 0;
    group = id & 0x7F;
    draw_sprite_ary(lb_row10_frame_ids, pos);
    spr_data_copy(&spr, get_lsp_data(0x1EC9, NULL));
    spr.color = GetMenuFontColor(enabled, sel, active, marked);
    if (sel != 0 && chk_pointer() == 0) {
        put_menu_cursor(lb_row10_cursor_ids, anim, pos);
        flags = 4;
        frame = 0x1ED0;
    } else {
        frame = 0x1ECF;
    }
    draw_sprite_anim_idx(frame, anim, pos);
    if (group != 0x7F) {
        draw_sprite_idx_color(0x1ECE, lb_group_color_get(group), pos);
        draw_font(spr, get_group2_name_str(group), flags, pos);
    }
}


/* The item cells' frame, cursor and layout tables, one set per cell kind (0..11).  A layout is six sprite ids: the
 * frame, the frame under the cursor, the icon, the name and the one- and two-digit count. */
u16 lb_cell0_cursor_ids[5] = { 0x0108, 0x0105, 0x0106, 0x0107, 0xFFFF };
u16 lb_cell0_layout[6] = { 0x0103, 0x0104, 0x00FF, 0x00FC, 0x00FD, 0x00FE };
u16 lb_cell0_frame_ids[4] = { 0x0100, 0x0101, 0x0102, 0xFFFF };
u16 lb_cell1_frame_ids[5] = { 0x0111, 0x0112, 0x0113, 0x0114, 0xFFFF };
u16 lb_cell1_cursor_ids[5] = { 0x011D, 0x011A, 0x011B, 0x011C, 0xFFFF };
u16 lb_cell9_cursor_ids[5] = { 0x0121, 0x011E, 0x011F, 0x0120, 0xFFFF };
u16 lb_cell1_layout[6] = { 0x0117, 0x0118, 0x010E, 0x010A, 0x010B, 0x010C };
u16 lb_cell9_layout[6] = { 0x0117, 0x0119, 0x010E, 0x010A, 0x010B, 0x010C };
u16 lb_cell2_frame_ids[5] = { 0x0131, 0x0132, 0x0133, 0x0134, 0xFFFF };
u16 lb_cell2_cursor_ids[5] = { 0x013A, 0x0137, 0x0138, 0x0139, 0xFFFF };
u16 lb_cell2_layout[6] = { 0x0135, 0x0136, 0x0130, 0x012D, 0x012E, 0x012F };
u16 lb_cell3_cursor_ids[5] = { 0x0146, 0x0143, 0x0144, 0x0145, 0xFFFF };
u16 lb_cell3_layout[6] = { 0x0141, 0x0142, 0x013D, 0x013C, 0xFFFF, 0xFFFF };
u16 lb_cell3_frame_ids[4] = { 0x013E, 0x013F, 0x0140, 0xFFFF };
u16 lb_cell4_cursor_ids[5] = { 0x0152, 0x014F, 0x0150, 0x0151, 0xFFFF };
u16 lb_cell4_layout[6] = { 0x014D, 0x014E, 0x0149, 0x0148, 0xFFFF, 0xFFFF };
u16 lb_cell4_frame_ids[4] = { 0x014A, 0x014B, 0x014C, 0xFFFF };
u16 lb_cell5_frame_ids[5] = { 0x015B, 0x015C, 0x015D, 0x015E, 0xFFFF };
u16 lb_cell5_cursor_ids[5] = { 0x0162, 0x015F, 0x0160, 0x0161, 0xFFFF };
u16 lb_cell5_layout[7] = { 0x0159, 0x015A, 0x0157, 0x0154, 0x0155, 0x0156, 0x0158 };
u16 lb_cell6_cursor_ids[5] = { 0x016F, 0x016C, 0x016D, 0x016E, 0xFFFF };
u16 lb_cell6_layout[6] = { 0x016A, 0x016B, 0x0165, 0x0164, 0xFFFF, 0xFFFF };
u16 lb_cell6_frame_ids[4] = { 0x0167, 0x0168, 0x0169, 0xFFFF };
u16 lb_cell7_cursor_ids[5] = { 0x2226, 0x2223, 0x2224, 0x2225, 0xFFFF };
u16 lb_cell7_layout[6] = { 0x221D, 0x221E, 0x221F, 0x222D, 0xFFFF, 0xFFFF };
u16 lb_cell7_frame_ids[4] = { 0x2220, 0x2221, 0x2222, 0xFFFF };
u16 lb_cell_pick_cursor_ids[10] = { 0x23FB, 0x23F3, 0x23F4, 0x23F5, 0x23F6, 0x23F7, 0x23F8, 0x23F9, 0x23FA, 0xFFFF };
u16 lb_cell10_cursor_ids[5] = { 0x17EA, 0x17E7, 0x17E8, 0x17E9, 0xFFFF };
u16 lb_cell10_layout[6] = { 0x17E5, 0x17E6, 0x17DF, 0x17DE, 0xFFFF, 0xFFFF };
u16 lb_cell10_frame_ids[4] = { 0x17E2, 0x17E3, 0x17E4, 0xFFFF };
u16 lb_cell11_layout[6] = { 0x25CB, 0x25CC, 0x25CA, 0x25CF, 0xFFFF, 0xFFFF };
u16 lb_cell11_cursor_ids[5] = { 0x25C9, 0x25C6, 0x25C7, 0x25C8, 0xFFFF };
u16 lb_cell11_frame_ids[3] = { 0x25CD, 0x25CE, 0xFFFF };

/* 0x80215F8C (0x564): draws an item cell of kind `kind` (0..11) at `pos`: its frame, icon, name and count, styled by
 * the `flags` bits (0x20 monster, 4 trade goods, 8 species, 1 barter goods, 2 price, 0x10 points, 0x40 pick cursor,
 * 0x80 no count, 0x100 no name, 0x200 `count` is the name's colour). */
void lb_item_cell_draw_layout(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, bool marked,
                              u16 flags, u8 kind)
{
    char buf[32];
    _SPR_DATA_ spr;
    u16* layout;
    u16* frame;
    u16* cursor;
    s32 font_flags;
    s32 no_count;
    u16 id;
    s32 pick;
    s32 monster;

    font_flags = 0;
    no_count = 0;
    switch (kind) {
    case 0:
        layout = lb_cell0_layout;
        frame = lb_cell0_frame_ids;
        cursor = lb_cell0_cursor_ids;
        break;
    case 1:
        layout = lb_cell1_layout;
        frame = lb_cell1_frame_ids;
        cursor = lb_cell1_cursor_ids;
        break;
    case 2:
        layout = lb_cell2_layout;
        frame = lb_cell2_frame_ids;
        cursor = lb_cell2_cursor_ids;
        break;
    case 3:
        layout = lb_cell3_layout;
        frame = lb_cell3_frame_ids;
        cursor = lb_cell3_cursor_ids;
        break;
    case 4:
        layout = lb_cell4_layout;
        frame = lb_cell4_frame_ids;
        cursor = lb_cell4_cursor_ids;
        break;
    case 5:
        layout = lb_cell5_layout;
        frame = lb_cell5_frame_ids;
        cursor = lb_cell5_cursor_ids;
        break;
    case 6:
        layout = lb_cell6_layout;
        frame = lb_cell6_frame_ids;
        cursor = lb_cell6_cursor_ids;
        break;
    case 7:
        layout = lb_cell7_layout;
        frame = lb_cell7_frame_ids;
        cursor = lb_cell7_cursor_ids;
        break;
    case 8:
        layout = lb_cell1_layout;
        frame = lb_cell2_frame_ids;
        cursor = lb_cell2_cursor_ids;
        break;
    case 9:
        layout = lb_cell9_layout;
        frame = lb_cell1_frame_ids;
        cursor = lb_cell9_cursor_ids;
        break;
    case 10:
        layout = lb_cell10_layout;
        frame = lb_cell10_frame_ids;
        cursor = lb_cell10_cursor_ids;
        break;
    case 11:
        layout = lb_cell11_layout;
        frame = lb_cell11_frame_ids;
        cursor = lb_cell11_cursor_ids;
        break;
    }
    pick = flags & 0x40;
    if (pick != 0 && chk_pointer() == 0 && sel && active) {
        cursor = lb_cell_pick_cursor_ids;
        put_menu_cursor(cursor, 0, pos);
    }
    draw_sprite_ary(frame, pos);
    if (sel && chk_pointer() == 0) {
        id = layout[1];
    } else {
        id = layout[0];
    }
    draw_sprite_idx(id, pos);
    if (chk_pointer() == 0 && pick == 0 && sel && active) {
        put_menu_cursor(cursor, 0, pos);
    }
    spr_data_copy(&spr, get_lsp_data(layout[2], NULL));
    spr.color = GetMenuIconColor(enabled, sel, active, marked);
    monster = flags & 0x20;
    if (monster != 0) {
        draw_monstericon_rec(&spr, item, pos);
    } else if ((flags & 4) != 0) {
        switch (lb_goods_tier_get(item)) {
        case 0:
            id = 0x17DF;
            break;
        case 1:
            id = 0x17E1;
            break;
        case 2:
            id = 0x17E0;
            break;
        }
        draw_sprite_idx(id, pos);
    } else if ((flags & 1) != 0) {
        draw_tex_icon_idx(layout[2], 0x22, spr.color, pos);
    } else {
        draw_itemicon_item_id(spr, item, pos);
    }
    spr_data_copy(&spr, get_lsp_data(layout[3], NULL));
    if ((flags & 0x200) != 0 && count != 0) {
        spr.color = count;
    } else {
        spr.color = GetMenuFontColor(enabled, sel, active, marked);
    }
    if (sel) {
        font_flags |= 4;
    }
    if ((flags & 0x100) == 0) {
        if (monster != 0) {
            draw_font(spr, str_tbl_33_get(item), font_flags, pos);
        } else if ((flags & 4) != 0) {
            draw_font(spr, (s8*)LbStr(0, item + 0x15C), font_flags, pos);
        } else if ((flags & 8) != 0) {
            draw_font(spr, item_species_name_str(item), font_flags, pos);
        } else if ((flags & 1) != 0) {
            draw_font(spr, lb_tr_flag_get(item)->name_0x04, font_flags, pos);
        } else {
            if (item == 0) {
                no_count = 1;
            }
            draw_font(spr, (s8*)ItemName(item), font_flags, pos);
        }
    }
    if ((flags & 0x80) == 0) {
        if ((flags & 2) != 0) {
            sprintf(buf, "%dz", count);
        } else if ((flags & 0x10) != 0) {
            sprintf(buf, "%dpt", count);
        } else if (no_count == 1) {
            return;
        } else {
            sprintf(buf, "%d", count);
        }
        if (count < 10) {
            id = layout[4];
        } else {
            id = layout[5];
        }
        if (id != 0xFFFF) {
            s32 num_flags;
            spr_data_copy(&spr, get_lsp_data(id, NULL));
            spr.color = GetMenuFontColor(enabled, sel, active, marked);
            num_flags = 2;
            if (sel) {
                num_flags |= 4;
            }
            draw_font(spr, (s8*)buf, num_flags, pos);
        }
    }
}

/* The page drawers, one per cell kind. */
void lb_item_cell_draw_wide(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 7);
}

void lb_item_cell_draw(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 0);
}

void lb_item_cell_draw_layout1(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 1);
}

void lb_price_cell_draw(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 2);
}

/* 0x802165D0 (0x38): draws a kind-3 item cell. */
void lb_item_cell_draw_layout3(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 3);
}

void lb_item_cell_draw_layout5(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 5);
}

void lb_window_draw(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 6);
}

/* 0x80216678 (0x38): draws a kind-10 item cell. */
void lb_item_cell_draw_layout10(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 10);
}

/* 0x802166B0 (0x64): draws a kind-8 item cell with no count. */
void lb_item_cell_draw_layout8(u16 item, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, 0, pos, enabled, sel, active, false, flags, 8);
}

void lb_item_cell_draw_layout11(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, false, flags, 11);
}

void lb_item_row_draw(u16 item, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, bool marked,
                      u16 flags)
{
    lb_item_cell_draw_layout(item, count, pos, enabled, sel, active, marked, flags, 1);
}

/* 0x8021677C (0x28C): draws equipment cell `equip` at `pos` (kind 5: the decoration layout with its new mark),
 * styled by the `flags` bits (8 no icon, 4 the "none" name, 2 a price). */
void lb_equip_cell_draw_layout(_EQUIP* equip, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active, u8 flags,
                               u8 kind, u8 mark)
{
    char buf[32];
    _SPR_DATA_ spr;
    u16* layout;
    u16* frame;
    u16* cursor;
    u16 new_id;
    s32 font_flags;
    u16 id;

    if ((s32)kind != 5) {
        layout = lb_cell3_layout;
        frame = lb_cell3_frame_ids;
        cursor = lb_cell3_cursor_ids;
        new_id = 0xFFFF;
    } else {
        layout = lb_cell5_layout;
        frame = lb_cell5_frame_ids;
        cursor = lb_cell5_cursor_ids;
        new_id = 0x158;
    }
    draw_sprite_ary(frame, pos);
    draw_sprite_idx(sel ? layout[1] : layout[0], pos);
    if ((flags & 8) == 0) {
        draw_equipicon_idx(layout[2], equip, pos);
        if (new_id != 0xFFFF && system_w.field_0x05 == 0 && mark != 0 && (system_w.field_0x0c & 0x20) != 0 &&
            userdata_equip_rec_flag_ck(equip, 2) == 1) {
            draw_sprite_idx(new_id, pos);
        }
    }
    spr_data_copy(&spr, get_lsp_data(layout[3], NULL));
    spr.color = GetMenuFontColor(enabled, sel, active, false);
    font_flags = 0;
    if (sel) {
        font_flags |= 4;
    }
    if ((flags & 4) != 0) {
        draw_font(spr, (s8*)LbStr(1, 0x30), font_flags, pos);
    } else {
        draw_font(spr, (s8*)GetEquipName(equip->kind, equip->item_id), font_flags, pos);
    }
    if ((flags & 2) != 0) {
        sprintf(buf, "%dz", count);
    } else {
        sprintf(buf, "%d", count);
    }
    if (count < 10) {
        id = layout[4];
    } else {
        id = layout[5];
    }
    if (id != 0xFFFF) {
        s32 num_flags;
        spr_data_copy(&spr, get_lsp_data(id, NULL));
        spr.color = GetMenuFontColor(enabled, sel, active, false);
        num_flags = 2;
        if (sel) {
            num_flags |= 4;
        }
        draw_font(spr, (s8*)buf, num_flags, pos);
    }
    if (sel && active) {
        put_menu_cursor(cursor, 0, pos);
    }
}

void lb_equip_cell_draw_layout3(_EQUIP* equip, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active,
                                u8 flags, u8 mark)
{
    lb_equip_cell_draw_layout(equip, count, pos, enabled, sel, active, flags, 3, mark);
}

void lb_equip_cell_draw_layout5(_EQUIP* equip, s32 count, _mh_ivec2_* pos, bool enabled, bool sel, bool active,
                                u8 flags, u8 mark)
{
    lb_equip_cell_draw_layout(equip, count, pos, enabled, sel, active, flags, 5, mark);
}

/* The item-need list's frames, row sprites and count digits. */
u16 lb_need_icon_frame_ids[6] = { 0x1869, 0x186A, 0x186B, 0x1867, 0x1868, 0xFFFF };
u16 lb_need_count_frame_ids[6] = { 0x1872, 0x1873, 0x1874, 0x1875, 0x1876, 0xFFFF };
u16 lb_need_frame_ids[3] = { 0x1859, 0x185A, 0xFFFF };
u16 lb_need_place_ids[4] = { 0x185B, 0x185C, 0x185D, 0x185E };
u16 lb_need_row_ids[4] = { 0x185F, 0x1860, 0x1861, 0x1862 };
u16 lb_need_have_digit_ids[4] = { 0x186F, 0x1870, 0x1871, 0xFFFF };

void lb_item_need_list_draw(const LbItemCount* items, s8 count, u8 flag, _mh_ivec2_* pos, u8 mode, u16 a, u16 b)
{
    _mh_ivec2_ at;
    u16 need_digit_ids[3] = { 0x186D, 0x186E, 0x186D };
    PlaceRec place;
    u16* row;
    _SPR_DATA_ spr;
    s8 i;
    u16* place_id;
    s32 have;
    u32 color;

    draw_sprite_ary(lb_need_frame_ids, pos);
    if (flag == 0) {
        draw_sprite_idx(0x1858, pos);
    }
    for (i = 0, row = lb_need_row_ids, place_id = lb_need_place_ids; i < count; i++) {
        if (items->item_0x00 != 0) {
            get_lsp_data(*row, &at);
            at.x += pos->x;
            at.y += pos->y;
            draw_sprite_ary(lb_need_count_frame_ids, &at);
            have = userdata_item_count_total(items->item_0x00, lobby_world_block);
            color = GetMenuFontColor(items->count_0x02 <= have, false, true, false);
            draw_number_2digit_ary(need_digit_ids, items->count_0x02, color, &at);
            draw_number_digits_ary(lb_need_have_digit_ids, have, color, &at);
            get_lsp_data(*place_id, &at);
            switch (mode) {
            case 0:
                place_rec_material_set(&place, a, items->item_0x00, items->count_0x02);
                place_rec_alloc_idx(&place, *place_id, pos, 1, 10);
                break;
            case 1:
                place_rec_make_set(&place, a, b, items->item_0x00, items->count_0x02);
                place_rec_alloc_idx(&place, *place_id, pos, 1, 10);
                break;
            case 2:
                place_rec_upgrade_set(&place, a, b, items->item_0x00, items->count_0x02);
                place_rec_alloc_idx(&place, *place_id, pos, 1, 10);
                break;
            case 3:
                place_rec_trade_set(&place, a, 0, 0, items->item_0x00, items->count_0x02);
                place_rec_alloc_idx(&place, *place_id, pos, 0, 9);
                break;
            }
            at.x += pos->x;
            at.y += pos->y;
            draw_sprite_ary(lb_need_icon_frame_ids, &at);
            spr_data_copy(&spr, get_lsp_data(0x1866, NULL));
            if (have < items->count_0x02) {
                spr.color = color;
            }
            draw_font(spr, (s8*)ItemName(items->item_0x00), 0, &at);
            draw_itemicon_item_id_idx(0x1865, items->item_0x00, &at);
        }
        items++;
        row++;
        place_id++;
    }
    ainpc_page_mark_b_draw(0x1863, pos);
}

/* The kakou (upgrade) list's equipment rows. */
u16 lb_kakou_row_ids[4] = { 0x1A5D, 0x1A5E, 0x1A5F, 0x1A60 };

/* 0x80216D50 (0x308): draws the upgrade source of `equips` (kind `kind`) or, with no source, the reason line for
 * `state`. */
void lb_kakou_source_draw(_EQUIP* equips, u8 kind, u16 state, _EQUIP* source, _mh_ivec2_* pos)
{
    _mh_ivec2_ at;
    s32 shown;
    u16 msg;
    u16 sub;

    shown = 0;
    draw_sprite_idx(0x1A59, pos);
    if (equips != NULL && state == 0) {
        switch (kind) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15: {
            s32 i;
            u16* row;
            for (i = 0, row = lb_kakou_row_ids; i < 4; i++) {
                if (equips->kind != 0) {
                    bool ok;
                    shown = 1;
                    ok = lb_kakou_weapon_flags_get(equips->kind, equips->item_id, source) == 0;
                    get_lsp_data(*row, &at);
                    at.x += (s16)(pos->x - 36);
                    at.y += pos->y;
                    lb_equip_cell_draw_layout3(equips, 0, &at, ok, false, true, 0, 0);
                }
                row++;
                equips++;
            }
            break;
        }
        case 11: {
            bool ok = lb_kakou_gun_flags_get(equips->kind, equips->item_id, equips->deco_count) == 0;
            u32 color;
            get_lsp_data(0x1A5C, &at);
            at.x += pos->x;
            at.y += pos->y;
            color = GetMenuFontColor(ok, false, true, false);
            lb_list_row_draw((s8*)LbStr(0, 0x254), 0, &at, color, 15);
            break;
        }
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6: {
            bool ok = lb_kakou_armor_flags_get(equips->kind, equips->item_id, equips->deco_count) == 0;
            u32 color;
            get_lsp_data(0x1A5C, &at);
            at.x += pos->x;
            at.y += pos->y;
            color = GetMenuFontColor(ok, false, true, false);
            lb_list_row_draw((s8*)LbStr(0, 0x254), 0, &at, color, 15);
            break;
        }
        }
    }
    if (shown == 0) {
        msg = 0xFFFF;
        sub = 0xFFFF;
        switch (state) {
        case 1:
            msg = 0x25A;
            sub = 0x25C;
            break;
        case 2:
            msg = 0x257;
            sub = 0x259;
            break;
        case 4:
            msg = 0x25B;
            sub = 0x25C;
            break;
        case 8:
            msg = 0x258;
            sub = 0x259;
            break;
        case 0x40:
            msg = 0x255;
            sub = 0x256;
            break;
        }
        if (msg != 0xFFFF) {
            draw_font_idx(0x1A5A, (s8*)LbStr(0, msg), 8, pos);
        }
        if (sub != 0xFFFF) {
            draw_font_idx(0x1A5B, (s8*)LbStr(0, sub), 1, pos);
        }
    }
}

/* The item detail panel's frames and sprite ids. */
u16 lb_detail_frame_ids[6] = { 0x1487, 0x1488, 0x1489, 0x148A, 0x148B, 0xFFFF };
u16 lb_detail_box_ids[3] = { 0x148C, 0x147F, 0xFFFF };
u16 lb_detail_pouch_ids[5] = { 0x148D, 0x148E, 0x148F, 0x1480, 0xFFFF };
u16 lb_window_frame_ids[7] = { 0x1907, 0x1908, 0x1909, 0x190A, 0x190B, 0x1903, 0xFFFF };
u16 lb_window_have_ids[5] = { 0x1910, 0x1911, 0x1912, 0x190F, 0xFFFF };
u16 lb_detail_lsp_ids[6] = { 0x1484, 0x1481, 0x1482, 0x147E, 0x1485, 0x1486 };
u16 lb_info_frame_ids[6] = { 0x12EF, 0x12F0, 0x12F1, 0x12F2, 0x12F3, 0xFFFF };
u16 lb_info_icon_ids[3] = { 0x12F4, 0x12E7, 0xFFFF };
u16 lb_info_price_ids[5] = { 0x12F5, 0x12F6, 0x12F7, 0x12E8, 0xFFFF };
u16 lb_window_deco_ids[4] = { 0x190D, 0x190E, 0x190C, 0xFFFF };

/* 0x80217058 (0xC0): draws the detail panel of item `item` on panel `panel`, marking the box or the pouch side. */
void lb_item_detail_panel_draw(u16 panel, u16 item, u8 side)
{
    _mh_ivec2_ pos;
    u16 pouch;
    u16 box;

    get_lsp_data(panel, &pos);
    draw_sprite_ary(lb_detail_frame_ids, &pos);
    pouch = side == 1;
    draw_sprite_anim_ary(lb_detail_box_ids, pouch, &pos);
    box = side == 0;
    draw_sprite_anim_ary(lb_detail_pouch_ids, box, &pos);
    lb_item_detail_draw(panel, item, lb_detail_lsp_ids, pouch, box);
}

void lb_item_detail_draw(u16 panel, u16 item, const u16* ids, u16 box_mark, u16 pouch_mark)
{
    u32 have;
    u32 box;
    u32 pouch;
    _mh_ivec2_ pos;
    char buf[16];
    _SPR_DATA_ spr;
    ItemDataRecord* data;
    u32 color;
    s32 total;

    data = GetItemData(item);
    get_lsp_data(panel, &pos);
    if (item != 0) {
        draw_font_idx(ids[0], (s8*)ItemName(item), 1, &pos);
        color = get_rare_color(data->level_0x01);
        spr_data_copy(&spr, get_lsp_data(ids[1], NULL));
        spr.color = color;
        draw_sprite(spr, &pos);
        draw_number_idx(ids[2], data->level_0x01 + 1, color, &pos);
        spr_data_copy(&spr, get_lsp_data(ids[3], NULL));
        draw_itemicon_item_id(spr, item, &pos);
        eft052_page_counts_get(item, &have, &box, &pouch);
        sprintf(buf, "%d", have);
        if (eft052_page_count_ck(item, 0) == 0) {
            spr_data_copy(&spr, get_lsp_data(ids[4], NULL));
            spr.color = 0xFF435DFF;
            draw_font(spr, (s8*)buf, 1, &pos);
        } else {
            draw_font_anim_idx(ids[4], box_mark, (s8*)buf, 1, &pos);
        }
        total = box + pouch;
        if (total > 999) {
            total = 999;
        }
        sprintf(buf, "%d", total);
        draw_font_anim_idx(ids[5], pouch_mark, (s8*)buf, 2, &pos);
    }
}

/* 0x802172DC (0x204): draws item `item`'s info box at `pos`: `count` held (red at the cap `max`) and, for a
 * nonzero `price_mode`, its unit (1) or total price. */
void lb_item_info_draw(u16 item, s32 count, s32 max, u8 price_mode, _mh_ivec2_* pos)
{
    char buf[32];
    _SPR_DATA_ spr;
    ItemDataRecord* data;
    u32 color;

    data = GetItemData(item);
    color = get_rare_color(data->level_0x01);
    draw_sprite_ary(lb_info_frame_ids, pos);
    draw_sprite_ary(lb_info_icon_ids, pos);
    if (item == 0) {
        if (price_mode != 0) {
            draw_sprite_ary(lb_info_price_ids, pos);
        }
    } else {
        draw_sprite_idx(0x12E9, pos);
        draw_font_idx(0x12EC, (s8*)ItemName(item), 1, pos);
        spr_data_copy(&spr, get_lsp_data(0x12E9, NULL));
        spr.color = color;
        draw_sprite(spr, pos);
        draw_number_idx(0x12EA, data->level_0x01 + 1, color, pos);
        spr_data_copy(&spr, get_lsp_data(0x12ED, NULL));
        if (count >= max && max != 0) {
            spr.color = 0xFF435DFF;
            sprintf(buf, "%d", max);
        } else {
            sprintf(buf, "%d", count);
        }
        draw_font(spr, (s8*)buf, 1, pos);
        if (price_mode != 0) {
            s32 price;
            draw_sprite_ary(lb_info_price_ids, pos);
            if (price_mode == 1) {
                price = data->field_0x010;
            } else {
                price = count * data->field_0x010;
            }
            sprintf(buf, "%dz", price);
            draw_font_idx(0x12EE, (s8*)buf, 2, pos);
        }
        spr_data_copy(&spr, get_lsp_data(0x12E6, NULL));
        draw_itemicon_item_id(spr, item, pos);
    }
}

/* 0x802174E0 (0x1C8): draws the decoration window of item `item` at `pos`: held count and the count set in the
 * equipment box. */
void lb_item_window_draw(u16 item, _mh_ivec2_* pos)
{
    u16 decos[3];
    _SPR_DATA_ spr;
    ItemDataRecord* data;
    u16 species;
    u32 color;
    s32 have;
    s32 used;
    u16 i;
    u16 slots;

    data = GetItemData(item);
    draw_sprite_ary(lb_window_frame_ids, pos);
    draw_sprite_ary(lb_window_have_ids, pos);
    draw_sprite_ary(lb_window_deco_ids, pos);
    if (item != 0) {
        draw_font_idx(0x1904, (s8*)ItemName(item), 1, pos);
        color = get_rare_color(data->level_0x01);
        spr_data_copy(&spr, get_lsp_data(0x1900, NULL));
        spr.color = color;
        draw_sprite(spr, pos);
        draw_number_idx(0x1901, data->level_0x01 + 1, color, pos);
        spr_data_copy(&spr, get_lsp_data(0x18FF, NULL));
        draw_itemicon_item_id(spr, item, pos);
        have = userdata_item_count_total(item, lobby_world_block);
        species = data->species;
        used = 0;
        slots = userdata_box_capacity(lobby_world_block);
        for (i = 0; i < slots; i++) {
            _EQUIP* equip = &((Q_UserData*)lobby_world_block)->equip_box_0x0E00[i];
            if (equip->kind != 0) {
                equip_deco_ids_get(equip, decos, 1);
                if (species == decos[0]) {
                    used++;
                }
                if (species == decos[1]) {
                    used++;
                }
                if (species == decos[2]) {
                    used++;
                }
            }
        }
        draw_font_idx_num(0x1906, used, 2, pos);
        draw_font_idx_num(0x1905, have, 2, pos);
    }
}

/* 0x802176A8 (0x210): draws equipment `equip`'s window on panel `panel` with its count (`flags` 1) and price
 * (`flags` 2) lines; bit 4 draws a bowgun as its type icon. */
void lb_equip_cell_draw(u16 panel, _EQUIP* equip, u8 flags)
{
    _mh_ivec2_ pos;
    _EQUIP barrel;
    _EQUIP frame;
    char buf[16];
    u32 color;
    _SPR_DATA_ spr;
    u8 rare;

    get_lsp_data(panel, &pos);
    draw_sprite_ary(lb_window_frame_ids, &pos);
    if (equip->item_id != 0) {
        rare = Get_equip_rare(equip);
        color = get_rare_color(rare);
        if (equip->kind == 11 && (flags & 4) != 0) {
            lb_row_init(&frame, 12, equip->item_id);
            lb_row_init(&barrel, 13, equip->item_id);
            draw_weaponicon_idx(0x18FF, Get_pl_type(equip, &frame), color, &pos);
        } else {
            draw_equipicon_idx(0x18FF, equip, &pos);
        }
        spr_data_copy(&spr, get_lsp_data(0x1900, NULL));
        spr.color = color;
        draw_sprite(spr, &pos);
        draw_number_idx(0x1901, rare + 1, color, &pos);
        draw_font_idx(0x1904, (s8*)GetEquipName(equip->kind, equip->item_id), 1, &pos);
    }
    if ((flags & 1) != 0) {
        draw_sprite_ary(lb_window_have_ids, &pos);
        if (equip->item_id != 0) {
            sprintf(buf, "%d", userdata_equip_box_count(equip->kind, equip->item_id));
            draw_font_idx(0x1905, (s8*)buf, 2, &pos);
        }
    }
    if ((flags & 2) != 0) {
        draw_sprite_ary(lb_info_price_ids, &pos);
        if (equip->item_id != 0) {
            sprintf(buf, "%dz", equip_sell_price_get(equip));
            draw_font_idx(0x12EE, (s8*)buf, 2, &pos);
        }
    }
}

/* 0x802178B8 (0x7C): releases the lobby camera (`on` 0) and lets every lobby NPC pick its talk again. */
s32 lb_scene_npc_refresh(s32 on)
{
    s32 i;

    if (on == 0) {
        lobby_w.state_0x000 = 0;
        lobby_w.active_0x008 = 0;
        camera_lobby_disarm();
    }
    for (i = 0; i < 18; i++) {
        if (lb_npc[i].field_0x000 != 0) {
            lb_npc_talk_select(&lb_npc[i]);
        }
    }
}


s32 lb_panel_close(void)
{
    return lb_scene_npc_refresh(0);
}

/* 0x8021793C (0x8): lets every lobby NPC pick its talk again, keeping the camera. */
s32 lb_scene_npc_refresh_all(void)
{
    return lb_scene_npc_refresh(1);
}

/* 0x80217944 (0x44): whether the armour row of `equip` has every wear flag of `bits`. */
s32 lb_item_flags_ck(_EQUIP* equip, u8 bits)
{
    PlSlotRec* rec = equip_slot_rec_get(equip);
    return (rec->wear_flags & bits) == bits;
}

/* 0x80217988 (0x4C): the same test on armour `id` of kind `kind`. */
s32 lb_equip_flags_ck(u8 kind, u16 id, u8 bits)
{
    PlSlotRec* rec = equip_slot_rec_find(kind, id);
    return (rec->wear_flags & bits) == bits;
}

/* 0x802179D4 (0x130): whether the hunter's worn armour can all be worn with blade `equip` (1 when it can). */
s32 lb_equip_state_get(_EQUIP* equip)
{
    Q_UserData* user;
    u8 bits;

    user = (Q_UserData*)lobby_world_block;
    bits = 0;
    if ((u8)equip_kind_table_class(equip->kind) != 1) {
        return 0;
    }
    if (user->hunter_sex_0x0000 == 0) {
        bits |= 1;
    } else {
        bits |= 2;
    }
    if (equip->kind == 11) {
        bits |= 8;
    } else {
        bits |= 4;
    }
    if (lb_item_flags_ck(&user->armour_0x0044[4], bits) == 0) {
        return 0;
    }
    if (lb_item_flags_ck(&user->armour_0x0044[0], bits) == 0) {
        return 0;
    }
    if (lb_item_flags_ck(&user->armour_0x0044[2], bits) == 0) {
        return 0;
    }
    if (lb_item_flags_ck(&user->armour_0x0044[1], bits) == 0) {
        return 0;
    }
    if (lb_item_flags_ck(&user->armour_0x0044[3], bits) == 0) {
        return 0;
    }
    return lb_item_flags_ck(&user->armour_0x0044[5], bits) != 0;
}

/* 0x80217B04 (0xE0): the price of item `id` of equipment kind `kind` (an armour, blade or bowgun part row's). */
s32 lb_equip_base_price_get(u8 kind, u16 id)
{
    PlSkillRec* blade;
    PlWeaponRec* gun;
    s32 price;
    u8 k;

    price = 0;
    k = kind;
    switch ((u8)equip_kind_table_class(kind)) {
    case 0: {
        LbArmorExView* armor = (LbArmorExView*)equip_armor_ex_rec_find(kind, id);
        if (armor != NULL) {
            price = armor->price_0x08;
        }
        break;
    }
    case 1:
    case 2:
        equip_weapon_recs_find(kind, id, &blade, &gun);
        switch (k) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
            if (blade != NULL) {
                price = ((LbBladeView*)blade)->price_0x04;
            }
            break;
        case 11:
        case 12:
        case 13:
            if (gun != NULL) {
                price = ((LbGunView*)gun)->price_0x0C;
            }
            break;
        }
        break;
    }
    return price;
}

/* 0x80217BE4 (0x84): the summed value of the first `count` decorations set in `equip`. */
s32 lb_equip_deco_value_get(_EQUIP* equip, u8 count)
{
    s32 i;
    s32 sum;

    sum = 0;
    for (i = 0; i < count; i++) {
        if (equip->skill_id[i] != 0) {
            sum += GetItemData(get_item_species_data(equip->skill_id[i])->item_id)->field_0x010;
        }
    }
    return sum;
}

s32 equip_sell_price_get(_EQUIP* equip)
{
    s32 base;
    s32 upgrades;
    s32 i;

    base = lb_equip_base_price_get(equip->kind, equip->item_id);
    upgrades = 0;
    switch ((u8)equip_kind_table_class(equip->kind)) {
    case 0:
        if (equip->kind != 6 && equip->deco_count != 0) {
            for (i = 1; i < equip->deco_count; i++) {
                upgrades += lb_armor_upgrade_price_get(equip, i);
            }
        }
        break;
    case 1:
        if ((s32)equip->kind == 11 && equip->deco_count != 0) {
            for (i = 1; i < equip->deco_count; i++) {
                upgrades += lb_gun_upgrade_price_get(equip, i);
            }
        }
        break;
    }
    if (equip->kind != 6) {
        base += lb_equip_deco_value_get(equip, 3);
    } else {
        base += lb_equip_deco_value_get(equip, equip->deco_count);
    }
    return base + upgrades / 2;
}

s32 lb_seisan_price_get(u8 kind, u16 id)
{
    return lb_equip_base_price_get(kind, id);
}

/* 0x80217DAC (0x2C): the buy price of item `id` of kind `kind` (twice the base price). */
s32 lb_equip_price_get(u8 kind, u16 id)
{
    return lb_equip_base_price_get(kind, id) * 2;
}

/* The armour upgrade price of each upgrade stage: the base price's factor and the added amount. */
f32 lb_armor_upgrade_price_tbl[12] = {
    0.08f, 50.0f, 0.11f, 100.0f, 0.14f, 250.0f, 0.17f, 500.0f, 0.2f, 1500.0f, 1.5f, 4000.0f,
};

/* 0x80217DD8 (0xBC): the price of upgrading armour `equip` to level `level` (0xFF: its own level). */
s32 lb_armor_upgrade_price_get(_EQUIP* equip, u8 level)
{
    LbArmorExView* rec;
    u8 stage;

    rec = (LbArmorExView*)equip_armor_ex_rec_get(equip);
    if (level != 0xFF) {
        stage = equip_level_stage_get(equip, level);
    } else {
        stage = equip_level_stage_get(equip, equip->deco_count);
    }
    return (u32)rec->price_0x08 * lb_armor_upgrade_price_tbl[stage * 2] + lb_armor_upgrade_price_tbl[stage * 2 + 1];
}

/* The bowgun part upgrade price factor of each level. */
f32 lb_gun_upgrade_price_tbl[7] = { 0.5f, 0.2f, 0.3f, 0.8f, 1.5f, 2.0f, 3.0f };

/* 0x80217E94 (0xB8): the price of upgrading bowgun part `equip` to level `level` (0xFF: its own level). */
s32 lb_gun_upgrade_price_get(_EQUIP* equip, u8 level)
{
    LbGunView* rec;
    f32 price;

    rec = (LbGunView*)equip_weapon_rec_get(equip);
    if (level != 0xFF) {
        price = (u32)rec->price_0x0C * lb_gun_upgrade_price_tbl[level];
    } else {
        price = (u32)rec->price_0x0C * lb_gun_upgrade_price_tbl[equip->deco_count];
    }
    return price;
}

s32 lb_row_init(_EQUIP* row, u8 kind, u16 item)
{
    memset(row, 0, sizeof(_EQUIP));
    row->kind = kind;
    row->item_id = item;
}

/* The new-mark animation's sprites. */
u16 lb_mark_anim_ids[14] = { 0x1A07, 0x1A08, 0x1A09, 0x1A0B, 0x1A0C, 0x1A0A, 0x1A05,
                             0x1A06, 0x1A01, 0x1A02, 0x1A03, 0x1A04, 0xFFFF };
u16 lb_mark_glow_ids[4] = { 0x19FE, 0x19FF, 0x1A00, 0xFFFF };

/* 0x80217FA0 (0x6C): draws frame `anim` of the new mark at `pos`, with its glow unless `flag` is set. */
void lb_anim_mark_draw(u8 flag, u16 anim, _mh_ivec2_* pos)
{
    draw_sprite_anim_ary(lb_mark_anim_ids, anim, pos);
    if (flag == 0) {
        draw_sprite_anim_ary(lb_mark_glow_ids, anim, pos);
    }
}

s8* lb_speaker_name_get(u16 id)
{
    switch (id) {
    case 0x17:
    case 0x49:
        return (s8*)lb_pig_name_get(((LbUserNames*)lobby_world_block)->pig_name_0x483A);
    case 0x14:
        return (s8*)lb_cat_name_get(((LbUserNames*)lobby_world_block)->cat_name_0x3F95[0]);
    case 0x15:
        return (s8*)lb_cat_name_get(((LbUserNames*)lobby_world_block)->cat_name_0x3F95[1]);
    case 0x16:
        return (s8*)lb_cat_name_get(((LbUserNames*)lobby_world_block)->cat_name_0x3F95[2]);
    case 1:
        if (userdata_progress_flag_ck(0x74) == 1) {
            return (s8*)LbStr(2, 0x17);
        }
    default:
        return (s8*)LbStr(2, id);
    }
}

/* 0x802180D8 (0x60): the group of equipment kind `kind`: 3 armour, 4 a decoration... (1 blade, 2 gun part), 0 none. */
u8 lb_equip_kind_group_get(u8 kind)
{
    switch (kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        return 1;
    case 11:
    case 12:
    case 13:
        return 2;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return 3;
    case 6:
        return 4;
    default:
        return 0;
    }
}

/* 0x80218138 (0xF48): rebuilds the scroll list `self`'s rows for its kind and mode: the left and right column kinds
 * of each row, the two pages' modes and the row count. */
void lb_scroll_rows_build(LbScrollWork* self)
{
    s16 count;
    s32 i;
    s32 cmp_only;

    count = 0;
    for (i = 0; i < 10; i++) {
        self->left_rows_0x13[i] = 0;
        self->right_rows_0x1D[i] = 0;
    }
    self->paged_0x03 = 0;
    switch (self->kind_0x00) {
    case 3:
        count = 1;
        break;
    case 4:
        count = (s8)menu_equip_page_count(self->owner_0x34, &self->base_0x5C);
        break;
    case 10:
        self->paged_0x03 = 1;
        count = (s8)skill_table_page_count(self->owner_0x34, &self->base_0x5C, &self->base_parts_0x68[0],
                                       &self->base_parts_0x68[1]);
        break;
    case 8:
        self->left_mode_0x04 = 1;
        self->right_mode_0x05 = 1;
        switch (self->mode_0x02) {
        case 5:
            self->left_group_0x11 = 5;
            self->right_group_0x12 = 5;
            count = 1;
            self->left_rows_0x13[0] = 14;
            self->right_rows_0x1D[0] = 15;
            break;
        case 4:
            self->left_group_0x11 = 5;
            count = 1;
            self->left_rows_0x13[0] = 14;
            break;
        }
        break;
    case 1:
        switch (lb_equip_kind_group_get(self->base_0x5C.kind)) {
        case 1:
            self->left_group_0x11 = 1;
            self->right_group_0x12 = 1;
            switch (self->mode_0x02) {
            case 0:
                count = 2;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 3;
                self->right_rows_0x1D[1] = 6;
                break;
            case 1:
                count = 2;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 15;
                self->left_rows_0x13[1] = 2;
                self->right_rows_0x1D[1] = 6;
                break;
            case 2:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 15;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 9:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 16;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 4:
                count = 1;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                break;
            case 3:
            case 8:
                count = 3;
                self->left_mode_0x04 = 7;
                self->right_mode_0x05 = 7;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                break;
            }
            break;
        case 3:
            self->left_group_0x11 = 3;
            self->right_group_0x12 = 3;
            switch (self->mode_0x02) {
            case 0:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                break;
            case 1:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 15;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 2:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 15;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 9:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 16;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 4:
                count = 1;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                break;
            case 3:
            case 8:
                count = 3;
                self->left_mode_0x04 = 7;
                self->right_mode_0x05 = 7;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                break;
            }
            break;
        case 2:
            self->left_group_0x11 = 2;
            self->right_group_0x12 = 2;
            switch (self->mode_0x02) {
            case 1:
            case 2:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 15;
                self->left_rows_0x13[1] = 10;
                self->right_rows_0x1D[1] = 0;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 9:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 16;
                self->left_rows_0x13[1] = 10;
                self->right_rows_0x1D[1] = 0;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 4:
                count = 1;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                break;
            case 3:
            case 8:
                count = 4;
                if (lobby_w.state_0x000 == 6) {
                    self->left_mode_0x04 = 7;
                    self->right_mode_0x05 = 7;
                } else {
                    self->left_mode_0x04 = 1;
                    self->right_mode_0x05 = 1;
                }
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 10;
                self->right_rows_0x1D[1] = 0;
                self->left_rows_0x13[2] = 4;
                self->right_rows_0x1D[2] = 5;
                self->left_rows_0x13[3] = 3;
                self->right_rows_0x1D[3] = 6;
                break;
            }
            break;
        case 4:
            self->left_group_0x11 = 4;
            self->right_group_0x12 = 4;
            switch (self->mode_0x02) {
            case 3:
            case 8:
                count = 3;
                self->left_mode_0x04 = 7;
                self->right_mode_0x05 = 7;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                break;
            case 9:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                break;
            case 4:
                count = 1;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                break;
            }
            break;
        }
        break;
    case 5:
        self->paged_0x03 = 1;
        switch (lb_equip_kind_group_get(self->base_0x5C.kind)) {
        case 1:
            self->left_group_0x11 = 1;
            self->right_group_0x12 = 1;
            switch (self->mode_0x02) {
            case 2:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 15;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 4:
                count = 1;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                break;
            case 3:
                count = 3;
                self->left_mode_0x04 = 7;
                self->right_mode_0x05 = 7;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                break;
            }
            break;
        case 3:
            self->left_group_0x11 = 3;
            self->right_group_0x12 = 3;
            switch (self->mode_0x02) {
            case 2:
                count = 3;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 15;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                break;
            case 4:
                count = 1;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                break;
            case 3:
                count = 3;
                self->left_mode_0x04 = 7;
                self->right_mode_0x05 = 7;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                break;
            }
            break;
        case 2:
            self->left_group_0x11 = 2;
            self->right_group_0x12 = 2;
            switch (self->mode_0x02) {
            case 0:
                count = 6;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 10;
                self->right_rows_0x1D[1] = 0;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                self->left_rows_0x13[3] = 11;
                self->right_rows_0x1D[3] = 0;
                self->left_rows_0x13[4] = 12;
                self->right_rows_0x1D[4] = 0;
                self->left_rows_0x13[5] = 13;
                self->right_rows_0x1D[5] = 0;
                break;
            case 1:
            case 2:
                count = 6;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 15;
                self->left_rows_0x13[1] = 10;
                self->right_rows_0x1D[1] = 0;
                self->left_rows_0x13[2] = 2;
                self->right_rows_0x1D[2] = 6;
                self->left_rows_0x13[3] = 11;
                self->right_rows_0x1D[3] = 0;
                self->left_rows_0x13[4] = 12;
                self->right_rows_0x1D[4] = 0;
                self->left_rows_0x13[5] = 13;
                self->right_rows_0x1D[5] = 0;
                break;
            case 3:
            case 6:
            case 7:
                count = 7;
                if (lobby_w.state_0x000 == 6) {
                    self->left_mode_0x04 = 7;
                    self->right_mode_0x05 = 7;
                } else {
                    self->left_mode_0x04 = 1;
                    self->right_mode_0x05 = 1;
                }
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 10;
                self->right_rows_0x1D[1] = 0;
                self->left_rows_0x13[2] = 4;
                self->right_rows_0x1D[2] = 5;
                self->left_rows_0x13[3] = 3;
                self->right_rows_0x1D[3] = 6;
                self->left_rows_0x13[4] = 11;
                self->right_rows_0x1D[4] = 0;
                self->left_rows_0x13[5] = 12;
                self->right_rows_0x1D[5] = 0;
                self->left_rows_0x13[6] = 13;
                self->right_rows_0x1D[6] = 0;
                break;
            case 4:
                count = 1;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                break;
            }
            break;
        case 4:
            self->left_group_0x11 = 4;
            self->right_group_0x12 = 4;
            switch (self->mode_0x02) {
            case 3:
                count = 3;
                self->left_mode_0x04 = 7;
                self->right_mode_0x05 = 7;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                self->left_rows_0x13[1] = 4;
                self->right_rows_0x1D[1] = 5;
                self->left_rows_0x13[2] = 3;
                self->right_rows_0x1D[2] = 6;
                break;
            case 4:
                count = 1;
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 1;
                self->left_rows_0x13[0] = 1;
                self->right_rows_0x1D[0] = 2;
                break;
            }
            break;
        }
        break;
    case 6:
        self->paged_0x03 = 1;
    case 2:
        self->left_group_0x11 = lb_equip_kind_group_get(self->cmp_0x38.kind);
        self->right_group_0x12 = lb_equip_kind_group_get(self->base_0x5C.kind);
        switch (self->mode_0x02) {
        default:
            self->left_mode_0x04 = 0;
            self->right_mode_0x05 = 0xD;
            cmp_only = 1;
            break;
        case 1:
            self->left_mode_0x04 = 0;
            self->right_mode_0x05 = 0x49;
            cmp_only = 1;
            break;
        case 4:
            self->left_mode_0x04 = 6;
            self->right_mode_0x05 = 0x4F;
            cmp_only = 0;
            break;
        case 2:
            if (self->paged_0x03 == 0) {
                self->left_mode_0x04 = 1;
                self->right_mode_0x05 = 0x49;
            } else {
                self->left_mode_0x04 = 0;
                self->right_mode_0x05 = 0x49;
            }
            cmp_only = 1;
            break;
        case 3:
            if (lobby_w.state_0x000 == 6) {
                if ((self->flags_0x10 & 1) != 0) {
                    self->left_mode_0x04 = 7;
                } else {
                    self->left_mode_0x04 = 6;
                }
                self->right_mode_0x05 = 0xF;
            } else {
                if ((self->flags_0x10 & 1) != 0) {
                    self->left_mode_0x04 = 1;
                } else {
                    self->left_mode_0x04 = 0;
                }
                self->right_mode_0x05 = 0xD;
            }
            cmp_only = 0;
            break;
        case 8:
            if ((self->flags_0x10 & 1) != 0) {
                self->left_mode_0x04 = 7;
            } else {
                self->left_mode_0x04 = 6;
            }
            self->right_mode_0x05 = 0xF;
            cmp_only = 0;
            break;
        case 9:
            if ((self->flags_0x10 & 1) != 0) {
                self->left_mode_0x04 = 1;
            } else {
                self->left_mode_0x04 = 0;
            }
            self->right_mode_0x05 = 0xD;
            cmp_only = 0;
            break;
        }
        switch (self->left_group_0x11) {
        case 3:
        case 4: {
            s16 row;
            s32 left_decos;
            s32 right_decos;
            self->right_rows_0x1D[0] = 1;
            self->left_rows_0x13[0] = 1;
            self->right_rows_0x1D[1] = 4;
            self->left_rows_0x13[1] = 4;
            row = 2;
            if (self->cmp_0x38.kind == 0) {
                left_decos = 0;
            } else {
                left_decos = equip_deco_count(&self->cmp_0x38);
            }
            if (self->base_0x5C.kind == 0) {
                right_decos = 0;
            } else {
                right_decos = equip_deco_count(&self->base_0x5C);
            }
            if (cmp_only == 0) {
                self->right_rows_0x1D[2] = 5;
                self->left_rows_0x13[2] = 5;
                row = 3;
            } else if (left_decos + right_decos != 0) {
                if (left_decos != 0 || self->mode_0x02 == 4) {
                    self->left_rows_0x13[2] = 5;
                } else {
                    self->left_rows_0x13[2] = 17;
                }
                if (right_decos != 0 || self->mode_0x02 == 4) {
                    self->right_rows_0x1D[2] = 5;
                } else {
                    self->right_rows_0x1D[2] = 17;
                }
                row = 3;
            }
            self->right_rows_0x1D[row] = 2;
            self->left_rows_0x13[row] = 2;
            row++;
            self->right_rows_0x1D[row] = 6;
            self->left_rows_0x13[row] = 6;
            count = row + 1;
            break;
        }
        case 1:
        case 2: {
            s16 row;
            s32 left_decos;
            s32 right_decos;
            self->right_rows_0x1D[0] = 1;
            self->left_rows_0x13[0] = 1;
            row = 1;
            if (self->left_group_0x11 == 2 || self->right_group_0x12 == 2) {
                if (self->left_group_0x11 == 2) {
                    self->left_rows_0x13[1] = 7;
                    self->left_rows_0x13[2] = 8;
                    self->left_rows_0x13[3] = 9;
                } else {
                    self->left_rows_0x13[1] = 17;
                    self->left_rows_0x13[2] = 17;
                    self->left_rows_0x13[3] = 17;
                }
                if (self->right_group_0x12 == 2) {
                    self->right_rows_0x1D[1] = 7;
                    self->right_rows_0x1D[2] = 8;
                    self->right_rows_0x1D[3] = 9;
                } else {
                    self->right_rows_0x1D[1] = 17;
                    self->right_rows_0x1D[2] = 17;
                    self->right_rows_0x1D[3] = 17;
                }
                row = 4;
            }
            left_decos = (self->cmp_0x38.kind == 0) ? 0 : equip_deco_count(&self->cmp_0x38);
            if (self->base_0x5C.kind == 0) {
                left_decos = 0;
            }
            if (self->base_0x5C.kind != 0) {
                right_decos = equip_deco_count(&self->base_0x5C);
            }
            if (cmp_only == 0) {
                self->right_rows_0x1D[row] = 4;
                self->left_rows_0x13[row] = 4;
                row++;
                self->right_rows_0x1D[row] = 5;
                self->left_rows_0x13[row] = 5;
                row++;
            } else if (left_decos + right_decos != 0) {
                if (left_decos != 0 || self->mode_0x02 == 4) {
                    self->left_rows_0x13[row] = 5;
                } else {
                    self->left_rows_0x13[row] = 17;
                }
                if (right_decos != 0 || self->mode_0x02 == 4) {
                    self->right_rows_0x1D[row] = 5;
                } else {
                    self->right_rows_0x1D[row] = 17;
                }
                row++;
            }
            self->right_rows_0x1D[row] = 2;
            self->left_rows_0x13[row] = 2;
            row++;
            self->right_rows_0x1D[row] = 6;
            self->left_rows_0x13[row] = 6;
            count = row + 1;
            break;
        }
        }
        break;
    }
    self->count_0x0A = count;
}

void lb_scroll_init(_PLW* owner, LbScrollWork* self, u8 kind, u8 mode)
{
    memset(self, 0, 0x80);
    self->owner_0x34 = owner;
    self->kind_0x00 = kind;
    self->mode_0x02 = mode;
    self->flags_0x10 = 0;
    self->value_0x2C = 0;
}

void lb_scroll_flag_set(LbScrollWork* self, u8 flag)
{
    self->flag_0x01 = flag;
}

/* 0x802190FC (0xC8): copies the compared and base pieces (and their bowgun parts) given into `self` and rebuilds its
 * rows. */
void lb_scroll_tables_set(LbScrollWork* self, _EQUIP* cmp, _EQUIP* cmp_parts, _EQUIP* base, _EQUIP* base_parts)
{
    if (cmp != NULL) {
        memcpy(&self->cmp_0x38, cmp, sizeof(_EQUIP));
    }
    if (cmp_parts != NULL) {
        memcpy(self->cmp_parts_0x44, cmp_parts, sizeof(_EQUIP) * 2);
    }
    if (base != NULL) {
        memcpy(&self->base_0x5C, base, sizeof(_EQUIP));
    }
    if (base_parts != NULL) {
        memcpy(self->base_parts_0x68, base_parts, sizeof(_EQUIP) * 2);
    }
    lb_scroll_rows_build(self);
    if (self->cursor_0x08 >= self->count_0x0A) {
        self->cursor_0x08 = 0;
    }
}

void lb_scroll_kind_set(LbScrollWork* self, u8 kind)
{
    self->kind_0x00 = kind;
    lb_scroll_rows_build(self);
    if (self->cursor_0x08 >= self->count_0x0A) {
        self->cursor_0x08 = 0;
    }
}

void lb_scroll_item_set(LbScrollWork* self, u16 value)
{
    self->value_0x0E = value;
    lb_scroll_rows_build(self);
    if (self->cursor_0x08 >= self->count_0x0A) {
        self->cursor_0x08 = 0;
    }
}

void lb_scroll_materials_set(LbScrollWork* self, s32 value, u16 index)
{
    self->value_0x30 = value;
    self->value_0x0C = index;
}

} /* extern "C" */
