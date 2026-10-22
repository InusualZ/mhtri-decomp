/* lobby/fn_801E0ADC.cpp - the Cha-Cha mask model effect's steps, the eft033 joint effect, and the lobby equipment shop
 *   (screen 3) with the trade-goods sale (screen 4).
 * RANGE. .text 0x801E0ADC-0x801E7530 (77 functions, defined in address order); .data 0x805B75E8-0x805B7CB0, .sdata
 *   0x80791B38-0x80791B70, .sbss 0x80794AA8-0x80794AB0, .sdata2 0x80799788-0x80799870, extab, extabindex.  Three
 *   original TUs (seam requests lobby-801e0adc#1/#2): the mask effect ..0x801E2A9C (its pool ends with a zero word at
 *   0x80799814, its `.data` with `__vt__14Em007ModelCopy` at 0x805B76B8), eft033 0x801E2A9C-0x801E3EE0 (pool from
 *   0x80799818, `.data` from 0x805B76D0), the shop 0x801E3EE0- (`.data` from 0x805B7A88, read by lobby/fn_801E7530).
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off` (retail keeps `em007_model_colors_get`'s unfused `clrlwi` +
 *   `slwi` index scale and the unfused narrow compares).
 * NAMES. No `__FILE__` string covers the range; the map's real names are `eft033_set`/`eft033_set_ofs`.  The file keeps
 *   the map's stem until the seam re-draw (request lobby-801e0adc#3 proposes enemy/em007_model_eft, ef/eft033 and
 *   lobby/lb_equip_shop).  GUESS (from each body and its callers): the em007_model_eft_* steps (after
 *   enemy/em007_act.cpp's em007_model_eft_* scheme), em007_model_colors_get, em007_model_copy_bind, eft033_set_gun,
 *   eft033_alloc/_release/_step/_create/_move/_move_joint/_move_gun/_fade/_retire, the lb_eqshop_* shop rows,
 *   lb_shop_row_unlocked_ck, lb_weapon_price_get, lb_pl_model_turn_reset/_step, the lb_goods_* sale rows, item_price_get,
 *   lb_title_frame_draw, spr_data_copy; the data em007_model_colors_a/b, em007_model_job_busy, the eft033_* tables, the
 *   lb_eqshop_* tables, lb_title_frame_sprites/_rows.  Foreign names this unit proposed are in the name registry.
 * RESIDUALS. 4 rows unwritten: 0x801E0D38-0x801E1A2C (`em007_model_eft_update`, 0xCF4), 0x801E2260-0x801E2674
 *   (`em007_model_eft_job_draw`, 0x414), 0x801E2864-0x801E2A9C (`em007_model_copy_bind` 0x90 and
 *   `Em007ModelCopy::ExecCallbackB` 0x1A8: the class and its 0x60 work are defined in enemy/em007_act.cpp, request
 *   lobby-801e0adc#4; MaskEftView is this unit's view of that work until then).
 *   - `toggle_word_step`'s fifth argument is a u16 in retail (`clrlwi r7` after the constant; request #5):
 *     lb_eqshop_weapon_step, lb_eqshop_armor_step, lb_goods_sale_step, lb_goods_hold_step.
 *   - `eft052_page_count_add`'s delta is s32 in retail (no `extsh`; request #6): lb_goods_hold_give.
 *   - an early `return` retail compiles `bne next; b exit`, ours `beq exit`: lb_pl_model_turn_step, eft033_move_gun.
 *   - register colouring only: lb_eqshop_weapon_page_build, lb_eqshop_armor_page_build, lb_eqshop_goods_draw,
 *     lb_title_frame_draw, lb_eqshop_help_draw, eft033_create, eft033_move_joint (the second loop counter), and
 *     lb_eqshop_goods_screen_draw (the stats page's mode argument).
 *   - lb_eqshop_weapon_step: retail recomputes the goods row after `userdata_equip_mask_get`, ours keeps it in r29.
 *   - eft_em_spawn_joint: retail compares id 0x21 signed (`cmpwi`); the switch spelling of the test loses more (95.3).
 *   - em007_model_colors_get: retail indexes each colour byte off the scaled index (`lbzx`), ours adds displacements.
 *   - em007_model_eft_draw: one register.
 *   flipcheck: `.data`/`.sdata`/`.sbss` claimed, not emitted, `.sdata2` emitted in source order (not retail's); `.text`/
 *   extab/extabindex short of the claim.  The trade tables (0x805BA660-0x805BA750) are an unclaimed hole read by this
 *   unit and lobby/fn_801E7530 (src/unsplit/lb_trade_rows.h, request #8).  LbShopListTmpl is the 0x14 template
 *   `lb_choice_init` reads (lobby/lb_list.h states 0x18, request #7).
 * SHAPES. A one-value test retail compares signed (`cmpwi`) is a `switch` (eft033_set/_set_ofs, eft033_move,
 *   eft033_create, lb_goods_sale_all, the armour-set test of the shop steps).  A u8 counter decremented in place is
 *   `field--` (a `field = tmp - 1` adds a `clrlwi`, em007_model_eft_dance).  A helper whose count reaches the caller in r3
 *   returns it (lb_goods_price_group_get).  The shop draws' `if`/`else` keep retail's block order (the shared tail last).
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "mh3_pad.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_8012BDF4.h"
#include "stage/stg_w.h"
#include "Pl/Pl_master_ck.h"
#include "ef/eft002.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "g3d/g3d_calcview.h"
#include "g3d/g3d_calcworld.h"
#include "camera/camera.h"
#include "rotLocalMatX.h"
#include "fn_8004CAD8/mtx.h"
#include "g3d/g3d_scnroot.h"
#include "sound/fn_800DD1F0.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "Pl/pl_stagger_level_get.h"
#include "Pl/plw.h"
#include "Pl/Get_equip_rare.h"
#include "Pl/fn_8027D684.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_list_init.h"
#include "lobby/lb_talk_page_open.h"
#include "menu/menu_message.h"
#include "sound/fn_800D7F54.h"
#include "userdata_item.h"
#include "enemy/em020_ai.h"
#include "hud/cockpit.h"
#include "fn_8004CAD8.h"
#include "Pl/pl_net_slot_bind.h"
#include "Pl/equip_color_set.h"
#include "color_rgba_copy.h"
#include "ef/fn_800CDB2C.h"
#include "ef/system_lobby_flag_get.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_scroll_kind_set.h"

/* Foreign callees whose owners' headers do not declare them, as plain prototypes (rule 2 debt). */
extern "C" {
void userdata_zenny_add(s32 size);
void fn_8004C038(void* a, void* b);
u32 fn_800D0734(s32 a);
s32 lb_row_init(void*, u8, u16);
void lb_scroll_tables_set(void*, s32, s32, void*, void*);
void lb_equip_compare_set(void*, void*);
s32 lb_unlock_cond_ck(u16);
s32 fn_802754B4(void*);
s32 fn_8027E354(u8, u16);
void eft_res_slot_release(void* obj);
void lb_equip_records_copy(void*, u32, void*, void*, void*);
s32 lb_panel_close(void);
}

#include "ef/lb_eqshop_set_parts.h"
#include "lobby/lb_shop_row_next.h"
#include "lobby/lb_equip_price_get.h"
#include "lobby/lb_equip_kind_ck.h"
#include "lobby/lb_scroll_init.h"
#include "lobby/lb_equip_entry_apply.h"
#include "lobby/lb_equip_flags_ck.h"
#include "lobby/lb_equip_kind_se_set.h"
#include "lobby/lb_list_pane_init.h"
#include "lobby/lb_page_record_empty_ck.h"
#include "userdata_equip_mask_get.h"
#include "hud/layout_types.h"
#include "hud/get_lsp_data.h"
#include "hud/draw_sprite_ary.h"
#include "hud/draw_weaponicon_idx.h"
#include "hud/get_rare_color.h"
#include "hud/draw_lsp_sprite_variant.h"
#include "font/flfnt.h"
#include "main.h"
#include "menu/get_pop_dat_ptr.h"
#include "sound/fn_800E3CBC.h"
#include "sound/set_zmode.h"
#include "fn_80047398.h"
#include "lobby/LbStr.h"
#include "lobby/lb_sort_menu_frame.h"
#include "lobby/lb_title_frame_draw.h"
#include "lobby/lb_equip_page_draw.h"
#include "lobby/lb_equip_cell_draw.h"
#include "lobby/lb_equip_cell_draw_layout5.h"
#include "lobby/lb_equip_bg_draw.h"
#include "lobby/lb_pane_draw.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "ef/eft052.h"
#include "ef/cockpit_hold_block.h"
#include "ef/eft052_hold_cursor_step.h"
#include "Pl/item_bonus_class_get.h"
#include "ai/ainpc.h"
#include "Pl/fn_802693C4.h"
#include "sound/chacha_mask_se13_req.h"
#include "sound/fn_800E3CBC.h"
#include "sound/sound_job_request.h"
#include "g3d/g3d_scnmdl.h"
#include "Pl/Pl_frame_check_alt.h"
#include "Pl/equip_variant_get.h"
#include "g3d/mtx34_rot_xyz_deg.h"
#include "lobby/pl_slot_motion_allow_get.h"
#include "hud/ainpc_page_hold_clear.h"
#include "lobby/lb_npc.h"

/* The two `.data` colour tables `em007_model_colors_get` reads as flat byte runs (0x805B75E8 and 0x805B75F8 are
 * 0x10 B apart, so 15 B of 3-byte entries plus padding). */
extern u8 em007_model_colors_a[];
extern u8 em007_model_colors_b[];

/* This unit's view of the user-data block `lobby_world_block` points at: the money the shop charges and the two
 * 41-entry u16 flag arrays.  Size is an approximation - the block continues past the last named field. */
typedef struct LbEftFlagBlock {
    /* +0x0000 */ u8 unused_0x0000[0x18];
    /* +0x0018 */ s32 money_0x0018;
    /* +0x001C */ u8 unused_0x001C[0xDE4];
    /* +0x0E00 */ _EQUIP box_0x0E00[1000];    /* the equipment box (the count is an approximation) */
    /* +0x3CE0 */ u8 unused_0x3CE0[0xBF8];
    /* +0x48D8 */ u16 set_0x48D8[41];
    /* +0x492A */ u8 unused_0x492A[0xAE];
    /* +0x49D8 */ u16 active_0x49D8[41];
} LbEftFlagBlock; /* size: 0x4A2A (approximate) */

extern LbEftFlagBlock* lobby_world_block;

/* The player status block the item page reads its button flags from (a 0xD40-byte object in the map). */
typedef struct LbPadStatus {
    /* +0x000 */ u8 unused_0x000[0x2C8];
    /* +0x2C8 */ u16 press_0x2C8;
    /* +0x2CA */ u8 unused_0x2CA[0xA76];
} LbPadStatus; /* size: 0xD40 */

extern LbPadStatus Psw;

extern u8 lb_eqshop_weapon_kinds[8];
extern u8 lb_eqshop_armor_kinds[8];

/* One 6-byte row of the shop and trade tables: the goods id (0 ends the table) and two payload ids - the offline and
 * online unlock conditions of a goods row, an armour set's two extra pieces, or a price.  size: 0x6 */
typedef struct LbIdRow6 {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u16 sub_0x04;
} LbIdRow6;

/* One 8-byte row of `lb_trade_pair_rows`: the goods id (0 ends the table) and its prices for the two counts the user
 * data keeps.  size: 0x8 */
typedef struct LbTradePairRow {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u16 price_set_0x02;
    /* +0x04 */ u16 price_active_0x04;
    /* +0x06 */ u16 pad_0x06;
} LbTradePairRow;

/* The lookup record `fn_8027E354` hands back for a slot and an item id: `lobby/fn_80212810.cpp` defines the same record
 * as `LbRefEntry` in its `.cpp` (s32 fields at +0x04/+0x08/+0x0C), so this view keeps its own name until one header
 * holds it (rule 1).  Only +0x08 is read here. */
typedef struct LbItemRef {
    /* +0x00 */ u8 unused_0x00[8];
    /* +0x08 */ u32 value_0x08;
    /* +0x0C */ u8 unused_0x0C[4];
} LbItemRef; /* size: 0x10 (the sibling unit's view of the same record) */


/* This unit's view of the 0x24-byte sprite record (`_SPR_DATA_`, `hud/layout_types.h`) that `spr_data_copy` copies
 * field by field: retail moves the offset pair at +0x04 and the scale pair at +0x0C as single words. */
typedef struct LbItemRec {
    /* +0x00 */ u32 id_0x00;
    /* +0x04 */ u32 value_0x04;
    /* +0x08 */ s16 rate_0x08;
    /* +0x0A */ s16 rate_0x0A;
    /* +0x0C */ u32 data_0x0C;
    /* +0x10 */ u16 count_0x10;
    /* +0x12 */ u8 kind_0x12;
    /* +0x13 */ u8 flag_0x13;
    /* +0x14 */ u32 value_0x14;
    /* +0x18 */ u32 value_0x18;
    /* +0x1C */ u32 value_0x1C;
    /* +0x20 */ u32 value_0x20;
} LbItemRec; /* size: 0x24 */

/* One 8-byte goods row of the shop list `LbEquipShopWork::goods_0x134`: its state bits (0x40 open, 2 the box is
 * full, 4 too expensive), the equipment kind and id, and the price. */
typedef struct LbShopGoods {
    /* +0x00 */ u8 flags_0x00;
    /* +0x01 */ u8 kind_0x01;
    /* +0x02 */ u16 id_0x02;
    /* +0x04 */ u32 price_0x04;
} LbShopGoods; /* size: 0x8 */

/* The sort chooser of the shop: three nested lists, the row-to-value tables of the second and third, the two picked
 * values, the step and the equipment kind it sorts. */
typedef struct LbShopSortWork {
    /* +0x00 */ LbList lists_0x00[3];
    /* +0x60 */ u8 values_b_0x60[0x10];
    /* +0x70 */ u8 values_a_0x70[0x10];
    /* +0x80 */ u8 pick_b_0x80;
    /* +0x81 */ u8 pick_a_0x81;
    /* +0x82 */ u8 step_0x82;
    /* +0x83 */ u8 kind_0x83;
} LbShopSortWork; /* size: 0x84 */

/* This unit's view of the 0x80-byte scroll list `lb_scroll_init` clears (`LbScrollWork` in `lobby/fn_80212810.cpp`):
 * the moved bits and the cursor the release step drives. */
typedef struct LbShopScroll {
    /* +0x00 */ u8 unused_0x00[6];
    /* +0x06 */ u16 moved_0x06;
    /* +0x08 */ s16 cursor_0x08;
    /* +0x0A */ s16 count_0x0A;
    /* +0x0C */ u8 unused_0x0C[0x74];
} LbShopScroll; /* size: 0x80 */

/* The equipment shop's menu work (`lobby_w.menu_0xAC`, cleared over 0x2000 bytes when the shop opens).  Only the
 * fields these bodies reach are named.  Size is an approximation - the block continues past the last named field. */
typedef struct LbEquipShopWork {
    /* +0x000 */ u8 state_0x000;
    /* +0x001 */ u8 step_0x001;
    /* +0x002 */ u8 unused_0x002[2];
    /* +0x004 */ LbList menu_0x004;
    /* +0x024 */ LbList type_list_0x024;
    /* +0x044 */ LbList category_list_0x044;
    /* +0x064 */ u8 kind_0x064;
    /* +0x065 */ u8 unused_0x065[3];
    /* +0x068 */ LbShopSortWork sort_0x068;
    /* +0x0EC */ s16 cursor_0x0EC;
    /* +0x0EE */ u8 unused_0x0EE[4];
    /* +0x0F2 */ s16 page_0x0F2;
    /* +0x0F4 */ s16 pages_0x0F4;
    /* +0x0F6 */ u16 pick_0x0F6;
    /* +0x0F8 */ s16 count_0x0F8;
    /* +0x0FA */ u16 page_moved_0x0FA;
    /* +0x0FC */ s32 confirm_0x0FC;
    /* +0x100 */ u8 held_0x100;
    /* +0x101 */ u8 flag_0x101;
    /* +0x102 */ u8 unused_0x102[2];
    /* +0x104 */ LbList tab_list_0x104;
    /* +0x124 */ u16 box_0x124;
    /* +0x126 */ u16 box_0x126;
    /* +0x128 */ u16 box_0x128;
    /* +0x12A */ u8 unused_0x12A[2];
    /* +0x12C */ _PLW* plw_0x12C;
    /* +0x130 */ s32 arg_0x130;
    /* +0x134 */ LbShopGoods goods_0x134[8];
    /* +0x174 */ u8 pane_0x174[0x234];
    /* +0x3A8 */ LbShopScroll scroll_0x3A8;
    /* +0x428 */ u8 tab_kinds_0x428[8];
    /* +0x430 */ _EQUIP row_0x430;
    /* +0x43C */ _EQUIP sub_0x43C;
    /* +0x448 */ _EQUIP sub_0x448;
    /* +0x454 */ _EQUIP equip_0x454[6];
    /* +0x49C */ _EQUIP equip_0x49C;
    /* +0x4A8 */ _EQUIP equip_0x4A8[2];
    /* +0x4C0 */ s16 time_0x4C0;
} LbEquipShopWork; /* size: 0x4C2 (approximate) */

/* The trade-goods sale's menu work (`lobby_w.menu_0xAC`, lobby screen 4; cleared over 0x2000 bytes when it opens):
 * the step, the page and its arrows, the confirm word, the page starts, the two goods counts, the goods ids and the
 * sale total.  Size is an approximation - the block continues past the last named field. */
typedef struct LbTradeWork {
    /* +0x000 */ u8 unused_0x000;
    /* +0x001 */ u8 step_0x001;
    /* +0x002 */ u8 unused_0x002[0xA];
    /* +0x00C */ s16 page_0x00C;
    /* +0x00E */ s16 pages_0x00E;
    /* +0x010 */ u8 unused_0x010[2];
    /* +0x012 */ u16 page_moved_0x012;
    /* +0x014 */ u8 unused_0x014[4];
    /* +0x018 */ s32 confirm_0x018;
    /* +0x01C */ _PLW* plw_0x01C;
    /* +0x020 */ s32 arg_0x020;
    /* +0x024 */ u8 unused_0x024[0x20];
    /* +0x044 */ s16 page_starts_0x044[4];
    /* +0x04C */ s16 pair_count_0x04C;
    /* +0x04E */ s16 single_count_0x04E;
    /* +0x050 */ u16 ids_0x050[16];
    /* +0x070 */ u16 ids2_0x070[16];
    /* +0x090 */ u8 unused_0x090[0x70];
    /* +0x100 */ s32 total_0x100;
} LbTradeWork; /* size: 0x104 (approximate) */

/* The 0x24-byte work of the joint effect (`_EFT::work_0x38`): its effects, the offset from the joint, the parameter
 * scale, the joint, and the enemy action/sub-state and motion it was spawned in. */
typedef struct Eft033Work {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ nw4r::ef::Effect* effects_0x04[2];
    /* +0x0C */ nw4r::math::VEC3 ofs_0x0C;
    /* +0x18 */ f32 scale_0x18;
    /* +0x1C */ u32 joint_0x1C;
    /* +0x20 */ u8 action_0x20;
    /* +0x21 */ u8 state_sub_0x21;
    /* +0x22 */ u16 motion_0x22;
} Eft033Work; /* size: 0x24 */

/* What `_PLW::physics_0x13C` points at: a word, then the player's model. */
typedef struct Eft033Physics {
    /* +0x00 */ u8 unused_0x00[4];
    /* +0x04 */ MHchar chr_0x04;
} Eft033Physics; /* size: 0x140 (the model's size, an approximation) */

/* The joint effect's per-kind tables: its life, its resource group and id. */
extern s32 eft033_lives[34];
extern u16 eft033_groups[34];
extern u16 eft033_ids[34];
extern f32 eft033_ofs_keys_a[28];
extern f32 eft033_ofs_keys_b[28];
extern f32 eft033_rot_keys_a[14];
extern f32 eft033_rot_keys_b[14];
extern f32 eft033_scale_keys[12];
extern u8 eft033_alpha_keys_high[8];
extern u8 eft033_alpha_keys_low[8];
extern u8 eft033_gun_colors_a[12];
extern u8 eft033_gun_colors_b[12];
extern nw4r::math::VEC3 eft033_gun_angles;

/* This unit's view of the 0x60-byte work of the Cha-Cha mask effect (`Em007EftWork` in `enemy/em007_act.cpp`, which
 * allocates it): the models and effects, the mask state, its spring position, the dance timers and the area. */
typedef struct MaskEftView {
    /* +0x00 */ s32 model_count_0x00;
    /* +0x04 */ s32 effect_count_0x04;
    /* +0x08 */ MHchar* model_0x08;
    /* +0x0C */ nw4r::ef::Effect* effect_0x0C;
    /* +0x10 */ nw4r::ef::Effect* effects_0x10[5];
    /* +0x24 */ class Em007ModelCopy* copy_0x24;
    /* +0x28 */ u8 mode_0x28;
    /* +0x29 */ u8 node_count_0x29;
    /* +0x2A */ u8 kind_0x2A;
    /* +0x2B */ u8 state_0x2B;
    /* +0x2C */ nw4r::math::VEC3 pos_0x2C;
    /* +0x38 */ nw4r::math::VEC3 joint_0x38;
    /* +0x44 */ nw4r::math::VEC3 vel_0x44;
    /* +0x50 */ s32 angle_0x50;
    /* +0x54 */ s32 hold_timer_0x54;
    /* +0x58 */ u8 unused_0x58[2];
    /* +0x5A */ s8 se_timer_a_0x5A;
    /* +0x5B */ s8 se_timer_b_0x5B;
    /* +0x5C */ u8 flag_0x5C;
    /* +0x5D */ u8 area_0x5D;
    /* +0x5E */ u8 unused_0x5E[2];
} MaskEftView; /* size: 0x60 */

/* What `MHchar::g3d_0x10C` points at here: a word, then the scene model the mask is bound to. */
typedef struct MaskG3dWork {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 model_0x04;
} MaskG3dWork; /* size: 0x8 (approximate: +0x04 is the highest offset read) */

/* The mask-pose job's view of its 0x34-byte `PrimRec` (`sound/fn_800E3CBC.h`): the effect it serves sits in the
 * record's +0x2C word. */
typedef struct MaskJobRec {
    /* +0x00 */ u8 unused_0x00[0x2C];
    /* +0x2C */ _EFT* eft_0x2C;
    /* +0x30 */ u8 unused_0x30[4];
} MaskJobRec; /* size: 0x34 */

extern u8 em007_model_job_busy;

/* One 0x14-byte list template of `lb_eqshop_list_tmpls` (the layout `lb_choice_init` reads). */
typedef struct LbShopListTmpl {
    /* +0x00 */ s16 count_0x00;
    /* +0x02 */ u8 kind_0x02;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ s16 str_base_0x04;
    /* +0x06 */ s16 help_str_0x06;
    /* +0x08 */ s16* help_tbl_0x08;
    /* +0x0C */ s32 se_ok_0x0C;
    /* +0x10 */ u32 flags_0x10;
} LbShopListTmpl; /* size: 0x14 */

extern LbShopListTmpl lb_eqshop_list_tmpls[6];
extern u16 lb_eqshop_goods_frame[16];
extern u16 lb_eqshop_goods_titles[16];
extern u16 lb_eqshop_goods_row_lsp[8];
extern u16 lb_eqshop_bg_frame[6];
extern u16 lb_eqshop_bg_frame_wide[8];
extern u16 lb_eqshop_tab_frame[16];
extern u16 lb_eqshop_menu_frame[12];
extern u16 lb_eqshop_menu_rows[8];
extern u16 lb_eqshop_menu_titles[8];
extern u16 lb_title_frame_sprites[16];
extern u16 lb_title_frame_rows[8];
extern u16 lb_eqshop_scroll_frame[4];

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ */

extern "C" {

/* This unit's own bodies that an earlier body calls. */
u32 lb_eqshop_set_parts_get(LbEquipShopWork* self);
void lb_pl_model_turn_step(_PLW* plw);
_EFT* eft033_alloc(u8 area, u8 kind);
void eft033_release(_EFT* eft);
void eft033_step(_EFT* eft);
void eft033_create(_EFT* eft);
void eft033_move(_EFT* eft);
void eft033_move_joint(_EFT* eft);
void eft033_move_gun(_EFT* eft);
void eft033_fade(_EFT* eft);
void eft033_retire(_EFT* eft);
void em007_model_eft_job_draw(PrimRec* job);
void lb_eqshop_draw(void);
void lb_eqshop_scroll_frame_draw(u8 kind);
void spr_data_copy(LbItemRec* dst, const LbItemRec* src);

/* Steps the Cha-Cha mask's spark effects: drops them when the area changes, spawns one every eight frames while the
 * mask is raised (at the companion's hand, or at the mask for the dance), and moves and draws the live ones. */
void em007_model_eft_sparks(_EFT* eft)
{
    MaskEftView* work = (MaskEftView*)eft->work_0x38;
    _AINPC_W* npc = (_AINPC_W*)eft->source_0x30;
    _PLW* plw = npc->plw_0x16C;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 ofs;
    s32 i;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    VEC3_ctor(&ofs);
    if (work->area_0x5D != eft->area_0x44) {
        for (i = 0; i < 5; i++) {
            if (work->effects_0x10[i] != NULL) {
                push_eft_effect_heap_num(&work->effects_0x10[i], 1);
            }
        }
        work->area_0x5D = eft->area_0x44;
        return;
    }
    if (work->state_0x2B != 0 && eft->field_0x07 != 0) {
        if (eft->field_0x10 < 0) {
            switch (eft->field_0x07) {
            case 3:
                cpSetRotMatrix((_CP_VECTOR*)&npc->field_0x190, &mtx);
                copyVec3(&pos, &npc->vec_0x178);
                setVector3(&ofs, 0.0f, -30.0f, 40.0f);
                rotVecY(&ofs, npc->field_0x194);
                break;
            default:
                cpSetRotMatrix((_CP_VECTOR*)&plw->rot_0x54, &mtx);
                copyVec3(&pos, &eft->pos_0x18);
                setVector3(&ofs, 0.0f, 30.0f, 145.0f);
                rotVecY(&ofs, plw->rot_0x54.y);
                break;
            }
            addVec3To(&pos, &ofs);
            mtx34_set_trans(&mtx, &pos);
            for (i = 0; i < 5; i++) {
                if (work->effects_0x10[i] == NULL) {
                    work->effects_0x10[i] = res_eft_create(0xDF, 0x19, 0);
                    if (work->effects_0x10[i] != NULL) {
                        work->effects_0x10[i]->SetRootMtx(mtx);
                        work->effect_count_0x04++;
                    }
                    break;
                }
            }
            eft->field_0x10 = 8;
        }
        eft->field_0x10--;
    }
    if (work->effect_count_0x04 > 0) {
        for (i = 0; i < 5; i++) {
            if (work->effects_0x10[i] != NULL) {
                if (effect_move(work->effects_0x10[i]) == 0) {
                    push_eft_effect_heap_num(&work->effects_0x10[i], 1);
                    work->effect_count_0x04--;
                } else {
                    eft_res_models_spawn(eft, (void**)&work->effects_0x10[i], 1, 1, NULL);
                }
            }
        }
    }
    work->area_0x5D = eft->area_0x44;
}

/* Bumps the counter byte at +0x5 of the record the caller passes. */
void em007_model_eft_next(u8* rec)
{
    rec[5]++;
}

/* Forwards to the effect release helper (a one-instruction tail call). */
void em007_model_eft_retire(void* obj)
{
    eft_res_slot_release(obj);
}

/* Scatters the entry-0 colour of both tables over the first output pair and the indexed colour over the
 * second, or the other way round for kind 1. */
void em007_model_colors_get(u8 kind, u8 idx, u8* out0, u8* out1, u8* out2, u8* out3)
{
    s32 i;
    s32 j;
    s32 k;

    switch (kind) {
    case 0:
        out0[0] = em007_model_colors_a[0];
        out0[1] = em007_model_colors_a[1];
        out0[2] = em007_model_colors_a[2];
        out1[0] = em007_model_colors_b[0];
        out1[1] = em007_model_colors_b[1];
        out1[2] = em007_model_colors_b[2];
        i = idx * 3;
        j = i + 1;
        k = i + 2;
        out2[0] = em007_model_colors_a[i];
        out2[1] = em007_model_colors_a[j];
        out2[2] = em007_model_colors_a[k];
        out3[0] = em007_model_colors_b[i];
        out3[1] = em007_model_colors_b[j];
        out3[2] = em007_model_colors_b[k];
        break;
    case 1:
        out2[0] = em007_model_colors_a[0];
        out2[1] = em007_model_colors_a[1];
        out2[2] = em007_model_colors_a[2];
        out3[0] = em007_model_colors_b[0];
        out3[1] = em007_model_colors_b[1];
        out3[2] = em007_model_colors_b[2];
        i = idx * 3;
        j = i + 1;
        k = i + 2;
        out0[0] = em007_model_colors_a[i];
        out0[1] = em007_model_colors_a[j];
        out0[2] = em007_model_colors_a[k];
        out1[0] = em007_model_colors_b[i];
        out1[1] = em007_model_colors_b[j];
        out1[2] = em007_model_colors_b[k];
        break;
    }
}

/* Follows the player's mask-dance motions: counts the raised masks and their colour step by the motion frames, plays
 * the mask sound every 30 frames, and shows and tints the two mask models. */
void em007_model_eft_dance(_EFT* eft)
{
    _AINPC_W* npc;
    MaskEftView* work;
    _PLW* plw;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 ofs;
    _GXColor col_a;
    _GXColor col_b;
    _GXColor col_c;
    _GXColor col_d;

    work = (MaskEftView*)eft->work_0x38;
    npc = (_AINPC_W*)eft->source_0x30;
    plw = npc->plw_0x16C;
    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    VEC3_ctor(&ofs);
    eft->timer_0x0C++;
    switch (Get_motion_no(plw)) {
    case 0x14D:
        if (Pl_frame_check(plw, 1, 26.0f, 0.0f) == 0) {
            return;
        }
        eft->demo_flag_0x08 |= 1;
        eft->field_0x07 = 1;
        eft->field_0x06 = 0;
        break;
    case 0x14E:
        if (Pl_frame_check(plw, 1, 106.0f, 0.0f) != 0) {
            return;
        }
        if (Pl_frame_check(plw, 0, 2.0f, 0.0f) != 0) {
            eft->demo_flag_0x08 = 0;
            eft->field_0x07 = 0;
        }
        break;
    case 0x14F:
        if (Pl_frame_check(plw, 1, 100.0f, 0.0f) != 0) {
            return;
        }
        if (Pl_frame_check(plw, 0, 78.0f, 0.0f) != 0) {
            eft->field_0x06 = 1;
            eft->timer_0x0C = 0;
        }
        if (Pl_frame_check(plw, 0, 2.0f, 0.0f) != 0) {
            if (eft->field_0x07 == 1) {
                eft->demo_flag_0x08 = 0;
                eft->field_0x07 = 0;
            } else {
                eft->field_0x07--;
                eft->demo_flag_0x08 &= 0xFE;
            }
        }
        break;
    case 0x15A:
        if (Pl_frame_check(plw, 0, 2.0f, 0.0f) != 0) {
            eft->field_0x07--;
            eft->demo_flag_0x08 &= 0xFD;
        }
        if (Pl_frame_check(plw, 1, 64.0f, 0.0f) != 0) {
            eft->field_0x06 = 0;
            eft->timer_0x0C = 0;
        }
        break;
    case 0x15B:
        if (Pl_frame_check(plw, 0, 2.0f, 0.0f) != 0) {
            eft->field_0x07--;
            eft->demo_flag_0x08 &= 0xFE;
        }
        if (Pl_frame_check(plw, 1, 64.0f, 0.0f) != 0) {
            eft->field_0x06 = 1;
            eft->timer_0x0C = 0;
        }
        break;
    case 0x15C:
        if (Pl_frame_check(plw, 0, 2.0f, 0.0f) != 0) {
            if (eft->field_0x07 == 1) {
                eft->demo_flag_0x08 = 0;
                eft->field_0x07 = 0;
            } else {
                eft->field_0x07--;
                eft->demo_flag_0x08 &= 0xFD;
            }
        }
        if (Pl_frame_check(plw, 1, 60.0f, 0.0f) != 0) {
            eft->field_0x06 = 0;
            eft->timer_0x0C = 0;
        }
        break;
    case 0x15D:
        if (Pl_frame_check(plw, 0, 42.0f, 0.0f) != 0) {
            eft->demo_flag_0x08 |= 2;
            eft->field_0x07++;
        }
        break;
    case 0x15E:
        if (Pl_frame_check(plw, 0, 46.0f, 0.0f) != 0) {
            eft->demo_flag_0x08 |= 1;
            eft->field_0x07++;
        }
        break;
    }
    if (work->se_timer_b_0x5B % 30 == 0 && eft->area_0x44 == get_now_areano()) {
        chacha_mask_se13_req(npc->sound_0x498, &eft->pos_0x18);
    }
    work->se_timer_b_0x5B++;
    col_a.a = 0xFF;
    col_b.a = 0xFF;
    col_c.a = 0xFF;
    col_d.a = 0xFF;
    if (plw->field_0x00A == 0 && (u16)(plw->act_no + 0xFF5E) <= 3) {
        em007_model_colors_get(0, 0, &col_a.r, &col_b.r, &col_c.r, &col_d.r);
    } else if (eft->timer_0x0C >= 4) {
        if (eft->field_0x06 == 0) {
            em007_model_colors_get(0, plw->field_0x5BA, &col_a.r, &col_b.r, &col_c.r, &col_d.r);
        } else {
            em007_model_colors_get(1, plw->field_0x5BA, &col_a.r, &col_b.r, &col_c.r, &col_d.r);
        }
    } else if (eft->field_0x06 == 0) {
        em007_model_colors_get(1, plw->field_0x5BA, &col_a.r, &col_b.r, &col_c.r, &col_d.r);
    } else {
        em007_model_colors_get(0, plw->field_0x5BA, &col_a.r, &col_b.r, &col_c.r, &col_d.r);
    }
    work->model_0x08->setVisibility(8, false);
    work->model_0x08->setVisibility(9, false);
    if (eft->demo_flag_0x08 & 1) {
        work->model_0x08->setVisibility(8, true);
        work->model_0x08->setMatColor(1, (_GXChannelID)4, col_c, false);
        work->model_0x08->setMatColor(2, (_GXChannelID)4, col_d, false);
    }
    if (eft->demo_flag_0x08 & 2) {
        work->model_0x08->setVisibility(9, true);
        work->model_0x08->setMatColor(3, (_GXChannelID)4, col_a, false);
        work->model_0x08->setMatColor(4, (_GXChannelID)4, col_b, false);
    }
}

/* Holds the mask raised for the dance's last pose: counts the frames, plays the mask sound every 30 frames and
 * shows the first mask in the companion's colours. */
void em007_model_eft_hold(_EFT* eft)
{
    MaskEftView* work = (MaskEftView*)eft->work_0x38;
    _AINPC_W* npc = (_AINPC_W*)eft->source_0x30;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 ofs;
    _GXColor col_a;
    _GXColor col_b;
    _GXColor col_c;
    _GXColor col_d;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    VEC3_ctor(&ofs);
    work->hold_timer_0x54++;
    eft->field_0x07 = 3;
    if (work->se_timer_b_0x5B % 30 == 0 && eft->area_0x44 == get_now_areano()) {
        chacha_mask_se13_req(npc->sound_0x498, &eft->pos_0x18);
    }
    work->se_timer_b_0x5B++;
    col_a.a = 0xFF;
    col_b.a = 0xFF;
    col_c.a = 0xFF;
    col_d.a = 0xFF;
    em007_model_colors_get(0, npc->field_0x444, &col_a.r, &col_b.r, &col_c.r, &col_d.r);
    work->model_0x08->setVisibility(8, true);
    work->model_0x08->setMatColor(1, (_GXChannelID)4, col_c, false);
    work->model_0x08->setMatColor(2, (_GXChannelID)4, col_d, false);
    work->model_0x08->setVisibility(9, false);
}

/* Draws the mask model `model`: while a mask is raised and the dance motion runs (in the current area, visible),
 * queues the mask-pose job once and binds the model to the scene root; otherwise draws the effect's models. */
void em007_model_eft_draw(_EFT* eft, MHchar* model)
{
    MaskEftView* work = (MaskEftView*)eft->work_0x38;
    _PLW* plw = ((_AINPC_W*)eft->source_0x30)->plw_0x16C;
    MaskJobRec* job;
    u16 motion;

    switch (work->state_0x2B) {
    case 1:
        motion = Get_motion_no(plw);
        if ((u32)(motion - 0x15D) <= 1 || (motion == 0x150 && Pl_frame_check_alt(plw, 1, 0.0f, 0.0f) != 0)) {
            if (eft->area_0x44 != get_now_areano() || eft->flag_0x01 == 0) {
                g3d_root_model_bind(pRoot, model->field_0x118);
                break;
            }
            if (model != NULL && (u32)model->field_0x118 == ((MaskG3dWork*)model->g3d_0x10C)->model_0x04) {
                if (em007_model_job_busy == 0) {
                    job = (MaskJobRec*)sound_job_request(1, model->ready, model->field_0x118, 0, 0x40, NULL,
                                                     em007_model_eft_job_draw);
                    if (job != NULL) {
                        job->eft_0x2C = eft;
                    }
                }
                g3d_root_model_bind(pRoot, model->field_0x118);
            }
            break;
        }
        eft_res_models_spawn(eft, (void**)&work->model_0x08, 2, 1, NULL);
        break;
    case 2:
        if (eft->area_0x44 != get_now_areano() || eft->flag_0x01 == 0) {
            g3d_root_model_bind(pRoot, model->field_0x118);
            break;
        }
        if (model != NULL && (u32)model->field_0x118 == ((MaskG3dWork*)model->g3d_0x10C)->model_0x04) {
            job = (MaskJobRec*)sound_job_request(1, model->ready, model->field_0x118, 0, 0x40, NULL,
                                                     em007_model_eft_job_draw);
            if (job != NULL) {
                job->eft_0x2C = eft;
            }
            g3d_root_model_bind(pRoot, model->field_0x118);
        }
        break;
    default:
        eft_res_models_spawn(eft, (void**)&work->model_0x08, 2, 1, NULL);
        break;
    }
}

} /* extern "C" */

/* Spawns joint effect `kind` on the player `plw` at joint `joint` with the parameter scale `scale` (kinds 1 and 9
 * only for the master player). */
void eft033_set(_PLW* plw, u8 kind, u32 joint, f32 scale)
{
    _EFT* eft;
    Eft033Work* work;

    switch (kind) {
    case 1:
    case 9:
        if (Pl_master_ck(plw) == 0) {
            return;
        }
        break;
    }
    eft = eft033_alloc(plw->area_0x16, kind);
    if (eft != NULL) {
        eft->source_0x30 = plw;
        eft->demo_flag_0x08 = 0;
        eft_state_flags_set(eft, 0, 0);
        eft->timer_0x0C = eft033_lives[eft->type_0x02];
        work = (Eft033Work*)eft->work_0x38;
        work->scale_0x18 = 1.0f;
        work->joint_0x1C = joint;
        setVector3(&work->ofs_0x0C, 0.0f, 0.0f, scale);
    }
}

/* Spawns joint effect `kind` on the player `plw` at joint `joint`, offset by `ofs` and scaled by `scale` (kinds 1
 * and 9 only for the master player). */
void eft033_set_ofs(_PLW* plw, u8 kind, u32 joint, nw4r::math::VEC3* ofs, f32 scale)
{
    _EFT* eft;
    Eft033Work* work;

    switch (kind) {
    case 1:
    case 9:
        if (Pl_master_ck(plw) == 0) {
            return;
        }
        break;
    }
    eft = eft033_alloc(plw->area_0x16, kind);
    if (eft != NULL) {
        eft->source_0x30 = plw;
        eft->demo_flag_0x08 = 0;
        eft_state_flags_set(eft, 0, 0);
        eft->timer_0x0C = eft033_lives[eft->type_0x02];
        work = (Eft033Work*)eft->work_0x38;
        work->scale_0x18 = scale;
        work->joint_0x1C = joint;
        copyVec3(&work->ofs_0x0C, ofs);
    }
}

/* Spawns the master player's gunner joint effect (kind 9) with its fixed offset. */
extern "C" void eft033_set_gun(_PLW* plw)
{
    _EFT* eft;
    Eft033Work* work;

    if (Pl_master_ck(plw) != 0) {
        eft = eft033_alloc(plw->area_0x16, 9);
        if (eft != NULL) {
            eft->source_0x30 = plw;
            eft->demo_flag_0x08 = 0;
            eft_state_flags_set(eft, 1, 0);
            eft->timer_0x0C = eft033_lives[eft->type_0x02];
            work = (Eft033Work*)eft->work_0x38;
            work->scale_0x18 = 1.0f;
            setVector3(&work->ofs_0x0C, 5.0f, 0.0f, 195.0f);
        }
    }
}

extern "C" {

/* Spawns effect `id` at `joint`'s world position when it passes the height test, with `pos` and `scale` in its work
 * block and `life` in the record. */
void eft_em_spawn_joint(struct _ENEMY_WORK* self, u32 joint, u32 id, nw4r::math::VEC3* pos, f32 scale, u32 life)
{
    nw4r::math::VEC3 wpos;
    _EFT* eft;
    Eft033Work* work;

    VEC3_ctor(&wpos);
    get_joint_wpos_em(self, joint, (Vec3*)&wpos);
    if ((u32)((u8)id - 0x19) <= 6 || (u8)id == 0x21 || !(wpos.y > self->field_0x210)) {
        eft = eft033_alloc(self->area_no, id);
        if (eft != NULL) {
            eft->source_0x30 = self;
            eft->demo_flag_0x08 = 1;
            eft->field_0x07 = self->team;
            eft_state_flags_set(eft, 0, 0);
            eft->timer_0x0C = life;
            work = (Eft033Work*)eft->work_0x38;
            copyVec3(&work->ofs_0x0C, pos);
            work->joint_0x1C = joint;
            work->state_sub_0x21 = self->state_sub;
            work->action_0x20 = self->action;
            work->scale_0x18 = scale;
            work->motion_0x22 = em_get_mot_no(self);
            if (event_demo_ck() == 1) {
                eft->field_0x06 = 1;
            }
        }
    }
}

/* Takes a 0x24-byte effect slot in area `area` (the current one only) for joint effect `kind`: one effect, two for
 * kind 9, with the release and dispatch hooks. */
_EFT* eft033_alloc(u8 area, u8 kind)
{
    _EFT* eft;
    Eft033Work* work;

    if (area != get_now_areano()) {
        return NULL;
    }
    eft = (_EFT*)eft_res_slot_get(0x24);
    if (eft == NULL) {
        return NULL;
    }
    work = (Eft033Work*)eft->work_0x38;
    if (kind == 9) {
        work->count_0x00 = 2;
    } else {
        work->count_0x00 = 1;
    }
    eft->field_0x03 = 0x21;
    eft->type_0x02 = kind;
    eft->area_0x44 = area;
    eft->release_0x40 = eft033_release;
    eft->dispatch_0x34 = eft033_step;
    return eft;
}

/* Returns the joint effect's effects to the heap. */
void eft033_release(_EFT* eft)
{
    Eft033Work* work = (Eft033Work*)eft->work_0x38;

    push_eft_effect_heap_num(work->effects_0x04, work->count_0x00);
    work->count_0x00 = 0;
}

/* The joint effect's state dispatcher: create, move, fade, release. */
void eft033_step(_EFT* eft)
{
    switch (eft->state_0x05) {
    case 0:
        eft033_create(eft);
        break;
    case 1:
        eft033_move(eft);
        break;
    case 2:
        eft033_fade(eft);
        break;
    case 3:
        eft033_retire(eft);
        break;
    }
}

/* Creates the joint effect's effects from the kind's resource group and places them once on the source. */
void eft033_create(_EFT* eft)
{
    Eft033Work* work = (Eft033Work*)eft->work_0x38;
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;
    s32 i;
    u16 group;
    u16 id;
    _PLW* plw;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    eft->state_0x05++;
    group = eft033_groups[eft->type_0x02];
    id = eft033_ids[eft->type_0x02];
    switch (eft->type_0x02) {
    case 3:
        switch (eft->field_0x07) {
        case 0x14:
            group = 0x89F;
            id = 0x70;
            break;
        }
        break;
    case 4:
        switch (eft->field_0x07) {
        case 0x14:
            group = 0x8A0;
            id = 0x70;
            break;
        }
        break;
    case 5:
        switch (eft->field_0x07) {
        case 0x14:
            group = 0x8A1;
            id = 0x70;
            break;
        }
        break;
    case 6:
        switch (eft->field_0x07) {
        case 0x14:
            group = 0x8A2;
            id = 0x70;
            break;
        }
        break;
    }
    for (i = 0; i < work->count_0x00; i++) {
        work->effects_0x04[i] = res_eft_create(group, id, 0);
        if (work->effects_0x04[i] == NULL) {
            eft033_retire(eft);
            return;
        }
        group++;
    }
    switch (eft->type_0x02) {
    case 10:
        plw = (_PLW*)eft->source_0x30;
        if (eft_res_spawn_gate_ck(eft, 0) == 0) {
            eft->flag_0x01 = 0;
            eft->state_0x05 = 3;
            return;
        }
        copyVec3(&pos, &work->ofs_0x0C);
        eft_rot_vec_copy(&eft->rot_0x24, (_CP_VECTOR*)&plw->rot_0x54);
        mhchar_joint_mtx_get(&((Eft033Physics*)plw->physics_0x13C)->chr_0x04, work->joint_0x1C, (Mtx34*)&mtx);
        copyVec3(&pos, &work->ofs_0x0C);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &eft->pos_0x18);
        SetRootMtxTrans(work->effects_0x04[0], &eft->pos_0x18);
        break;
    case 19:
    case 24:
        plw = (_PLW*)eft->source_0x30;
        if (eft_res_spawn_gate_ck(eft, 0) == 0) {
            eft->flag_0x01 = 0;
            eft->state_0x05 = 3;
            return;
        }
        copyVec3(&pos, &work->ofs_0x0C);
        eft_rot_vec_copy(&eft->rot_0x24, (_CP_VECTOR*)&plw->rot_0x54);
        cpSetRotMatrix(&eft->rot_0x24, &mtx);
        copyVec3(&pos, &work->ofs_0x0C);
        mulVecMat(&pos, &mtx);
        ((Eft033Physics*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(work->joint_0x1C, &eft->pos_0x18);
        addVec3To(&eft->pos_0x18, &pos);
        mtx34_set_trans(&mtx, &eft->pos_0x18);
        work->effects_0x04[0]->SetRootMtx(mtx);
        break;
    case 20:
        work->scale_0x18 = 0.6f;
        break;
    case 21:
        work->effects_0x04[0]->owner_0x44 = eft;
        break;
    case 31:
        work->scale_0x18 *= get_em_scale((struct _ENEMY_WORK*)eft->source_0x30);
        break;
    }
    eft->flag_0x01 = 1;
    eft033_move(eft);
}

/* One frame of the joint effect: the gunner kind has its own mover. */
void eft033_move(_EFT* eft)
{
    switch (eft->type_0x02) {
    case 9:
        eft033_move_gun(eft);
        break;
    default:
        eft033_move_joint(eft);
        break;
    }
}

/* Moves the joint effect along its source: places it on the joint (the kind's own transform), scales and steps
 * its effects, then tints and spawns the models for the kind; ends it when the life runs out, the demo ends or the
 * enemy dies or changes action. */
void eft033_move_joint(_EFT* eft)
{
    Eft033Work* work = (Eft033Work*)eft->work_0x38;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 unused;
    nw4r::math::VEC3 spawn;
    nw4r::math::MTX34 mtx;
    nw4r::math::MTX34 rot;
    nw4r::math::VEC2 scale;
    _GXColor color;
    _PLW* plw;
    struct _ENEMY_WORK* em;
    u8* alpha;
    u8 level;
    s32 i;
    u32 col;

    VEC3_ctor(&pos);
    VEC3_ctor(&unused);
    VEC3_ctor(&spawn);
    MTX34_ctor(&mtx);
    MTX34_ctor(&rot);
    VEC2_ctor(&scale);
    if (--eft->timer_0x0C < 0) {
        eft->field_0x10 = 30;
        eft->flag_0x01 = 0;
        eft->state_0x05++;
        eft033_fade(eft);
        return;
    }
    if (eft->field_0x06 == 1 && event_demo_ck() == 0) {
        eft->flag_0x01 = 0;
        eft033_retire(eft);
        return;
    }
    switch (eft->demo_flag_0x08) {
    case 0:
        if (eft_res_spawn_gate_ck(eft, 0) == 0) {
            eft->flag_0x01 = 0;
            eft->state_0x05++;
            return;
        }
        plw = (_PLW*)eft->source_0x30;
        eft_rot_vec_copy(&eft->rot_0x24, (_CP_VECTOR*)&plw->rot_0x54);
        mhchar_joint_mtx_get(&((Eft033Physics*)plw->physics_0x13C)->chr_0x04, work->joint_0x1C, (Mtx34*)&mtx);
        break;
    case 1:
        em = (struct _ENEMY_WORK*)eft->source_0x30;
        if (em_work_die_ck(em) != 0) {
            eft->flag_0x01 = 0;
            eft->state_0x05++;
            return;
        }
        if (work->action_0x20 != em->action || work->state_sub_0x21 != em->state_sub) {
            eft->state_0x05++;
            eft->field_0x10 = 30;
            switch (eft->type_0x02) {
            case 30:
                se_req_pos_ps(em->se_0xB14, 0x93, 2, &eft->pos_0x18);
                break;
            }
            eft033_fade(eft);
            return;
        }
        switch (eft->type_0x02) {
        case 29:
            if (work->motion_0x22 != em_get_mot_no(em)) {
                eft->state_0x05++;
                eft->field_0x10 = 30;
                eft033_fade(eft);
                return;
            }
            get_joint_wmat_em(em, work->joint_0x1C, &mtx);
            break;
        case 30:
            shell_se_req(em->se_0xB14, &eft->pos_0x18, 0x1E, em->field_0x01A);
            get_joint_wmat_em(em, work->joint_0x1C, &mtx);
            break;
        case 31:
            cpSetRotMatrix(&eft->rot_0x24, &mtx);
            get_joint_wpos_em(em, work->joint_0x1C, (Vec3*)&eft->pos_0x18);
            mtx34_set_trans(&mtx, &eft->pos_0x18);
            break;
        default:
            get_joint_wmat_em(em, work->joint_0x1C, &mtx);
            break;
        }
        break;
    }
    switch (eft->type_0x02) {
    case 0:
    case 2:
    case 3:
    case 4:
    case 8:
    case 11:
    case 12:
    case 15:
    case 16:
    case 20:
    case 22:
    case 23:
    case 29:
    case 30:
    case 33:
        copyVec3(&pos, &work->ofs_0x0C);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &eft->pos_0x18);
        work->effects_0x04[0]->SetRootMtx(mtx);
        break;
    case 1:
    case 5:
    case 6:
    case 13:
    case 14:
    case 17:
    case 18:
    case 21:
    case 31:
    case 32:
        copyVec3(&pos, &work->ofs_0x0C);
        mulVecMat(&pos, &mtx);
        mtx34_trans_get(&mtx, &eft->pos_0x18);
        addVec3To(&eft->pos_0x18, &pos);
        SetRootMtxTrans(work->effects_0x04[0], &eft->pos_0x18);
        break;
    case 25:
    case 27:
        getKeyData3(eft033_ofs_keys_a, (f32)eft->field_0x10, &pos.x, &pos.y, &pos.z);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &eft->pos_0x18);
        rotLocalMatY((u16)(0.5f + (65536.0f * getKeyData(eft033_rot_keys_a, (f32)eft->field_0x10)) / 360.0f),
                     &mtx);
        work->effects_0x04[0]->SetRootMtx(mtx);
        work->scale_0x18 = getKeyData(eft033_scale_keys, (f32)eft->field_0x10);
        eft->field_0x10++;
        break;
    case 26:
    case 28:
        getKeyData3(eft033_ofs_keys_b, (f32)eft->field_0x10, &pos.x, &pos.y, &pos.z);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &eft->pos_0x18);
        rotLocalMatY((u16)(0.5f + (65536.0f * getKeyData(eft033_rot_keys_b, (f32)eft->field_0x10)) / 360.0f),
                     &mtx);
        work->effects_0x04[0]->SetRootMtx(mtx);
        work->scale_0x18 = getKeyData(eft033_scale_keys, (f32)eft->field_0x10);
        eft->field_0x10++;
        break;
    case 7:
        level = pl_stagger_level_get(plw);
        if (level > 1) {
            switch (level) {
            case 2:
                alpha = eft033_alpha_keys_low;
                work->scale_0x18 = 0.8f;
                break;
            case 3:
                alpha = eft033_alpha_keys_high;
                work->scale_0x18 = 1.3f;
                break;
            }
        } else {
            alpha = eft033_alpha_keys_low;
            work->scale_0x18 = 0.3f;
        }
        copyVec3(&pos, &work->ofs_0x0C);
        mulVecMat(&pos, &mtx);
        eft->field_0x10++;
        eft_rot_vec_copy(&eft->rot_0x24, (_CP_VECTOR*)&plw->rot_0x54);
        color.a = eftGetKeyAlpha(alpha, eft->field_0x10);
        color.b = 0xFF;
        color.g = 0xFF;
        color.r = 0xFF;
        mtx34_identity(&rot);
        rotLocalMatY(eft->rot_0x24.y, &rot);
        eft->pos_0x18.x = rot.m[0][3] = mtx.m[0][3] + pos.x;
        eft->pos_0x18.y = rot.m[1][3] = mtx.m[1][3] + pos.y;
        eft->pos_0x18.z = rot.m[2][3] = mtx.m[2][3] + pos.z;
        for (i = 0; i < work->count_0x00; i++) {
            work->effects_0x04[i]->SetRootMtx(rot);
        }
        break;
    }
    if ((u32)(eft->type_0x02 - 0x19) <= 3) {
        scale.x = scale.y = work->scale_0x18;
        eft_effect_foreach_pm_scale(work->effects_0x04[0], (u32)&scale, 0);
    } else {
        change_paramscale_eff(work->effects_0x04[0], work->scale_0x18);
    }
    for (i = 0; i < work->count_0x00; i++) {
        effect_move(work->effects_0x04[i]);
    }
    switch (eft->type_0x02) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 8:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 22:
    case 23:
    case 24:
        if (get_camera_pos().y < 0.0f) {
            col = get_stg_eft_col(eft->area_0x44, 1);
            color.r = (col & 0xFF000000) >> 24;
            color.g = (col & 0xFF0000) >> 16;
            color.b = (col & 0xFF00) >> 8;
            color.a = 0xFF;
            change_color_eff(work->effects_0x04[0], &eft->pos_0x18, color);
            copyVec3(&spawn, &eft->pos_0x18);
            if (spawn.y > -10.0f) {
                spawn.y = -10.0f;
            }
            eft_res_models_spawn(eft, (void**)work->effects_0x04, 1, work->count_0x00, &spawn);
        }
        break;
    case 7:
        change_color_eff(work->effects_0x04[0], &eft->pos_0x18, color);
        eft_res_models_spawn(eft, (void**)work->effects_0x04, 1, work->count_0x00, NULL);
        break;
    case 25:
    case 26:
    case 27:
    case 28:
        col = get_stg_eft_col(eft->area_0x44, 0);
        color.r = (col & 0xFF000000) >> 24;
        color.g = (col & 0xFF0000) >> 16;
        color.b = (col & 0xFF00) >> 8;
        color.a = 0xFF;
        change_color_eff(work->effects_0x04[0], &eft->pos_0x18, color);
        eft_res_models_spawn(eft, (void**)work->effects_0x04, 1, work->count_0x00, NULL);
        break;
    default:
        eft_res_models_spawn(eft, (void**)work->effects_0x04, 1, work->count_0x00, NULL);
        break;
    }
}

/* Moves the gunner joint effect (kind 9) along the master player's bowgun: the barrel joint (or the stock for the
 * heavy class), the stagger level's tint, then the models. */
void eft033_move_gun(_EFT* eft)
{
    Eft033Work* work = (Eft033Work*)eft->work_0x38;
    _PLW* plw = (_PLW*)eft->source_0x30;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 scale;
    nw4r::math::VEC3 ang;
    nw4r::math::MTX34 mtx;
    nw4r::math::MTX34 joint;
    _GXColor color;
    s8 level;
    u8 slot;
    s32 i;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    VEC3_ctor(&ofs);
    VEC3_ctor(&scale);
    VEC3_ctor(&ang);
    MTX34_ctor(&mtx);
    MTX34_ctor(&joint);
    if (eft_res_spawn_gate_ck(eft, 0) == 0) {
        eft->flag_0x01 = 0;
        eft->state_0x05++;
        return;
    }
    eft->area_0x44 = plw->area_0x16;
    if (plw->field_0x002 != 7) {
        return;
    }
    if (plw->field_0x018 == 0) {
        return;
    }
    if (event_demo_ck() == 1) {
        return;
    }
    level = pl_stagger_level_get(plw);
    if (level == 0) {
        return;
    }
    slot = pl_slot_motion_allow_get(plw, 7);
    switch (slot) {
    case 0:
        mhchar_joint_mtx_get(&((Eft033Physics*)plw->physics_0x13C)->chr_0x04, 11, (Mtx34*)&joint);
        ang = eft033_gun_angles;
        mtx34_identity(&mtx);
        mtx34_rot_xyz_deg(&mtx, ang.x, ang.y, ang.z);
        mtx34_trans_get(&joint, &eft->pos_0x18);
        mtx34_concat((Mtx34*)&mtx, (Mtx34*)&joint, (Mtx34*)&mtx);
        mtx34_set_trans(&mtx, &eft->pos_0x18);
        break;
    case 1:
        mhchar_joint_mtx_get(&((Eft033Physics*)plw->physics_0x13C)->chr_0x04, 7, (Mtx34*)&mtx);
        break;
    default:
        return;
    }
    if (equip_variant_get(&plw->equipB) == 7) {
        switch (slot) {
        case 0:
            setVector3(&ofs, 20.0f, 0.0f, 90.0f);
            mulVecMat(&ofs, &mtx);
            rotLocalMatY(0x11C7, &mtx);
            break;
        case 1:
            setVector3(&ofs, -6.0f, 0.0f, 118.0f);
            mulVecMat(&ofs, &mtx);
            rotLocalMatY(0xEFA6, &mtx);
            rotLocalMatZ(0x8000, &mtx);
            break;
        }
        mtx34_trans_add(&mtx, &ofs);
        mtx34_trans_get(&mtx, &eft->pos_0x18);
        setVector3(&scale, 0.4f, 1.0f, 1.8f);
    } else {
        copyVec3(&ofs, &work->ofs_0x0C);
        mulVecMat(&ofs, &mtx);
        mtx34_trans_add(&mtx, &ofs);
        mtx34_trans_get(&mtx, &eft->pos_0x18);
        setVector3(&scale, 0.3f, 1.0f, 2.5f);
    }
    for (i = 0; i < work->count_0x00; i++) {
        work->effects_0x04[i]->SetRootMtx(mtx);
        change_paramscale_eff_vec3(work->effects_0x04[i], &scale);
        effect_move(work->effects_0x04[i]);
    }
    c0 = (s8)(level - 1) * 4;
    color.r = eft033_gun_colors_a[c0];
    c1 = c0 + 1;
    color.g = eft033_gun_colors_a[c1];
    c2 = c0 + 2;
    color.b = eft033_gun_colors_a[c2];
    c3 = c0 + 3;
    color.a = eft033_gun_colors_a[c3];
    change_color_eff(work->effects_0x04[0], &eft->pos_0x18, color);
    color.r = eft033_gun_colors_b[c0];
    color.g = eft033_gun_colors_b[c1];
    color.b = eft033_gun_colors_b[c2];
    color.a = eft033_gun_colors_b[c3];
    change_color_eff(work->effects_0x04[1], &eft->pos_0x18, color);
    eft_res_models_spawn(eft, (void**)work->effects_0x04, 1, work->count_0x00, NULL);
}

/* Fades the joint effect out: the fading kinds retire their emitters and spawn their models for the fade time, the
 * others end at once. */
void eft033_fade(_EFT* eft)
{
    Eft033Work* work = (Eft033Work*)eft->work_0x38;

    if ((u32)(eft->type_0x02 - 0x19) <= 3 || (u32)(eft->type_0x02 - 5) <= 1) {
        switch (eft->field_0x06) {
        case 0:
            eft->flag_0x01 = 1;
            eft->field_0x06++;
            work->effects_0x04[0]->RetireEmitterAll();
        case 1:
            effect_retire(work->effects_0x04[0], 0);
            if (--eft->field_0x10 < 0) {
                eft->state_0x05++;
                return;
            }
            eft_res_models_spawn(eft, (void**)work->effects_0x04, 1, work->count_0x00, NULL);
            break;
        }
    } else {
        eft->state_0x05++;
    }
}

/* Forwards to the effect release helper (a one-instruction tail call). */
void eft033_retire(_EFT* obj)
{
    eft_res_slot_release(obj);
}

/* Copies the shop work's equipment records onto the player with mode `kind`. */
void lb_eqshop_pl_equip_set(_PLW* plw, LbEquipShopWork* self, u8 kind)
{
    lb_equip_records_copy(plw, kind, self->equip_0x454, &self->equip_0x49C, self->equip_0x4A8);
}

/* Seeds the player's equipment records from `src` and refreshes its body type. */
void lb_eqshop_pl_equip_seed(_PLW* plw, LbEquipShopWork* src)
{
    lb_equip_records_copy(plw, 1, src->equip_0x454, &src->equip_0x49C, src->equip_0x4A8);
    plw->field_0x002 = Get_pl_type(&plw->equipB, &plw->equipC);
}

/* Opens the equipment shop for `plw`: clears the menu work and enters lobby screen 3. */
void lb_eqshop_open(_PLW* plw, s32 arg)
{
    LbEquipShopWork* self = (LbEquipShopWork*)lobby_w.menu_0xAC;

    memset(self, 0, 0x2000);
    self->plw_0x12C = plw;
    self->arg_0x130 = arg;
    lobby_w.state_0x000 = 3;
    lobby_w.active_0x008 = 1;
    lobby_w.param_0x12D = 0;
    lb_talk_page_open(0);
    lb_eqshop_pl_equip_set(plw, self, 0);
}

/* Closes the equipment shop: puts the player's equipment back and sends the profile when it changed. */
void lb_eqshop_close(void)
{
    LbEquipShopWork* self = (LbEquipShopWork*)lobby_w.menu_0xAC;

    fn_800D0734(0);
    fn_8004C038(self->plw_0x12C, lobby_world_block);
    if (lobby_w.param_0x12D != 0) {
        lobby_w.param_0x12D = 0;
        ((s32 (*)(LbEftFlagBlock*))lobby_net_profile_part1_send)(lobby_world_block);
    }
    lb_panel_close();
}

/* Marks every goods row of the shop open (0x40) and clears the count, keeping the page `page`. */
void lb_eqshop_goods_clear(LbEquipShopWork* self, s16 page)
{
    self->count_0x0F8 = 0;
    self->page_0x0F2 = page;
    memset(self->goods_0x134, 0, sizeof(self->goods_0x134));
    self->goods_0x134[0].flags_0x00 = 0x40;
    self->goods_0x134[1].flags_0x00 = 0x40;
    self->goods_0x134[2].flags_0x00 = 0x40;
    self->goods_0x134[3].flags_0x00 = 0x40;
    self->goods_0x134[4].flags_0x00 = 0x40;
    self->goods_0x134[5].flags_0x00 = 0x40;
    self->goods_0x134[6].flags_0x00 = 0x40;
    self->goods_0x134[7].flags_0x00 = 0x40;
}

/* Whether a shop row is unlocked: the offline condition, or the online one when the game is ready. */
s32 lb_shop_row_unlocked_ck(LbIdRow6* row)
{
    if (game_ready_ck() == 0) {
        if (lb_unlock_cond_ck(row->value_0x02) == 0) {
            return 0;
        }
    } else {
        if (lb_unlock_cond_ck(row->sub_0x04) == 0) {
            return 0;
        }
    }
    return 1;
}

/* The sprite index the weapon record carries for a slot and an item id. */
u32 lb_weapon_price_get(u8 slot, u16 id)
{
    LbItemRef* rec = (LbItemRef*)fn_8027E354(slot, id);

    return rec->value_0x08 << 1;
}

/* Fills the work block's info record from the selected slot record, then feeds it to the page. */
void lb_eqshop_goods_preview_set(LbEquipShopWork* self)
{
    s16 sel = self->cursor_0x0EC;

    lb_row_init(&self->row_0x430, self->goods_0x134[sel].kind_0x01,
                self->goods_0x134[sel].id_0x02);
    if (lb_eqshop_set_parts_get(self) == 1) {
        lb_scroll_tables_set(&self->scroll_0x3A8, 0, 0, &self->row_0x430, &self->sub_0x43C);
    } else {
        lb_scroll_tables_set(&self->scroll_0x3A8, 0, 0, &self->row_0x430, 0);
    }
    lb_equip_compare_set(&self->scroll_0x3A8, &self->row_0x430);
}

/* Builds page `page` of the weapon goods: walks the category's table in sort order and keeps the unlocked rows
 * the kind filter allows, marking the ones the box or the money cannot take. */
void lb_eqshop_weapon_page_build(LbEquipShopWork* self, s16 page)
{
    LbEftFlagBlock* user = lobby_world_block;
    u16 free = userdata_equip_box_free_count();
    s32 first = page * 8;
    s32 last = first + 7;
    LbIdRow6* table;
    s32 n;
    s32 idx;
    u16 id;
    LbShopGoods* goods;

    lb_eqshop_goods_clear(self, page);
    table = (LbIdRow6*)lb_eqshop_weapon_tables[self->category_list_0x044.cursor_0x00];
    n = 0;
    self->count_0x0F8 = 0;
    idx = -1;
    self->kind_0x064 = lb_eqshop_weapon_kinds[self->category_list_0x044.cursor_0x00];
    for (;;) {
        idx = lb_shop_row_next(self->kind_0x064, self->sort_0x068.pick_b_0x80, idx, table);
        if (idx == -1) {
            break;
        }
        id = table[idx].id_0x00;
        if (lb_equip_kind_ck(self->sort_0x068.pick_a_0x81, self->kind_0x064, id) != 1 || id == 0) {
            continue;
        }
        if (first <= n && n <= last) {
            goods = &self->goods_0x134[self->count_0x0F8];
            goods->kind_0x01 = self->kind_0x064;
            goods->id_0x02 = id;
            goods->price_0x04 = lb_weapon_price_get(goods->kind_0x01, goods->id_0x02);
            goods->flags_0x00 = 0;
            if (free == 0) {
                goods->flags_0x00 |= 2;
            } else if (goods->price_0x04 > user->money_0x0018) {
                goods->flags_0x00 |= 4;
            }
            self->count_0x0F8++;
        }
        n++;
    }
    if (self->cursor_0x0EC >= self->count_0x0F8) {
        self->cursor_0x0EC = self->count_0x0F8 - 1;
        if (self->cursor_0x0EC < 0) {
            self->cursor_0x0EC = 0;
        }
    }
    self->pages_0x0F4 = menu_page_count(n, 8);
    lb_eqshop_goods_preview_set(self);
}

/* Sets the shop's tab list up: four tabs, the kind each one shows, and the scroll list of the current tab. */
void lb_eqshop_tabs_init(LbEquipShopWork* self)
{
    lb_list_init(&self->tab_list_0x104, 4, 0, 0xD1, -1, 0, 0, 3, 0);
    memset(self->tab_kinds_0x428, 0, 8);
    if (self->kind_0x064 == 0xB) {
        self->tab_kinds_0x428[0] = 5;
        self->tab_kinds_0x428[1] = 6;
        self->tab_kinds_0x428[3] = 10;
    } else {
        self->tab_kinds_0x428[0] = 1;
        self->tab_kinds_0x428[1] = 2;
        self->tab_kinds_0x428[3] = 4;
    }
    self->tab_kinds_0x428[2] = 3;
    lb_scroll_init(self->plw_0x12C, (struct LbScrollWork*)&self->scroll_0x3A8, self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00], 0);
}

/* Opens the weapon goods of the current category on the first page. */
void lb_eqshop_weapon_open(LbEquipShopWork* self)
{
    self->kind_0x064 = lb_eqshop_weapon_kinds[self->category_list_0x044.cursor_0x00];
    self->page_0x0F2 = 0;
    self->cursor_0x0EC = 0;
    lb_eqshop_tabs_init(self);
    lb_eqshop_weapon_page_build(self, 0);
    lb_eqshop_goods_preview_set(self);
}

/* Returns whether the info record's id is in the page table, filling the two sub-records when it is. */
u32 lb_eqshop_set_parts_get(LbEquipShopWork* self)
{
    const LbIdRow6* row;

    if (self->row_0x430.kind == 0x0B) {
        row = (const LbIdRow6*)lb_eqshop_set_parts;
        while (row->id_0x00 != 0) {
            if (row->id_0x00 == self->row_0x430.item_id) {
                lb_row_init(&self->sub_0x43C, 0xC, row->value_0x02);
                lb_row_init(&self->sub_0x448, 0xD, row->sub_0x04);
                return 1;
            }
            row++;
        }
    }
    return 0;
}

/* Buys the picked goods: puts the piece (and a set's two extra pieces) in the box and charges the price. */
void lb_eqshop_buy(LbEquipShopWork* self)
{
    lb_row_init(&self->row_0x430, self->goods_0x134[self->pick_0x0F6].kind_0x01,
                self->goods_0x134[self->pick_0x0F6].id_0x02);
    self->box_0x124 = userdata_equip_box_add_new((struct Q_UserData*)lobby_world_block, self->row_0x430.kind,
                                                 self->row_0x430.item_id);
    self->box_0x126 = 0xFFFF;
    self->box_0x128 = 0xFFFF;
    if (lb_eqshop_set_parts_get(self) == 1) {
        self->box_0x126 = userdata_equip_box_add_new((struct Q_UserData*)lobby_world_block,
                                                     self->sub_0x43C.kind, self->sub_0x43C.item_id);
        self->box_0x128 = userdata_equip_box_add_new((struct Q_UserData*)lobby_world_block,
                                                     self->sub_0x448.kind, self->sub_0x448.item_id);
    }
    score_add_clamped(-self->goods_0x134[self->pick_0x0F6].price_0x04, &lobby_world_block->money_0x0018);
    sysSE_req(9);
}

/* Equips the bought pieces on the player and refreshes its equipment records. */
void lb_eqshop_bought_equip(LbEquipShopWork* self)
{
    lb_equip_entry_apply(self->box_0x124);
    if (self->box_0x126 != 0xFFFF) {
        lb_equip_entry_apply(self->box_0x126);
    }
    if (self->box_0x128 != 0xFFFF) {
        lb_equip_entry_apply(self->box_0x128);
    }
    fn_8004C038(self->plw_0x12C, lobby_world_block);
    lb_eqshop_pl_equip_set(self->plw_0x12C, self, 2);
}

/* Builds page `page` of the armour goods: as the weapon page, with a set's price summed over its three pieces
 * and the box needing room for all of them. */
void lb_eqshop_armor_page_build(LbEquipShopWork* self, s16 page)
{
    LbEftFlagBlock* user = lobby_world_block;
    u16 free = userdata_equip_box_free_count();
    s32 first = page * 8;
    s32 last = first + 7;
    u8 kind;
    u32 need;
    LbShopGoods* goods;
    LbIdRow6* table;
    s32 n;
    s32 idx;
    u16 id;
    s32 price;

    lb_eqshop_goods_clear(self, page);
    kind = lb_eqshop_armor_kinds[self->category_list_0x044.cursor_0x00];
    self->kind_0x064 = kind;
    need = 1;
    if (kind == 0xB) {
        need = 3;
    }
    table = (LbIdRow6*)lb_eqshop_armor_tables[self->category_list_0x044.cursor_0x00];
    n = 0;
    self->count_0x0F8 = 0;
    idx = -1;
    for (;;) {
        idx = lb_shop_row_next(self->kind_0x064, self->sort_0x068.pick_b_0x80, idx, table);
        if (idx == -1) {
            break;
        }
        id = table[idx].id_0x00;
        if (lb_equip_kind_ck(self->sort_0x068.pick_a_0x81, self->kind_0x064, id) != 1 || id == 0) {
            continue;
        }
        if (first <= n && n <= last) {
            goods = &self->goods_0x134[self->count_0x0F8];
            goods->kind_0x01 = self->kind_0x064;
            goods->id_0x02 = id;
            if (self->kind_0x064 != 0xB) {
                price = lb_equip_price_get(self->kind_0x064, id);
            } else {
                price = lb_equip_price_get(self->kind_0x064, id);
                price += lb_equip_price_get(0xC, id);
                price += lb_equip_price_get(0xD, id);
            }
            self->goods_0x134[self->count_0x0F8].price_0x04 = price;
            self->goods_0x134[self->count_0x0F8].flags_0x00 = 0;
            if (need > free) {
                self->goods_0x134[self->count_0x0F8].flags_0x00 |= 2;
            } else if (price > user->money_0x0018) {
                self->goods_0x134[self->count_0x0F8].flags_0x00 |= 4;
            }
            self->count_0x0F8++;
        }
        n++;
    }
    if (self->cursor_0x0EC >= self->count_0x0F8) {
        self->cursor_0x0EC = self->count_0x0F8 - 1;
        if (self->cursor_0x0EC < 0) {
            self->cursor_0x0EC = 0;
        }
    }
    self->pages_0x0F4 = menu_page_count(n, 8);
    lb_eqshop_goods_preview_set(self);
}

/* Opens the armour goods of the current category on the first page. */
void lb_eqshop_armor_open(LbEquipShopWork* self)
{
    self->kind_0x064 = lb_eqshop_armor_kinds[self->category_list_0x044.cursor_0x00];
    self->page_0x0F2 = 0;
    self->cursor_0x0EC = 0;
    lb_eqshop_tabs_init(self);
    lb_eqshop_armor_page_build(self, 0);
    lb_eqshop_goods_preview_set(self);
}

/* Clears the sort chooser for equipment kind `kind` and sets its first list up. */
void lb_eqshop_sort_init(LbShopSortWork* self, u8 kind)
{
    self->step_0x82 = 0;
    self->kind_0x83 = kind;
    memset(&self->lists_0x00[0], 0, sizeof(LbList));
    memset(&self->lists_0x00[1], 0, sizeof(LbList));
    memset(&self->lists_0x00[2], 0, sizeof(LbList));
    memset(self->values_b_0x60, 0, sizeof(self->values_b_0x60));
    memset(self->values_a_0x70, 0, sizeof(self->values_a_0x70));
    lb_choice_init(&self->lists_0x00[0], (const LbListTmpl*)&lb_eqshop_list_tmpls[5], (u32)(kind - 11) <= 2, 0);
}

/* Sets the chooser's second list up for the first list's pick. */
void lb_eqshop_sort_sub_init(LbShopSortWork* self)
{
    switch (self->lists_0x00[0].cursor_0x00) {
    case 0:
        if ((u32)((u8)equip_kind_table_class(self->kind_0x83) - 1) <= 1) {
            lb_list_init(&self->lists_0x00[1], 3, 2, 0x108, -1, 0, -1, 3, 0);
            return;
        }
        lb_list_init(&self->lists_0x00[1], 3, 2, 0x102, -1, 0, -1, 3, 0);
        self->values_a_0x70[0] = 9;
        self->values_a_0x70[1] = 0xA;
        self->values_a_0x70[2] = 0;
        return;
    case 1:
        if ((u32)((u8)equip_kind_table_class(self->kind_0x83) - 1) <= 1) {
            lb_list_init(&self->lists_0x00[1], 2, 2, 0x10B, -1, 0, -1, 3, 0);
            self->values_b_0x60[0] = 0xC;
            self->values_b_0x60[1] = 0;
            return;
        }
        lb_list_init(&self->lists_0x00[1], 3, 2, 0x105, -1, 0, -1, 3, 0);
        self->values_b_0x60[0] = 6;
        self->values_b_0x60[2] = 0;
        return;
    }
}

/* Takes the second list's pick: decides the sort, or sets the third list up.  Returns 1 when decided. */
u8 lb_eqshop_sort_pick(LbShopSortWork* self)
{
    u8 done = 0;

    switch (self->lists_0x00[0].cursor_0x00) {
    case 0:
        if ((u32)((u8)equip_kind_table_class(self->kind_0x83) - 1) <= 1) {
            switch (self->lists_0x00[1].cursor_0x00) {
            case 2:
                self->pick_a_0x81 = 0;
                done = 1;
                break;
            case 0:
                lb_list_init(&self->lists_0x00[2], 5, 6, 0xFA, -1, 0, -1, 3, 0);
                self->values_a_0x70[0] = 1;
                self->values_a_0x70[1] = 2;
                self->values_a_0x70[2] = 3;
                self->values_a_0x70[3] = 4;
                self->values_a_0x70[4] = 5;
                break;
            case 1:
                lb_list_init(&self->lists_0x00[2], 3, 6, 0xFF, -1, 0, -1, 3, 0);
                self->values_a_0x70[0] = 6;
                self->values_a_0x70[1] = 7;
                self->values_a_0x70[2] = 8;
                break;
            }
        } else {
            self->pick_a_0x81 = self->values_a_0x70[self->lists_0x00[1].cursor_0x00];
            done = 1;
        }
        break;
    case 1:
        if ((u32)((u8)equip_kind_table_class(self->kind_0x83) - 1) <= 1) {
            self->pick_b_0x80 = self->values_b_0x60[self->lists_0x00[1].cursor_0x00];
            done = 1;
        } else if (self->lists_0x00[1].cursor_0x00 != 1) {
            self->pick_b_0x80 = self->values_b_0x60[self->lists_0x00[1].cursor_0x00];
            done = 1;
        } else {
            lb_list_init(&self->lists_0x00[2], 5, 6, 0xF1, -1, 0, -1, 3, 0);
            self->values_b_0x60[0] = 1;
            self->values_b_0x60[1] = 2;
            self->values_b_0x60[2] = 3;
            self->values_b_0x60[3] = 4;
            self->values_b_0x60[4] = 5;
        }
        break;
    }
    return done;
}

/* One input step of the sort chooser: 1 when a sort is decided, 2 when it is cancelled, else 0. */
s32 lb_eqshop_sort_step(LbShopSortWork* self)
{
    s32 ret = 0;

    switch (self->step_0x82) {
    case 0:
        switch (lb_choice_step(&self->lists_0x00[0])) {
        case 1:
            self->step_0x82 = 1;
            self->lists_0x00[1].cursor_0x00 = 0;
            lb_eqshop_sort_sub_init(self);
            break;
        case 2:
            ret = 2;
            break;
        }
        break;
    case 1:
        switch (lb_choice_step(&self->lists_0x00[1])) {
        case 1:
            self->lists_0x00[2].cursor_0x00 = 0;
            if (lb_eqshop_sort_pick(self) == 0) {
                self->step_0x82 = 2;
                sysSE_req(0);
            } else {
                ret = 1;
                sysSE_req(0xD);
            }
            break;
        case 2:
            self->step_0x82 = 0;
            break;
        }
        break;
    case 2:
        switch (lb_choice_step(&self->lists_0x00[2])) {
        case 1:
            if (self->lists_0x00[0].cursor_0x00 != 1) {
                self->pick_a_0x81 = self->values_a_0x70[self->lists_0x00[2].cursor_0x00];
            } else {
                self->pick_b_0x80 = self->values_b_0x60[self->lists_0x00[2].cursor_0x00];
            }
            ret = 1;
            sysSE_req(0xD);
            break;
        case 2:
            self->step_0x82 = 1;
            break;
        }
        break;
    }
    return ret;
}

/* Clears the player's model turn angle. */
void lb_pl_model_turn_reset(_PLW* plw)
{
    plw->field_0x032 = 0;
}

/* Turns the player's model by the pointer's horizontal press flags. */
void lb_pl_model_turn_step(_PLW* plw)
{
    u16 flags;

    if ((u32)chk_pointer() != 1) {
        if (lobby_net_busy_ck() != 1) {
            flags = Psw.press_0x2C8;
            if (flags & 0x0C00) {
                if (flags & 0x0800) {
                    plw->field_0x032 -= 0x400;
                }
                if (flags & 0x0400) {
                    plw->field_0x032 += 0x400;
                }
            }
        }
    }
}

/* On the model tab, drives the player's preview: the camera mode by the view, and the model turn. */
void lb_eqshop_preview_step(LbEquipShopWork* self)
{
    if (self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00] == 3) {
        if ((u8)(lobby_w.field_0x0B0 + 0xFF) <= 1) {
            fn_800D0734(0);
            fn_802754B4(self->plw_0x12C);
            return;
        }
        switch (self->type_list_0x024.cursor_0x00) {
        case 0:
            fn_800D0734(2);
            lb_pl_model_turn_step(self->plw_0x12C);
            break;
        case 1:
            fn_800D0734(1);
            lb_pl_model_turn_step(self->plw_0x12C);
            break;
        }
    }
}

/* Puts the player's equipment back and resets its pose when the model tab or the flag asks for it. */
void lb_eqshop_preview_end(LbEquipShopWork* self)
{
    fn_8004C038(self->plw_0x12C, lobby_world_block);
    if (self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00] == 3) {
        fn_802754B4(self->plw_0x12C);
    } else if (self->flag_0x101 != 0) {
        fn_802754B4(self->plw_0x12C);
    }
}

/* Arms the 15-frame timer at +0x4C0 when the current tab is the model tab. */
void lb_eqshop_preview_timer_set(LbEquipShopWork* self)
{
    if (self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00] == 3) {
        self->time_0x4C0 = 15;
    }
}

/* One frame of the weapon shop: the goods list input, the buy confirmation, the equip question, the sort chooser
 * and the box notice.  Returns 2 when the shop is left. */
s32 lb_eqshop_weapon_step(LbEquipShopWork* self)
{
    s32 ret = 0;
    s16 prev;
    u8 kind;
    _EQUIP row;
    _EQUIP part_a;
    _EQUIP part_b;
    u8 rgb[4];
    s32 rgba;
    s32 b;

    if (self->time_0x4C0 > 0) {
        self->time_0x4C0--;
    }
    switch (self->step_0x001) {
    case 0:
        if (file_loading_ck(NULL, NULL) == 1 || self->time_0x4C0 > 0 || lb_page_record_empty_ck(self->plw_0x12C) == 0) {
            lb_eqshop_preview_step(self);
            break;
        }
        if (lb_cmd_held_ck(0x80) != 0) {
            if (self->held_0x100 == 0) {
                sysSE_req(5);
            }
            self->held_0x100 = 1;
            if (lb_cmd_repeat_ck(3) != 0) {
                prev = self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00];
                self->tab_list_0x104.cursor_0x00 = menu_cursor_step(self->tab_list_0x104.cursor_0x00,
                                                                    self->tab_list_0x104.count_0x06,
                                                                    lb_cmd_repeat_get(), 1, 2);
                lb_scroll_kind_set((struct LbScrollWork*)&self->scroll_0x3A8,
                                   self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00]);
                if (prev != self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00] &&
                    self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00] == 3) {
                    fn_802754B4(self->plw_0x12C);
                }
            }
        } else {
            self->held_0x100 = 0;
            if (lb_cmd_pressed_ck(0x10) != 0) {
                if (self->goods_0x134[self->cursor_0x0EC].flags_0x00 == 0) {
                    self->pick_0x0F6 = self->cursor_0x0EC;
                    if (lb_equip_flags_ck(self->goods_0x134[self->cursor_0x0EC].kind_0x01,
                                          self->goods_0x134[self->cursor_0x0EC].id_0x02,
                                          userdata_equip_mask_get((struct Q_UserData*)lobby_world_block)) == 1) {
                        self->step_0x001 = 1;
                    } else {
                        self->step_0x001 = 3;
                    }
                    self->confirm_0x0FC = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                ret = 2;
                sysSE_req(1);
                lb_eqshop_preview_end(self);
                break;
            } else if (lb_cmd_pressed_ck(0x40) == 0) {
                if (lb_cmd_repeat_ck(3) != 0) {
                    kind = self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00];
                    if (kind != 4 && kind != 10) {
                        self->cursor_0x0EC = menu_cursor_step(self->cursor_0x0EC, self->count_0x0F8,
                                                              lb_cmd_repeat_get(), 1, 2);
                        lb_eqshop_goods_preview_set(self);
                        lb_eqshop_preview_timer_set(self);
                    }
                } else if (lb_cmd_repeat_ck(0xC) != 0) {
                    kind = self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00];
                    if (kind != 4 && kind != 10) {
                        self->page_0x0F2 = menu_cursor_step_forward(self->page_0x0F2, self->pages_0x0F4,
                                                                    lb_cmd_repeat_get(), 4, 8, 6,
                                                                    &self->page_moved_0x0FA);
                        lb_eqshop_weapon_page_build(self, self->page_0x0F2);
                        lb_eqshop_preview_timer_set(self);
                    }
                } else if (lobby_cmd_release_ck(0xC00) != 0) {
                    self->scroll_0x3A8.cursor_0x08 = menu_cursor_step_fixed_tail(
                        self->scroll_0x3A8.cursor_0x08, self->scroll_0x3A8.count_0x0A, lobby_cmd_release_get(), 0x800,
                        0x400, &self->scroll_0x3A8.moved_0x06);
                }
            }
        }
        lb_eqshop_preview_step(self);
        if (file_loading_ck(NULL, NULL) == 1 || lb_page_record_empty_ck(self->plw_0x12C) == 0) {
            break;
        }
        if (system_lobby_flag_get() != 0) {
            if (self->count_0x0F8 == 0) {
                fn_802754B4(self->plw_0x12C);
                break;
            }
            lb_row_init(&row, self->goods_0x134[self->cursor_0x0EC].kind_0x01,
                        self->goods_0x134[self->cursor_0x0EC].id_0x02);
            switch ((u8)equip_kind_table_class(row.kind)) {
            case 0:
                if (EnableChangeColor(&row) == 1) {
                    rgba = equip_color_rgba_get(equip_color_index_get(&row));
                    color_rgba_copy(rgb, (u8*)&rgba);
                    b = (rgb[2] & 0xF8) >> 3;
                    equip_color_set(&row, b | (((rgb[0] & 0xF8) << 8) | ((rgb[1] & 0xFC) << 3)));
                }
                pl_equip_model_set(self->plw_0x12C, (u8*)&row);
                break;
            case 1:
                switch (row.kind) {
                case 0xB:
                    lb_row_init(&part_a, 0xC, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                    lb_row_init(&part_b, 0xD, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                    pl_equip_models_set(self->plw_0x12C, (u8*)&row, (u8*)&part_a, (u8*)&part_b);
                    break;
                default:
                    pl_equip_models_set(self->plw_0x12C, (u8*)&row, NULL, NULL);
                    break;
                }
                break;
            case 2:
                lb_row_init(&row, 0xB, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                if ((u32)(self->goods_0x134[self->cursor_0x0EC].kind_0x01 - 0xC) <= 1) {
                    lb_row_init(&part_a, 0xC, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                    lb_row_init(&part_b, 0xD, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                    pl_equip_models_set(self->plw_0x12C, (u8*)&row, (u8*)&part_a, (u8*)&part_b);
                }
                break;
            }
            self->flag_0x101 = 1;
            lb_equip_kind_se_set(&row);
        } else {
            lb_eqshop_pl_equip_seed(self->plw_0x12C, self);
        }
        break;
    case 4:
        lb_eqshop_preview_step(self);
        switch (lb_eqshop_sort_step(&self->sort_0x068)) {
        case 1:
            self->step_0x001 = 0;
            self->page_0x0F2 = 0;
            self->cursor_0x0EC = 0;
            lb_eqshop_weapon_page_build(self, 0);
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 1:
        lb_eqshop_preview_step(self);
        switch (toggle_word_step(&self->confirm_0x0FC, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFF)) {
        case 1:
            lb_eqshop_buy(self);
            lb_eqshop_weapon_page_build(self, self->page_0x0F2);
            if (lb_item_flags_ck(&lobby_world_block->box_0x0E00[self->box_0x124],
                                 userdata_equip_mask_get((struct Q_UserData*)lobby_world_block)) == 1) {
                self->step_0x001 = 2;
                self->confirm_0x0FC = 0;
            } else {
                self->step_0x001 = 5;
                lb_list_pane_init((struct LbEntryPane*)self->pane_0x174, 6, self->plw_0x12C);
                lb_list_pane_pos_set((struct LbEntryPane*)self->pane_0x174, self->box_0x124, -1, -1);
                sysSE_req(8);
                self->tab_list_0x104.cursor_0x00 = 0;
                lb_scroll_kind_set((struct LbScrollWork*)&self->scroll_0x3A8, self->tab_kinds_0x428[0]);
                lb_eqshop_preview_end(self);
                fn_802754B4(self->plw_0x12C);
            }
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 2:
        lb_eqshop_preview_step(self);
        switch (toggle_word_step(&self->confirm_0x0FC, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFE)) {
        case 1:
            self->step_0x001 = 0;
            lb_eqshop_bought_equip(self);
            lb_eqshop_weapon_page_build(self, self->page_0x0F2);
            sysSE_stop(3);
            self->tab_list_0x104.cursor_0x00 = 0;
            lb_scroll_kind_set((struct LbScrollWork*)&self->scroll_0x3A8, self->tab_kinds_0x428[0]);
            lb_eqshop_preview_end(self);
            fn_802754B4(self->plw_0x12C);
            break;
        case 2:
            self->step_0x001 = 5;
            lb_list_pane_init((struct LbEntryPane*)self->pane_0x174, 6, self->plw_0x12C);
            lb_list_pane_pos_set((struct LbEntryPane*)self->pane_0x174, self->box_0x124, -1, -1);
            sysSE_req(8);
            self->tab_list_0x104.cursor_0x00 = 0;
            lb_scroll_kind_set((struct LbScrollWork*)&self->scroll_0x3A8, self->tab_kinds_0x428[0]);
            lb_eqshop_preview_end(self);
            fn_802754B4(self->plw_0x12C);
            break;
        }
        break;
    case 3:
        lb_eqshop_preview_step(self);
        switch (lb_yes_no_step(&self->confirm_0x0FC)) {
        case 1:
            self->step_0x001 = 1;
            self->confirm_0x0FC = 0;
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 5:
        if (lb_cmd_pressed_ck(0x30) != 0) {
            self->step_0x001 = 0;
            lb_eqshop_weapon_page_build(self, self->page_0x0F2);
            sysSE_req(0);
        }
        break;
    }
    return ret;
}

/* One frame of the armour shop: as the weapon shop, with a set bought whole and no dye preview.  Returns 2 when the
 * shop is left. */
s32 lb_eqshop_armor_step(LbEquipShopWork* self)
{
    s32 ret = 0;
    s16 prev;
    u8 kind;
    _EQUIP row;
    _EQUIP part_a;
    _EQUIP part_b;

    if (self->time_0x4C0 > 0) {
        self->time_0x4C0--;
    }
    switch (self->step_0x001) {
    case 0:
        if (file_loading_ck(NULL, NULL) == 1 || self->time_0x4C0 > 0 || lb_page_record_empty_ck(self->plw_0x12C) == 0) {
            lb_eqshop_preview_step(self);
            break;
        }
        if (lb_cmd_held_ck(0x80) != 0) {
            if (self->held_0x100 == 0) {
                sysSE_req(5);
            }
            self->held_0x100 = 1;
            if (lb_cmd_repeat_ck(3) != 0) {
                prev = self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00];
                self->tab_list_0x104.cursor_0x00 = menu_cursor_step(self->tab_list_0x104.cursor_0x00,
                                                                    self->tab_list_0x104.count_0x06,
                                                                    lb_cmd_repeat_get(), 1, 2);
                lb_scroll_kind_set((struct LbScrollWork*)&self->scroll_0x3A8,
                                   self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00]);
                if (prev != self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00] &&
                    self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00] == 3) {
                    fn_802754B4(self->plw_0x12C);
                }
            }
        } else {
            self->held_0x100 = 0;
            if (lb_cmd_pressed_ck(0x10) != 0) {
                if (self->goods_0x134[self->cursor_0x0EC].flags_0x00 == 0) {
                    self->step_0x001 = 1;
                    self->pick_0x0F6 = self->cursor_0x0EC;
                    self->confirm_0x0FC = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                ret = 2;
                sysSE_req(1);
                lb_eqshop_preview_end(self);
                break;
            } else if (lb_cmd_pressed_ck(0x40) == 0) {
                if (lb_cmd_repeat_ck(3) != 0) {
                    kind = self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00];
                    if (kind != 4 && kind != 10) {
                        self->cursor_0x0EC = menu_cursor_step(self->cursor_0x0EC, self->count_0x0F8,
                                                              lb_cmd_repeat_get(), 1, 2);
                        lb_eqshop_goods_preview_set(self);
                        lb_eqshop_preview_timer_set(self);
                    }
                } else if (lb_cmd_repeat_ck(0xC) != 0) {
                    kind = self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00];
                    if (kind != 4 && kind != 10) {
                        self->page_0x0F2 = menu_cursor_step_forward(self->page_0x0F2, self->pages_0x0F4,
                                                                    lb_cmd_repeat_get(), 4, 8, 6,
                                                                    &self->page_moved_0x0FA);
                        lb_eqshop_armor_page_build(self, self->page_0x0F2);
                        lb_eqshop_preview_timer_set(self);
                    }
                } else if (lobby_cmd_release_ck(0xC00) != 0) {
                    self->scroll_0x3A8.cursor_0x08 = menu_cursor_step_fixed_tail(
                        self->scroll_0x3A8.cursor_0x08, self->scroll_0x3A8.count_0x0A, lobby_cmd_release_get(), 0x800,
                        0x400, &self->scroll_0x3A8.moved_0x06);
                }
            }
        }
        lb_eqshop_preview_step(self);
        if (system_lobby_flag_get() != 0) {
            if (self->count_0x0F8 == 0) {
                fn_802754B4(self->plw_0x12C);
                break;
            }
            lb_row_init(&row, self->goods_0x134[self->cursor_0x0EC].kind_0x01,
                        self->goods_0x134[self->cursor_0x0EC].id_0x02);
            switch ((u8)equip_kind_table_class(row.kind)) {
            case 0:
                pl_equip_model_set(self->plw_0x12C, (u8*)&row);
                break;
            case 1:
                switch (row.kind) {
                case 0xB:
                    lb_row_init(&part_a, 0xC, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                    lb_row_init(&part_b, 0xD, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                    pl_equip_models_set(self->plw_0x12C, (u8*)&row, (u8*)&part_a, (u8*)&part_b);
                    break;
                default:
                    pl_equip_models_set(self->plw_0x12C, (u8*)&row, NULL, NULL);
                    break;
                }
                break;
            case 2:
                lb_row_init(&row, 0xB, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                if ((u32)(self->goods_0x134[self->cursor_0x0EC].kind_0x01 - 0xC) <= 1) {
                    lb_row_init(&part_a, 0xC, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                    lb_row_init(&part_b, 0xD, self->goods_0x134[self->cursor_0x0EC].id_0x02);
                    pl_equip_models_set(self->plw_0x12C, (u8*)&row, (u8*)&part_a, (u8*)&part_b);
                }
                break;
            }
            self->flag_0x101 = 1;
            lb_equip_kind_se_set(&row);
        } else {
            lb_eqshop_pl_equip_seed(self->plw_0x12C, self);
        }
        break;
    case 4:
        lb_eqshop_preview_step(self);
        if ((u32)(lb_eqshop_sort_step(&self->sort_0x068) - 1) <= 1) {
            self->step_0x001 = 0;
            self->page_0x0F2 = 0;
            self->cursor_0x0EC = 0;
            lb_eqshop_armor_page_build(self, 0);
        }
        break;
    case 1:
        lb_eqshop_preview_step(self);
        switch (toggle_word_step(&self->confirm_0x0FC, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFF)) {
        case 1:
            self->step_0x001 = 2;
            lb_eqshop_buy(self);
            lb_eqshop_armor_page_build(self, self->page_0x0F2);
            self->confirm_0x0FC = 0;
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 2:
        lb_eqshop_preview_step(self);
        switch (toggle_word_step(&self->confirm_0x0FC, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFE)) {
        case 1:
            self->step_0x001 = 0;
            lb_eqshop_bought_equip(self);
            lb_eqshop_armor_page_build(self, self->page_0x0F2);
            sysSE_stop(3);
            self->tab_list_0x104.cursor_0x00 = 0;
            lb_scroll_kind_set((struct LbScrollWork*)&self->scroll_0x3A8, self->tab_kinds_0x428[0]);
            lb_eqshop_preview_end(self);
            fn_802754B4(self->plw_0x12C);
            break;
        case 2:
            self->step_0x001 = 5;
            lb_list_pane_init((struct LbEntryPane*)self->pane_0x174, 6, self->plw_0x12C);
            lb_list_pane_pos_set((struct LbEntryPane*)self->pane_0x174, self->box_0x124, self->box_0x126,
                                 self->box_0x128);
            sysSE_req(8);
            self->tab_list_0x104.cursor_0x00 = 0;
            lb_scroll_kind_set((struct LbScrollWork*)&self->scroll_0x3A8, self->tab_kinds_0x428[0]);
            lb_eqshop_preview_end(self);
            fn_802754B4(self->plw_0x12C);
            break;
        }
        break;
    case 5:
        if (lb_cmd_pressed_ck(0x30) != 0) {
            self->step_0x001 = 0;
            lb_eqshop_armor_page_build(self, self->page_0x0F2);
            sysSE_req(0);
        }
        break;
    }
    return ret;
}

/* One frame of lobby screen 3, the equipment shop: the shop menu, the weapon/armour choice and category lists, the
 * two goods screens, the box pane and the closing talk page; registers the shop's draw while it is open. */
void lb_screen03_step(void)
{
    LbEquipShopWork* self = (LbEquipShopWork*)lobby_w.menu_0xAC;

    self->page_moved_0x0FA = 0;
    self->scroll_0x3A8.moved_0x06 = 0;
    fn_800D0734(0);
    switch (self->state_0x000) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            self->state_0x000++;
            lb_choice_init(&self->menu_0x004, (const LbListTmpl*)&lb_eqshop_list_tmpls[0], 0, 0);
            sysSE_stop(0x29);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&self->menu_0x004)) {
        case 1:
            switch (self->menu_0x004.cursor_0x00) {
            case 0:
                self->state_0x000 = 2;
                lb_choice_init(&self->type_list_0x024, (const LbListTmpl*)&lb_eqshop_list_tmpls[1], 0, 0);
                break;
            case 1:
                self->state_0x000 = 6;
                lb_list_pane_init((struct LbEntryPane*)self->pane_0x174, 4, self->plw_0x12C);
                break;
            }
            break;
        case 2:
            self->state_0x000 = 7;
            lb_talk_page_open(1);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if (self->menu_0x004.cursor_0x00 == 0) {
            switch (lb_choice_step(&self->type_list_0x024)) {
            case 1:
                self->sort_0x068.pick_b_0x80 = 0;
                self->sort_0x068.pick_a_0x81 = 0;
                self->state_0x000 = 3;
                memset(&self->category_list_0x044, 0, sizeof(LbList));
                lb_choice_init(&self->category_list_0x044,
                               (const LbListTmpl*)&lb_eqshop_list_tmpls[self->type_list_0x024.cursor_0x00 + 2], 0, 0);
                break;
            case 2:
                self->state_0x000 = 1;
                break;
            }
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if (lb_page_record_empty_ck(self->plw_0x12C) == 1) {
            switch (lb_choice_step(&self->category_list_0x044)) {
            case 1:
                switch (self->type_list_0x024.cursor_0x00) {
                case 0:
                    self->state_0x000 = 5;
                    self->step_0x001 = 0;
                    lb_eqshop_armor_open(self);
                    self->time_0x4C0 = 0;
                    break;
                case 1:
                    self->state_0x000 = 4;
                    self->step_0x001 = 0;
                    lb_eqshop_weapon_open(self);
                    self->time_0x4C0 = 0;
                    break;
                }
                self->flag_0x101 = 0;
                lb_pl_model_turn_reset(self->plw_0x12C);
                break;
            case 2:
                self->state_0x000 = 2;
                break;
            }
        }
        break;
    case 4:
        ainpc_page_hold_set();
        if (lb_eqshop_weapon_step(self) == 2) {
            self->state_0x000 = 3;
        }
        break;
    case 5:
        ainpc_page_hold_set();
        if (lb_eqshop_armor_step(self) == 2) {
            self->state_0x000 = 3;
        }
        break;
    case 6:
        ainpc_page_hold_set();
        if (lb_entry_pane_sell_step((struct LbEntryPane*)self->pane_0x174, 0) == 2) {
            self->state_0x000 = 1;
        }
        break;
    case 7:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_eqshop_close();
        }
        break;
    }
    if (self->state_0x000 != 7 && self->state_0x000 != 0) {
        subTransSet((u32)lb_eqshop_draw, 0, NULL);
    }
}

/* Draws the goods page: the frame and the kind's title, the page arrows, one cell per goods row (a set with its
 * weapon icon) and the button guide. */
void lb_eqshop_goods_draw(LbEquipShopWork* self)
{
    _EQUIP part_b;
    _EQUIP part_a;
    _EQUIP row;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16 title = lb_eqshop_goods_titles[self->kind_0x064];
    s32 active;
    s32 i;
    const u16* lsp;
    s32 open;
    s32 sel;
    _SPR_DATA_* spr;
    u8 flags;
    u8 type;

    if (self->step_0x001 == 0 && self->held_0x100 == 0) {
        active = 1;
    } else {
        active = 0;
    }
    get_lsp_data(0x1915, &base);
    draw_sprite_ary(lb_eqshop_goods_frame, &base);
    draw_sprite_idx(title, &base);
    lb_page_arrow_draw(0x1914, self->page_0x0F2, self->pages_0x0F4, self->page_moved_0x0FA, active);
    for (i = 0, lsp = lb_eqshop_goods_row_lsp; i < self->count_0x0F8; i++, lsp++) {
        open = self->goods_0x134[i].flags_0x00 == 0;
        lb_row_init(&row, self->goods_0x134[i].kind_0x01, self->goods_0x134[i].id_0x02);
        if (i == self->cursor_0x0EC) {
            sel = 1;
            ((void (*)(u16, _EQUIP*, u8))lb_equip_cell_draw)(0x18FE, &row, 5);
        } else {
            sel = 0;
        }
        spr = get_lsp_data(*lsp, NULL);
        pos.x = base.x + spr->pos.x;
        pos.y = base.y + spr->pos.y;
        flags = 2;
        if (row.kind == 0xB) {
            flags |= 8;
        }
        lb_equip_cell_draw_layout5(&row, self->goods_0x134[i].price_0x04, &pos, open, sel, active, flags, 0);
        if (row.kind == 0xB) {
            lb_row_init(&part_a, 0xC, self->goods_0x134[i].id_0x02);
            lb_row_init(&part_b, 0xD, self->goods_0x134[i].id_0x02);
            type = Get_pl_type(&row, &part_a);
            draw_weaponicon_idx(0x157, type, get_rare_color(Get_equip_rare(&row)), &pos);
        }
    }
    if (self->step_0x001 == 0 && self->held_0x100 == 0) {
        if (get_option_cfg(7) == 0) {
            draw_lsp_sprite_variant(0x1999, 0);
        } else {
            draw_lsp_sprite_variant(0x1999, 0xD);
        }
    }
}

/* Draws the sort chooser: the first list, then the second with its title by the first pick and kind `kind`, and the
 * third while it is open. */
void lb_eqshop_sort_draw(LbShopSortWork* self, u8 kind)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x1968, &pos);
    if (self->step_0x82 == 0) {
        lb_choice_draw(&self->lists_0x00[0], 0x1968, lb_sort_menu_frame, lb_sort_menu_rows, 0x1877, 0);
        draw_sprite_idx(0x1969, &pos);
        return;
    }
    lb_choice_draw(&self->lists_0x00[1], 0x1968, lb_sort_menu_frame, lb_sort_menu_rows, 0x1877, self->step_0x82 != 1);
    if (self->lists_0x00[0].cursor_0x00 == 0) {
        draw_sprite_idx(0x196A, &pos);
    } else if (kind == 0) {
        draw_sprite_idx(0x196B, &pos);
    } else {
        draw_sprite_idx(0x1969, &pos);
    }
    if (self->step_0x82 == 2) {
        lb_choice_draw(&self->lists_0x00[2], 0x1985, lb_sort_sub_frame, lb_sort_sub_rows, 0x1877, 0);
    }
}

/* Draws the goods screen: the stats page, then by the step the box pane, the sort chooser over the goods, or the
 * goods with the tab bar while the tab button is held. */
void lb_eqshop_goods_screen_draw(LbEquipShopWork* self)
{
    _mh_ivec2_ pos;
    s32 mode;
    u8 kind;

    font_set_size(0x12, 0x12);
    lb_eqshop_goods_preview_set(self);
    if (self->step_0x001 != 5 && self->count_0x0F8 != 0) {
        if (self->held_0x100 != 0) {
            mode = 0;
        } else {
            mode = self->step_0x001 == 0;
        }
        lb_equip_page_draw((struct LbScrollWork*)&self->scroll_0x3A8, mode);
    }
    switch (self->step_0x001) {
    case 5:
        get_lsp_data(0x1965, &pos);
        lb_pane_draw((struct LbEntryPane*)self->pane_0x174, pos.x, pos.y, 0);
        break;
    case 4:
        lb_eqshop_goods_draw(self);
        lb_eqshop_scroll_frame_draw(0);
        lb_eqshop_sort_draw(&self->sort_0x068, 0);
        break;
    default:
        kind = self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00];
        if (kind != 4 && kind != 10) {
            lb_eqshop_goods_draw(self);
        } else if (self->held_0x100 == 0 && self->step_0x001 == 0) {
            draw_lsp_sprite_variant(0x694, (u8)(get_option_cfg(7) == 0 ? 0 : 13));
        }
        if (self->held_0x100 != 0) {
            kind = self->tab_kinds_0x428[self->tab_list_0x104.cursor_0x00];
            if (kind != 4 && kind != 10) {
                lb_eqshop_scroll_frame_draw(0);
            }
            lb_title_frame_draw(&self->tab_list_0x104, self->tab_kinds_0x428);
        }
        break;
    }
}

/* Draws the equipment box pane of the shop's sell screen. */
void lb_eqshop_box_pane_draw(LbEquipShopWork* self)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x1965, &pos);
    lb_pane_draw((struct LbEntryPane*)self->pane_0x174, pos.x, pos.y, 0);
}

/* Draws the shop background (the wide-screen frame when wide), the tab header with tab `tab` lit when `tabs` is set,
 * and the money. */
void lb_eqshop_bg_draw(u8 tab, u8 tabs)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x195F, &pos);
        draw_sprite_ary(lb_eqshop_bg_frame_wide, &pos);
    } else {
        get_lsp_data(0x195A, &pos);
        draw_sprite_ary(lb_eqshop_bg_frame, &pos);
    }
    if (tabs != 0) {
        get_lsp_data(0x1948, &pos);
        draw_sprite_ary(lb_eqshop_tab_frame, &pos);
        if (tab == 0) {
            draw_sprite_idx(0x1949, &pos);
        } else {
            draw_sprite_idx(0x194A, &pos);
        }
    }
    menu_money_draw(0x18D8);
}

/* Draws a shop menu list with the title of entry `title` (0xFF: none; entry 4 widened by 30). */
void lb_eqshop_menu_draw(LbList* list, u8 title, u8 flag)
{
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;

    lb_choice_draw(list, 0x18DA, lb_eqshop_menu_frame, lb_eqshop_menu_rows, 0x1877, flag);
    if (title != 0xFF) {
        get_lsp_data(0x18DA, &pos);
        if (title == 4) {
            spr_data_copy((LbItemRec*)&spr, (LbItemRec*)get_lsp_data(lb_eqshop_menu_titles[title], NULL));
            spr.width += 30;
            spr.uv1.u += 30;
            draw_sprite(spr, &pos);
        } else {
            draw_sprite_idx(lb_eqshop_menu_titles[title], &pos);
        }
    }
}

/* Copies one 0x24-byte item record onto another. */
void spr_data_copy(LbItemRec* dst, const LbItemRec* src)
{
    *dst = *src;
}

/* Draws a shop menu list without a title. */
void lb_eqshop_menu_draw_plain(LbList* list, u8 flag)
{
    lb_choice_draw(list, 0x18DA, lb_eqshop_menu_frame, lb_eqshop_menu_rows, 0x1877, flag);
}

/* Draws the title list `list` with each row's kind from `kinds`. */
void lb_title_frame_draw(LbList* list, u8* kinds)
{
    _mh_ivec2_ pos;
    const u16* lsp;
    s32 i;
    bool sel;
    s32 color;

    font_flush();
    get_lsp_data(0x18B7, &pos);
    draw_sprite_anim_ary(lb_title_frame_sprites, list->count_0x06, &pos);
    for (i = 0, lsp = lb_title_frame_rows; i < list->count_0x06; lsp++, i++, kinds++) {
        sel = list->cursor_0x00 == i;
        color = GetMenuFontColor((list->off_mask_0x02 & (1 << i)) == 0, sel, 1, 0);
        lb_frame_draw_at(0x18B7, *lsp, (s8*)LbStr(0, *kinds + 0xD1), sel, color, list->kind_0x08);
    }
}

/* Draws the scroll frame of the goods page (kind 0 only). */
void lb_eqshop_scroll_frame_draw(u8 kind)
{
    _mh_ivec2_ pos;

    font_flush();
    if (kind == 0) {
        get_lsp_data(0x1915, &pos);
        draw_sprite_ary(lb_eqshop_scroll_frame, &pos);
    }
}

/* Puts the shop's help line for the current state and step, with the second line for a refused row. */
void lb_eqshop_help_draw(LbEquipShopWork* self)
{
    u16 msg = 0xFFFF;
    u16 line = 0xFFFF;
    u16 panel = 0x1877;
    s32 row_no = 1;
    _EQUIP row;

    if (self->held_0x100 != 0) {
        msg = 0x54;
    } else if ((u32)(self->state_0x000 - 4) > 1) {
        switch (self->state_0x000) {
        case 3:
            if (self->type_list_0x024.cursor_0x00 == 0) {
                msg = 0x49;
            } else {
                msg = 0x4A;
            }
            break;
        }
    } else {
        switch (self->step_0x001) {
        case 0:
            msg = 0x4B;
            if (self->goods_0x134[self->cursor_0x0EC].flags_0x00 & 2) {
                line = 0x51;
                row_no = 2;
            } else if (self->goods_0x134[self->cursor_0x0EC].flags_0x00 & 4) {
                line = 0x52;
                row_no = 2;
            }
            break;
        case 1:
            lb_help_num_put(0x1877, 0x4C, self->goods_0x134[self->pick_0x0F6].price_0x04);
            lb_panel_yes_no_draw(0x1879, self->confirm_0x0FC);
            break;
        case 2:
            msg = 0x4D;
            lb_panel_yes_no_draw(0x1879, self->confirm_0x0FC);
            lb_row_init(&row, self->goods_0x134[self->cursor_0x0EC].kind_0x01,
                        self->goods_0x134[self->cursor_0x0EC].id_0x02);
            if ((u8)equip_kind_table_class(row.kind) == 1 && lb_equip_state_get(&row) == 0) {
                line = 0x4E;
            }
            break;
        case 3:
            msg = 0x4F;
            lb_panel_yes_no_draw(0x1879, self->confirm_0x0FC);
            break;
        case 5:
            msg = 0x50;
            panel = 0x1878;
            break;
        case 4:
            panel = 0x1877;
            if ((u32)(self->sort_0x068.step_0x82 - 1) <= 1) {
                msg = self->sort_0x068.lists_0x00[0].cursor_0x00 == 0 ? 0x78 : 0x79;
            }
            break;
        }
    }
    if (msg != 0xFFFF) {
        lb_panel_msg_draw(panel, msg);
        if (line != 0xFFFF) {
            lb_panel_line_draw(panel, line, 2, row_no);
        }
    }
}

/* The shop's draw callback: the background, the menu lists by state, the goods or box screens, and the help line. */
void lb_eqshop_draw(void)
{
    LbEquipShopWork* self = (LbEquipShopWork*)lobby_w.menu_0xAC;

    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    if ((u32)(self->state_0x000 - 4) > 1) {
        switch (self->state_0x000) {
        case 1:
            lb_eqshop_bg_draw(0, 1);
            lb_eqshop_menu_draw(&self->menu_0x004, 0xFF, 0);
            break;
        case 2:
            lb_eqshop_bg_draw(0, 1);
            lb_eqshop_menu_draw(&self->type_list_0x024, 0xFF, 0);
            break;
        case 3:
            lb_eqshop_bg_draw(0, 1);
            lb_eqshop_menu_draw(&self->category_list_0x044, self->type_list_0x024.cursor_0x00, 0);
            lb_panel_msg_draw(0x1877, self->type_list_0x024.cursor_0x00 == 0 ? 0x49 : 0x4A);
            break;
        case 6:
            lb_equip_bg_draw();
            menu_money_draw(0x18D8);
            lb_eqshop_box_pane_draw(self);
            break;
        }
    } else {
        lb_equip_bg_draw();
        menu_money_draw(0x18D8);
        lb_eqshop_goods_screen_draw(self);
    }
    lb_eqshop_help_draw(self);
}

/* Opens the trade-goods sale for `plw`: clears the menu work and enters lobby screen 4. */
void lb_goods_sale_open(_PLW* plw, s32 arg)
{
    LbTradeWork* self = (LbTradeWork*)lobby_w.menu_0xAC;

    memset(self, 0, 0x2000);
    self->plw_0x01C = plw;
    self->arg_0x020 = arg;
    lobby_w.state_0x000 = 4;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* Forwards to the lobby item-table rebuild helper (a one-instruction tail call). */
s32 lb_goods_sale_close(void)
{
    return lb_panel_close();
}

/* Returns whether the id is present in the 6-byte-row table. */
s32 lb_goods_sale_row_ck(s32 id)
{
    const LbIdRow6* row = (const LbIdRow6*)lb_trade_flag_rows;

    while (row->id_0x00 != 0) {
        if (row->id_0x00 == id) {
            return 1;
        }
        row++;
    }
    return 0;
}

/* Sells every trade good: pays the total and clears both count tables. */
void lb_goods_sale_all(LbTradeWork* self)
{
    LbEftFlagBlock* user = lobby_world_block;
    s32 i;

    if (self->total_0x100 > 0) {
        userdata_zenny_add(self->total_0x100);
    }
    for (i = 0; i < 41; i++) {
        if (user->set_0x48D8[i] != 0) {
            switch (lb_goods_sale_row_ck(i)) {
            case 0:
                user->set_0x48D8[i] = 0;
                break;
            default:
                user->set_0x48D8[i] = 0;
                break;
            }
        }
    }
    memset(user->active_0x49D8, 0, sizeof(user->active_0x49D8));
}

/* Returns the 16-bit flag a slot id carries: kind 0 the set table, 1 the active table, 2 their sum. */
s32 lb_goods_count_get(u16 idx, u8 kind)
{
    LbEftFlagBlock* blk = lobby_world_block;

    switch (kind) {
    case 2:
        return blk->set_0x48D8[idx] + blk->active_0x49D8[idx];
    case 0:
        return blk->set_0x48D8[idx];
    case 1:
        return blk->active_0x49D8[idx];
    }
}

/* Returns the 8-byte table row for an id, or the null pointer. */
LbTradePairRow* lb_goods_pair_row_get(u16 id)
{
    LbTradePairRow* row = (LbTradePairRow*)lb_trade_pair_rows;

    while (row->id_0x00 != 0) {
        if (id == row->id_0x00) {
            return row;
        }
        row++;
    }
    return 0;
}

/* Returns the 6-byte table row for an id, or the null pointer. */
LbIdRow6* lb_goods_flag_row_get(u16 id)
{
    LbIdRow6* row = (LbIdRow6*)lb_trade_flag_rows;

    while (row->id_0x00 != 0) {
        if (id == row->id_0x00) {
            return row;
        }
        row++;
    }
    return 0;
}

/* Builds the sale list: the paired goods the player holds, then the single goods, their total value and the page
 * starts (a new page at 0/3 paired and 0/5 single rows). */
void lb_goods_sale_list_build(LbTradeWork* self)
{
    LbTradePairRow* pair = (LbTradePairRow*)lb_trade_pair_rows;
    LbIdRow6* single = (LbIdRow6*)lb_trade_flag_rows;
    s32 n;
    u16 set;
    u16 active;
    u16 count;

    self->pair_count_0x04C = 0;
    self->single_count_0x04E = 0;
    self->pages_0x00E = 0;
    self->total_0x100 = 0;
    memset(self->ids_0x050, 0, sizeof(self->ids_0x050));
    memset(self->page_starts_0x044, 0, sizeof(self->page_starts_0x044));
    n = 0;
    while (pair->id_0x00 != 0) {
        set = lb_goods_count_get(pair->id_0x00, 0);
        active = lb_goods_count_get(pair->id_0x00, 1);
        if (set + active != 0) {
            if (self->pair_count_0x04C == 0 || self->pair_count_0x04C == 3) {
                self->page_starts_0x044[self->pages_0x00E] = n;
                self->pages_0x00E++;
            }
            self->total_0x100 += set * pair->price_set_0x02;
            self->total_0x100 += active * pair->price_active_0x04;
            self->ids_0x050[n] = pair->id_0x00;
            n++;
            self->pair_count_0x04C++;
        }
        pair++;
    }
    switch (self->pages_0x00E) {
    case 1:
        n = 3;
        break;
    case 2:
        n = 6;
        break;
    }
    while (single->id_0x00 != 0) {
        count = lb_goods_count_get(single->id_0x00, 2);
        if (count != 0) {
            if (self->single_count_0x04E == 0 || self->single_count_0x04E == 5) {
                self->page_starts_0x044[self->pages_0x00E] = n;
                self->pages_0x00E++;
            }
            self->total_0x100 += count * single->value_0x02;
            self->ids_0x050[n] = single->id_0x00;
            n++;
            self->single_count_0x04E++;
        }
        single++;
    }
    if (self->pages_0x00E == 0) {
        self->pages_0x00E = 1;
    }
}

/* One frame of the sale: the list (decide, leave, page), the sale question and the closing notice.  Returns 2 when
 * left, 1 when the notice is closed. */
s32 lb_goods_sale_step(LbTradeWork* self)
{
    s32 ret = 0;

    switch (self->step_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (self->pair_count_0x04C + self->single_count_0x04E != 0) {
                self->step_0x001 = 1;
                self->confirm_0x018 = 0;
                sysSE_req(0);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            sysSE_req(1);
            ret = 2;
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->page_0x00C = menu_cursor_step_forward(self->page_0x00C, self->pages_0x00E, lb_cmd_repeat_get(), 4, 8,
                                                        6, &self->page_moved_0x012);
        }
        break;
    case 1:
        switch (toggle_word_step(&self->confirm_0x018, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFF)) {
        case 1:
            self->step_0x001 = 2;
            lb_goods_sale_all(self);
            sysSE_stop(0x15);
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    case 2:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            sysSE_req(0);
            ret = 1;
        }
        break;
    }
    return ret;
}

/* Returns the payload the 6-byte table carries for an id, or 0. */
u16 item_price_get(u16 id)
{
    LbIdRow6* row = (LbIdRow6*)lb_trade_price_rows;

    while (row->id_0x00 != 0) {
        if (id == row->id_0x00) {
            return row->value_0x02;
        }
        row++;
    }
    return 0;
}

/* Hands over the held item row: counts it on the item page, pays its price and latches the player's act for a
 * special item. */
void lb_goods_hold_give(LbTradeWork* self)
{
    s32 count;
    u16 id;

    eft052_hold_row_get(&id, &count);
    eft052_page_count_add(id, count);
    userdata_zenny_add(count * item_price_get(id));
    if (item_bonus_class_get(id) != 0) {
        lb_act_latch_set(self->plw_0x01C, 5);
    }
}

/* One frame of the item hand-over: the hold strip, then the hand-over question.  Returns 2 when left. */
s32 lb_goods_hold_step(LbTradeWork* self)
{
    u8* hold = cockpit_hold_block;
    s32 ret = 0;

    switch (self->step_0x001) {
    case 0:
        switch (eft052_hold_step()) {
        case 1:
            self->step_0x001++;
            self->confirm_0x018 = 0;
            break;
        case 2:
            ret = 2;
            break;
        }
        if (hold[0] == 1) {
            ainpc_page_hold_clear();
        }
        break;
    case 1:
        switch (toggle_word_step(&self->confirm_0x018, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFF)) {
        case 1:
            self->step_0x001 = 0;
            lb_goods_hold_give(self);
            eft052_hold_cursor_step();
            sysSE_stop(0x15);
            break;
        case 2:
            self->step_0x001 = 0;
            break;
        }
        break;
    }
    return ret;
}

/* Copies the ids of price group `group` (six rows each) into the work's second id list and returns their count. */
s32 lb_goods_price_group_get(LbTradeWork* self, s16 group)
{
    LbIdRow6* row;
    s32 i;

    memset(self->ids2_0x070, 0, sizeof(self->ids2_0x070));
    i = 0;
    for (row = &((LbIdRow6*)lb_trade_price_rows)[group * 6]; row->id_0x00 != 0; row++) {
        self->ids2_0x070[i] = row->id_0x00;
        i++;
    }
    return i;
}

} /* extern "C" */
