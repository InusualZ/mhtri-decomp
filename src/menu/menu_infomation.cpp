/*
 * menu/menu_infomation.cpp - the equipment-information screen: the hunter and equipment pages, the skill table, the
 *   equipment detail pages per piece kind (armour, blades, bowgun parts and their stats/ammo/skill/decoration panels),
 *   the comparison getters behind them and the column arrangement (`Set_equip_column_arrangement`).  C++; the
 *   `Put_*`/`Set_*` rows are the free functions their manglings spell, every other row is `extern "C"`.
 * RANGE. .text 0x80308FB4-0x8031A6C0 (149 functions); extab, extabindex, .data 0x805DCC50-0x805DCE98, .sdata
 *   0x80792BB8-0x80792C20, .sdata2 0x8079AE30-0x8079AE50.  One TU: `.data` 0x805DCCDC is the bare source name
 *   "menu_infomation.cpp" (one copy in the DOL) and its referrers span 0x8030A328-0x8031A244; the extab/extabindex
 *   runs tile with `ef/fn_8030681C.cpp` below and `menu/fn_8031A6C0.cpp` above.
 * FLAGS. `cflags_menu` (configure.py); its `-opt nopeephole` is what `equip_list_page_count`'s target wants.
 * NAMES. Module `menu` and the file name from the `__FILE__` string (the retail misspelling is the original).  The
 *   five `Put_*`/`Set_*` rows are the map's; every other row is named for its body and its callers (GUESS, all of
 *   them): the page/panel drawers `*_draw`, the resolve-then-draw entries `*_entry`/`*_entry_zero`/`*_entry_pair`, the
 *   comparison getters `*_compare` (bit 0 shown, bits 1/2 better/worse than the worn piece, bits 3/4 skill-raised or
 *   -lowered), `equip_variant_resolve*`, `skill_table_*`, `ammo_*`, `gun_*`, the data tables (`weapon_class_attack_mult`,
 *   `ammo_grid_*`, `gun_*_recoil_*`, `status_*_icon`, `sword_color_table`, `sharpness_colors`, `equip_panel_row_layout`)
 *   and the `StatusScreenWork` fields.  Callees named through the registry (lane menu-infomation) are declared in
 *   their owners' headers (`Pl/fn_8027D684.h`, `Pl/fn_80273B14.h`, `Pl/pl_act_step_data.h`, `hud/layout.h`,
 *   `menu/menu_item.h`, `menu/menu_row.h`, `draw_shape.h`) or in the leaf headers `Pl/pl_status_sheet_get.h`,
 *   `hud/draw_font_align.h` and `userdata_hunter_rank_info_get.h`.  Ten `Pl/fn_80273B14.h` getters are called
 *   through `_c1` views: their owner declarations return `s32`/`void*` where this screen reads `s16`/`u8` and typed rows.
 * RESIDUALS. Every row has a body; 132 of 149 are byte-identical.  The 17 open rows, largest first:
 *  - `ammo_grid_draw`: `rows` is spilled to the stack where retail keeps it in r14 - ours holds one more value live
 *    across the row loop (`i`/`row`/the `ids` index colour r22/r21/r25 against retail's r18/r25/r24).
 *  - `gun_stats_column_draw`, `skill_table_body_draw`: register colouring (r20.. vs r21..); the body's weapon-kind
 *    `switch` lowers to a binary search (`cmplwi 12`/`13`/`11`) where retail tests the 12..13 range first
 *    (`subi 12` + `cmplwi 1`) and then `cmpwi 11`; an `if` chain spelling of it loses more than it gains.
 *  - `ammo_table_draw`: the five arguments live one register window higher than retail's r16-r20 (`_savegpr_14` vs
 *    retail's `_savegpr_15`): one more value is live across the row loop.
 *  - `gun_recoil_text_get`: the `level`/`worn_level` byte pair loads into r3/r0 where retail uses r0/r3, and one
 *    `lbz`+`slwi` pair is scheduled two instructions earlier.
 *  - `active_skills_list`: retail hoists the unrolled inner loop's `li 2` trip count into r24 (the dead `view`
 *    register); ours materialises it in the loop.
 *  - `equip_level_compare`: retail reloads `worn->item_id` for the second test; MWCC reuses the first load.
 *  - register colouring only: `gun_attack_compare` (every saved value two registers lower in retail),
 *    `put_equip_info_upper_page` (`plw` r29 vs r30), `sharpness_gauge_draw` (the volatile r5/r6/r7 of `origin.y`,
 *    `quad` and `uv_width`), `put_equip_info_piece_page` (`plw`/`dyeable`/second-loop `i` rotate r29/r28/r27),
 *    `equip_skill_trees_list` (`frame` r30 vs r28), `weapon_affinity_compare` (the masked class r3 vs r4),
 *    `Put_equip_dtl_basis_bowgun_gan2` (the two `clrlwi 31` flags), `Put_equip_dtl_basis_sword_colorX` (the tier
 *    is masked into r0 where ours reuses r3).
 *  - `equip_deco_skills_compare`: the two `cmplw` take their operands in the other order (both spellings of the
 *    test compile to ours).
 *  - `equip_list_draw`: the two by-value position copies take the 0x10/0xC slots and `dye_in` 0x8 in retail; ours
 *    gives `dye_in` the highest slot.  Retail's slot order is what a by-value `_mh_ivec2_` third argument of
 *    `put_lsp_anchor_offset` would produce (EABI copies a struct argument into a caller temporary); that
 *    declaration lives in `hud/layout.h` and has not been tried.
 *  - Data: every table and literal is emitted (`.data` 0x242 of 0x248, `.sdata` 0x63 of 0x68, `.sdata2` 0x1C of 0x20)
 *    but not in retail's order - retail interleaves them with the functions in address order, and this file is not
 *    in address order.
 * SHAPES. `ammo_kind_label_draw`'s `d.x = d.x - pos.x` stores through the `extsh` retail has; `d.x -= pos.x` drops it
 *   (playbook 38).
 *  - Locals are declared in descending stack-slot order: MWCC gives the first-declared local the highest slot.
 *  - A `_SPR_DATA_` copied by initialisation (`_SPR_DATA_ spr = *get_lsp_data(...)`) is copied word by word; an
 *    assignment copies it field by field.
 *  - `w += e < 0 ? -e : e;` keeps retail's `extsh` on both paths; `if (e < 0) e = -e;` puts it inside the branch.
 *  - A resolver result kept in a `u8` local and tested as `(s32)r == 1` is masked into a scratch register, and
 *    `return r;` hands back the raw `r3`.
 *  - A byte parameter whose address a resolver takes is passed as `&b` itself: MWCC spills it in parameter order.
 *  - `x = (u8)(x & ~6);` keeps the `rlwinm`+`clrlwi` pair; `x &= ~6;` gives `andi.`.
 *  - `v = v + 6;` on an `s16` keeps the `extsh`; `v += 6;` drops it.
 *  - A 0/1 flag raised with `b++` keeps its `cmplwi 1` test; `b = 1` lets MWCC fold it to `cmpwi 0`.
 *  - The size argument of `menu_page_count` is a `u8` local: an `s16` local loses the `extsh`.
 *  - `sharpness_gauge_draw` walks its quads with `quad`/`tex` pointers loaded before they advance, and `ends`
 *    advances itself; indexing `verts[i]` gives a different loop.
 *  - A `u8` getter's result kept in a `u8` local is moved (`mr`); kept in an `s32` local it is masked (`clrlwi`).  The
 *    comparison getters `equip_decos_sort`/`equip_decos_compare`/`weapon_slot_count_get` return `u8` for that reason.
 *  - Sums of three getter calls are written `x = a(); x += b(); x += c();` (retail's `add` chain, no `clrlwi`).
 *  - A flag byte built as `m = 0; if (..) m |= A; if (..) m |= B;` keeps retail's `ori` (not `li`) on the first bit.
 *  - Switches on a resolver's result list their cases in retail's dispatch order (`6, 7, 2, 1`) with `return` in the
 *    terminal cases; the rest of the page follows the `switch`.
 *  - The `Panic` calls carry retail's line as a literal (`nw4r::db::Panic("menu_infomation.cpp", line, ...)`) or
 *    through `#line`.
 *  - Records: `StatusScreenWork` (`menu/menu_infomation.h`) is the status screen's work record (shared with
 *    `ef/fn_8030681C.cpp`); its +0x190 `equip` is the `_PLW` (`Pl/plw.h`).  `_EQUIP_INDEX` (the shown/worn view the
 *    resolvers fill) and `EquipListWork` are this unit's own; `PlSkillRec`/`PlWeaponRec`/`PlSlotRec`/`PlRgba`/
 *    `PlStatusSheet` live in `Pl/fn_8027D684.h`.
 */

#include "types.h"
#include "pl.h"
#include "hud/layout.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "menu/menu_infomation.h"
#include "Pl/pl_skill.h"
#include "Pl/fn_8027D684.h"
#include "Pl/fn_80273B14.h"
#include "Pl/pl_status_sheet_get.h"
#include "Pl/pl_act.h"
#include "Pl/pl_act_step_data.h"
#include "draw_shape.h"
#include "pad_hold_ck.h"
#include "mh3_pad/system_w.h"
#include "font/flfnt.h"
#include "menu/menu_row.h"
#include "hud/cockpit.h"
#include "MSL_C/string.h"
#include "MSL_C/strcat.h"
#include "fn_8004CAD8.h"
#include "sound/fn_800D7F54.h"
#include "ef/system_core.h"
#include "hud/draw_font_align.h"
#include "userdata_hunter_rank_info_get.h"
#include "fn_80047398/lobby_world_block.h"
#include "quest/quest_types.h"
#include "ef/fn_800CDB2C.h"
#include "ai/fn_802D44F4.h"
#include "unsplit/menu.h"
#include "userdata_item.h"
#include "nw4r/db_assert.h"

/* Call views of `Pl/pl_act.cpp` getters whose owner declarations (`Pl/fn_80273B14.h`) carry a wider return or an
 * untyped record: this screen's call sites read the narrow result (`s16`/`u8`) and the typed rows retail's code
 * shows, and pass `u8` error flags. */
#define pl_weapon_stat_get_c1 \
    ((s16 (*)(struct _PLW*, s32, u8, struct _EQUIP*, struct _EQUIP*, struct _EQUIP*, u8*))pl_weapon_stat_get)
#define pl_weapon_affinity_get_c1 \
    ((s16 (*)(struct _PLW*, struct _EQUIP*, struct _EQUIP*, struct _EQUIP*, u8*))pl_weapon_affinity_get)
#define pl_gun_clip_get_c1 \
    ((u8 (*)(struct _PLW*, u8, struct _EQUIP*, struct _EQUIP*, struct _EQUIP*, u8*))pl_gun_clip_get)
#define pl_gun_recoil_get_c1 ((u8 (*)(struct _PLW*, struct _EQUIP*, struct _EQUIP*, u8*))pl_gun_recoil_get)
#define pl_gun_reload_get_c1 ((u8 (*)(struct _PLW*, struct _EQUIP*, struct _EQUIP*, u8*))pl_gun_reload_get)
#define pl_gun_deviation_get_c1 \
    ((f32 (*)(struct _PLW*, struct _EQUIP*, struct _EQUIP*, u8*))pl_gun_deviation_get)
#define equip_slot_rec_get_c1 ((PlSlotRec* (*)(struct _EQUIP*))equip_slot_rec_get)
#define equip_defense_get_c1 ((s16 (*)(struct _EQUIP*))equip_defense_get)
#define equip_weapon_rec_get_c1 ((PlWeaponRec* (*)(struct _EQUIP*))equip_weapon_rec_get)
#define equip_gun_clip_get_c1 ((u8 (*)(struct _EQUIP*, u8))equip_gun_clip_get)

extern "C" int sprintf(s8*, const char*, ...);   /* 0x8045DECC, the OS Runtime's */

/* The resolved equipment view the detail putters draw from, filled by `equip_variant_resolve`/
 * `equip_variant_resolve_ex` and the six-piece resolver at 0x80315730: the three pieces shown (for a bowgun the
 * frame/barrel/stock triple, kinds 0xB/0xC/0xD) and the three the hunter wears beside them (`_PLW`'s
 * +0x1D0/+0x1E8/+0x1F4 records).  size: 0x18 (the six words the resolvers clear) */
struct _EQUIP_INDEX {
    /* +0x00 */ _EQUIP* shown[3];
    /* +0x0C */ _EQUIP* worn[3];
};

/* This unit's own C++-linkage exports (address order).  They are declared as their real signatures so
 * the C++ front-end reproduces the map's mangling (rule 9). */
void Put_equip_dtl_basis_bowgun_gan1(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, _mh_ivec2_* off);
void Put_equip_dtl_basis_bowgun_gan2(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, _mh_ivec2_* off);
void Put_equip_dtl_basis_bowgun_gan_lv(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, _mh_ivec2_* off);
void Put_equip_dtl_basis_sword_ken1(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, _mh_ivec2_* off, u8 flags);
void Put_equip_dtl_basis_sword_ken2(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, _mh_ivec2_* off, u8 flags);
void Put_equip_dtl_basis_sword_colorX(_PLW* plw, _EQUIP_INDEX* idx, u16 a, u16 b, _mh_ivec2_* pos, u8 c);
void Put_equip_dtl_syo_item1(_EQUIP* piece, _EQUIP* sub, bool selected, bool grey, u8 color, _mh_ivec2_ off, u16 id);
void Put_status_invork_skill_sep(_PLW* plw, _mh_ivec2_ pos, s8 page);
void Put_status_equip_status_block(_PLW* plw, _EQUIP* columns, u16 anchor, _mh_ivec2_* at);
void Put_status_equip_element_block(_PLW* plw, _EQUIP* columns, u16 anchor, _mh_ivec2_* at);
u8   Set_equip_column_arrangement(_PLW*, _EQUIP_INDEX*, _EQUIP*);

#ifdef __cplusplus
extern "C" {
#endif

/* This unit's own forward declarations (address order).  They carry C linkage here so the definitions
 * below emit the map's bare `fn_XXXXXXXX` stems (rule 9: an `fn_` stem is not a mangling). */
void equip_info_update(StatusScreenWork*);
void put_equip_info_upper_page(StatusScreenWork* self);
void put_equip_info_piece_page(StatusScreenWork* self);
s32  menu_equip_page_count(_PLW*, _EQUIP*);
s32 skill_table_page_count(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock);
void equip_page_count_step(_PLW*, _EQUIP*, u16, s8);
void skill_table_page_draw(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, u16 anchor, s8 page);
void skill_table_body_draw(_PLW* plw, _EQUIP* columns, u8* trees, s8* points, u8* active_trees, u16* active_ids, s8* active_levels, s8 page, const _mh_ivec2_* pos);
void equip_detail_page_refresh(_PLW* plw, u16 anchor, s16 page, s16 count, u16 input);
u16  weapon_attack_display(u8 kind, u16 raw);
void equip_detail_screen_reset(StatusScreenWork*);
void equip_detail_melee_draw(_PLW*, _EQUIP*, u32, u32);
void blade_basis_page_draw(_PLW* plw, _EQUIP* equip, _EQUIP* other, u16 anchor, u8 flags);
u8   gun_basis_entry(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
void equip_detail_bowgun_draw(_PLW*, _EQUIP*, u16, u8);
u8   gun_basis_panel_draw(_PLW*, _EQUIP_INDEX*, u16, u8);
u8 put_equip_row_variant(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
void equip_detail_slots_draw(_PLW*, _EQUIP*, u16, u8);
void weapon_desc_entry(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8 put_equip_row_variant_ex(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 armor_skills_entry_pair(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 armor_skills_entry(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8 deco_page_entry(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8 put_equip_row_variant2(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 deco_page_entry_pair(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 active_skills_entry(_PLW*, _EQUIP*, _EQUIP*, u16, u8);
u8 active_skills_entry_ex(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
u8 active_skills_page_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags);
void ammo_panel_entry(_PLW*, _EQUIP*, _EQUIP*, u16, u8, u8);
void put_equip_panel_row_ex(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, u8 grid);
void put_equip_panel_row_variant(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8, u8);
void put_equip_panel_row_zero(_PLW*, _EQUIP*, u16, u8);
void put_equip_panel_row(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u16, u8);
void put_lsp_sprite_at_anchor(u32*, u8);
void put_lsp_sprite_runs(u32*);
void put_equip_panel_row_base(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags);
void weapon_desc_page_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags);
void put_lsp_sprite_offset(u16);
u8 armor_skills_page_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags);
u8 deco_page_panel_draw(_PLW*, _EQUIP_INDEX*, u16, u8);
u8 deco_page_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, const _mh_ivec2_* pos);
u8 deco_skill_id_get(u16 id, s8 index);
s8 deco_skill_points_get(u16 id, s8 index);
u32 deco_rare_color_get(u16 id);
void armor_stats_page_draw(_PLW* plw, _EQUIP* equip, _EQUIP* other, u16 anchor, u8 flags);
void armor_stats_entry_zero(_PLW*, _EQUIP*, u16, u8);
void deco_skills_page_draw(_PLW* plw, _EQUIP* equip, _EQUIP* other, u16 anchor, u8 flags);
void equip_detail_kind6_draw(_PLW*, _EQUIP*, u16, u8);
void ammo_kind_label_draw(u16, u8);
void ammo_kind_label6_draw(u16);
void ammo_kind_label7_draw(u16);
void armor_skills_entry_zero(_PLW*, _EQUIP*, u16, u8);
u8 weapon_skills_page_draw(_PLW* plw, _EQUIP* equip, _EQUIP* other, u16 anchor, u8 flags);
void weapon_skills_entry_zero(_PLW*, _EQUIP*, u16, u8);
void deco_page_entry_zero(_PLW*, _EQUIP*, u16, u8);
void active_skills_entry_zero(_PLW*, _EQUIP*, u16, u8);
void ammo_panel_entry_zero(_PLW*, _EQUIP*, u16, u8, u8);
void gun_stats_footer_draw(u32*);
void ammo_table_draw(_PLW*, _EQUIP_INDEX*, u8, u8, const _mh_ivec2_*);
void ammo_table_row_frame_draw(u8, u8, const _mh_ivec2_*, u8, u8);
void ammo_table_row_label_draw(u8 kind, u8 idx, const _mh_ivec2_* pos, u8 state, u8 mode);
void ammo_table_cell_draw(u8 row, u8 col, u8 icon, const _mh_ivec2_* pos, u8 value, u8 state, u32 selected, u8 mode);
u8   sharpness_reach_get(_PLW*, s8, u32);
void sharpness_gauge_draw(_PLW* plw, _EQUIP* equip, const _mh_ivec2_* pos, u8 flags);
u8   deco_slots_view_draw(u16, _EQUIP_INDEX*, u8, const _mh_ivec2_*);
u8 deco_slots_draw(u16 anchor, _EQUIP* equip, _EQUIP* sub, const _mh_ivec2_* pos);
u8   equip_slot_count_get(_EQUIP*, _EQUIP*);
u8 equip_decos_sort(_EQUIP* equip, _EQUIP* sub, u16* ids);
u8 shown_skills_set(_PLW* plw, _EQUIP_INDEX* view, u8 flags);
void shown_skills_clear(_PLW*, u8);
u8   equip_variant_resolve(_PLW*, _EQUIP*, _EQUIP*, u8*, _EQUIP_INDEX*);
u8   equip_variant_resolve_ex(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, u8, _EQUIP_INDEX*);
u8   equip_variant_resolve_pair(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*, u8*, _EQUIP_INDEX*);
u8   equip_compare_class(u8);
u8   equip_wear_check(_PLW*, _EQUIP_INDEX*, u8);
u8   equip_defense_compare(_PLW*, _EQUIP_INDEX*, u8, u16*);
u8   equip_level_compare(_PLW*, _EQUIP_INDEX*, u8, u8*);
u8   equip_resist_compare(_PLW*, _EQUIP_INDEX*, u8, s32, s16*);
u8   weapon_attack_compare(_PLW*, _EQUIP_INDEX*, u8, u16*);
u8   weapon_element_compare(_PLW*, _EQUIP_INDEX*, u8, u8*, s16*);
u8   weapon_status_compare(_PLW*, _EQUIP_INDEX*, u8, u8*, s16*);
u8 weapon_affinity_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s16* out_value);
u8   weapon_slot_count_get(_PLW*, _EQUIP_INDEX*, u8, u8*);
u8 gun_attack_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* out_value);
u8 gun_affinity_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s16* out_value);
u8 gun_defense_bonus_get(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u32* out_value);
u8   gun_size_get(u32, _EQUIP_INDEX*, u8, u8*);
u8 gun_deviation_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, f32* out_value);
u8 gun_reload_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_value);
u8 gun_recoil_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s16* out_value);
void gun_recoil_level_get(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, u8* level, u8* icon, u8* changed);
u8   gun_recoil_kind_split(u8, u8*, u8*);
u8 gun_recoil_text_get(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s8* out_text);
u8   gun_bonus_flag_ck(_PLW*, _EQUIP_INDEX*, u8);
u8 gun_slot_bonus_get(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_value);
u8 gun_clip_compare(_PLW* plw, _EQUIP_INDEX* view, u8 ammo, u8 flags, u8* out_count);
u8 gun_ammo_usable_ck(_PLW* plw, _EQUIP_INDEX* view, u8 ammo, u8 flags);
u8 gun_frame_level_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_level);
u8   equip_deco_skills_get(_EQUIP*, u16*, s8*);
u8 equip_deco_skills_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* ids, s8* levels, u8* marks);
u8 equip_skill_trees_list(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* skills, s8* points, s8* deco_points, u8* marks);
u8   equip_decos_compare(_PLW*, _EQUIP_INDEX*, u8, u16*, u8*);
u8 active_skills_list(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* ids, u8* marks);
void equip_columns_gun_view_set(_PLW*, _EQUIP_INDEX*, _EQUIP*);
void equip_columns_gun_set(_PLW*, _EQUIP*, _EQUIP*, _EQUIP*, _EQUIP*);
void equip_panel_slot_reset(MenuSlot*);
void menu_slot_items_return(MenuSlot*);
void put_equip_panel_kind4(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, u16 anchor, u8 flags, u8 tab);
void put_equip_panel_kind5(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, u16 anchor, u8 flags, u8 page);
void equip_detail_page_draw(_PLW* plw, _EQUIP* equip, s8 page);
void equip_detail_armor_page_draw(_PLW* plw, _EQUIP* equip, s8 page);
void equip_detail_kind6_page_draw(_PLW* plw, _EQUIP* equip, s8 page);
void equip_detail_blade_page_draw(_PLW* plw, _EQUIP* equip, s8 page);
void put_equip_category_panel(_PLW* plw, _EQUIP* equip, s8 page);
void gun_parts_names_draw(const _mh_ivec2_* pos, _EQUIP_INDEX* view);
void ammo_panel_frame_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8 page, const _mh_ivec2_* pos);
void gun_stats_column_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, const _mh_ivec2_* pos);
void gun_stats_tabs_draw(const _mh_ivec2_* pos, u8 tab);
void value_badge_draw(u16 id, s32 value, _mh_ivec2_* pos);
void ammo_grid_draw(_PLW* plw, const _mh_ivec2_* pos, _EQUIP_INDEX* view, u8 flags, u8 grid, u8 mode);
void ammo_level_gauges_draw(const _mh_ivec2_* pos, u8 id, u8 level);
void gun_stats_columns_draw(_PLW* plw, _EQUIP* columns, u16 anchor);
void ammo_panel_entry_pair(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, _EQUIP* other_frame, _EQUIP* other_barrel, _EQUIP* other_stock, u16 anchor, u8 flags, u8 grid);
void column_skills_page_draw(_PLW* plw, _EQUIP* columns, u16 anchor);
u8 skill_rows_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* rows, u8 page, const _mh_ivec2_* pos);
void deco_skills_rows_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, const _mh_ivec2_* pos);
void armor_resist_block_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, const _mh_ivec2_* off);
void gun_part_tabs_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, const _mh_ivec2_* pos);
void armor_defense_block_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, const _mh_ivec2_* off);
u8 gun_basis_entry_pair(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, _EQUIP* other_frame, _EQUIP* other_barrel, _EQUIP* other_stock, u16 anchor, u8 flags);
void equip_list_draw(_EQUIP* columns, s8 cursor, u8 color, u16 anchor, u16 grey_mask);
s32 equip_detail_screen_step(StatusScreenWork* self);
void equip_detail_screen_draw(StatusScreenWork* self);
void skill_name_row_draw(u16 anchor, u16 skill, u32 color, const _mh_ivec2_* off);
u32 skill_table_row_draw(_PLW* plw, _EQUIP* columns, u16 anchor, u8 tree, u8 mode, const _mh_ivec2_* pos);
s32 skill_table_trees_list(_PLW* plw, _EQUIP* columns, u8* trees, s8* points, u8* active_trees, u16* active_ids, s8* active_levels);
void info_gun_parts_draw(u16 id, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock);

/* Callees other units own.  Most are declared in their owner's header and included above
 * (the equipment getters in `Pl/pl_status_sheet_get.h` and `Pl/fn_8027D684.h`; `Pl_Skill_slot_item_get`
 * and `pl_item_add` in `Pl/pl_skill.h`; `menu_slot_entry_b_select` and `get_menu_lsp_tbl` in `menu/menu_item.h`;
 * `GameMode_ck` in `ef/fn_800CDB2C.h`; `put_lsp_anchor_offset` and `get_str_tbl` in `unsplit/menu.h`).
 * The two below cannot: `fn_8031AE38`/`menu_sel_cursor_init` are owned by `menu/fn_8031A6C0.cpp`, whose header
 * declares neither and `unsplit/menu.h` may not.  Declared here as this unit's view. */
void menu_equip_entries_update(MenuSlot* self);
void menu_sel_cursor_init(void* cursor, s32 kind, MenuSlot* owner);


#ifdef __cplusplus
}
#endif

/* ---------------------------------------------------------------------------------------------------
 * 0x80308FB4..0x8030CA68 - the screen half's head: the five bodies the old `ef/fn_8030681C.cpp`
 * registration held (it ran into this range at 0x80308FB4, a seam the `__FILE__` string proves false)
 * --------------------------------------------------------------------------------------------------- */

/* The equip page count with the default column arrangement. */
s32 menu_equip_page_count(_PLW* equip, _EQUIP* frame)
{
    return skill_table_page_count(equip, frame, NULL, NULL);
}

/* The same, for a caller that carries a page id and a signed page delta. */
void equip_page_count_step(_PLW* a, _EQUIP* b, u16 page, s8 delta)
{
    skill_table_page_draw(a, b, NULL, NULL, page, delta);
}

/* The equip detail page with the row and the item list's own selector. */
void equip_detail_melee_draw(_PLW* a, _EQUIP* b, u32 row, u32 sel)
{
    blade_basis_page_draw(a, b, 0, row, sel);
}

/* The bowgun recoil tables `gun_recoil_level_get` reads by `PlWeaponRec::recoil_kind`: a frame's rating and its icon, and
 * the rating a barrel takes off. */
u8 gun_frame_recoil_rating[8] = { 0, 10, 20, 10, 20, 10, 20, 0 };
u8 gun_frame_recoil_icon[8] = { 0, 1, 1, 2, 2, 3, 3, 0 };
s8 gun_barrel_recoil_rating[8] = { 0, 5, 15, -6, -16, 0, 0, 0 };

/* The sharpness gauge's vertex colours `sharpness_gauge_draw` draws its seven runs with (four per run): red, orange, yellow,
 * green, blue, white, purple. */
u32 sharpness_colors[28] = {
    0xC50F3AFF, 0xC50F3AFF, 0xC50F3AFF, 0xC50F3AFF, 0xE85218FF, 0xE85218FF, 0xE85218FF, 0xE85218FF,
    0xE9BE32FF, 0xE9BE32FF, 0xE9BE32FF, 0xE9BE32FF, 0x64DF00FF, 0x64DF00FF, 0x64DF00FF, 0x64DF00FF,
    0x3068EEFF, 0x3068EEFF, 0x3068EEFF, 0x3068EEFF, 0xF0F0F0FF, 0xF0F0F0FF, 0xF0F0F0FF, 0xF0F0F0FF,
    0xF088F0FF, 0xF088F0FF, 0xF088F0FF, 0xF088F0FF,
};

/* The status sheet's element icons (by element type, from 1) and status icons (by status type, from 12); 0xFF has
 * none. */
u8 status_element_icon[8] = { 0x12, 0x14, 0x18, 0x1A, 0x16, 0xFF, 0, 0 };
u8 status_ailment_icon[4] = { 0x08, 0x06, 0x09, 0xFF };

/* The displayed-attack multiplier per weapon class (`weapon_class_index`), in hundredths: great sword, sword and
 * shield, hammer, lance, the three bowgun forms, long sword, switch axe. */
s32 weapon_class_attack_mult[9] = { 480, 140, 520, 230, 120, 120, 120, 330, 540 };

/* The bowgun ammo grid's ammo kinds (`ammo_grid_draw`): the three-level kinds (one row of six per level), the
 * two-level kinds (one row of eight per level) and the single kinds. */
u8 ammo_grid_triple[3][6] = {
    { 0x00, 0x03, 0x06, 0x09, 0x0C, 0x27 },
    { 0x01, 0x04, 0x07, 0x0A, 0x0D, 0x28 },
    { 0x02, 0x05, 0x08, 0x0B, 0x0E, 0x29 },
};
u8 ammo_grid_double[2][8] = {
    { 0x14, 0x16, 0x18, 0x1A, 0x22, 0x1E, 0x20, 0x24 },
    { 0x15, 0x17, 0x19, 0x1B, 0x23, 0x1F, 0x21, 0x24 },
};
u8 ammo_grid_single[8] = { 0x0F, 0x10, 0x12, 0x11, 0x13, 0x26, 0x1C, 0x1D };

/* 0x8030BA64 (0x68): Scales a weapon's raw attack to the displayed value by its class multiplier. */
u16 weapon_attack_display(u8 kind, u16 raw) {
    return raw * weapon_class_attack_mult[weapon_class_index(kind)] / 100;
}

/* Resets the screen's page counters and the four row flags for the equip-detail mode (mode 8). */
void equip_detail_screen_reset(StatusScreenWork* self)
{
    self->screen_mode = 8;
    self->piece_cursor = 0;
    self->page = 0;
    self->page_count = 0;
    self->saved_page = 0;
    self->saved_page_count = 0;
    self->page_input = 0;
}

/* The number of equip rows the screen shows: every row whose slot id and flag are both set, rounded up
 * to the six-per-page step (`(n + 5) / 6`) and never below one page. */
s32 equip_list_page_count(EquipListWork* self)
{
    u32 i;
    s32 count = 0;

    for (i = 0; i < 8; i++) {
        if (self->slot_id[i] != 0 && self->row_flag[i] != 0) {
            count++;
        }
    }

    {
        s8 pages = (s8)((count + 5) / 6);
        if (pages == 0) {
            pages = 1;
        }
        return pages;
    }
}

/* The sword colour panel's tint per colour form (`equip_color_tier_get`, minus one). */
u32 sword_color_table[5] = { 0x872828FF, 0x3B4E8CFF, 0x8F7428FF, 0x5746B1FF, 0x325F37FF };

/* 0x8030D338 - the sword's colour panel: classifies the slot (`equip_color_tier_get`), draws the `get_menu_lsp_tbl(0x8E|0x8F)`
 * sprite run recoloured from `sword_color_table` and, for the colour forms, up to two label rows. */
void Put_equip_dtl_basis_sword_colorX(_PLW* plw, _EQUIP_INDEX* idx, u16 a, u16 b, _mh_ivec2_* pos, u8 c) {
    u8 n = equip_color_tier_get(idx->shown[0]);
    if (n == 0) {
        return;
    }
    int sel = n - 1;
    u32 color = sword_color_table[(u8)sel];
    u16* tbl;
    _mh_ivec2_ vec;
    u32 srcA;
    u32 srcB;
    s8 buf[8];
    _SPR_DATA_ spr;
    if ((u8)sel < 3) {
        tbl = (u16*)get_menu_lsp_tbl(0x8E);
        if (a == 0) {
            srcA = *(u32*)pos;
            put_lsp_anchor_offset(0x80E, &vec, &srcA);
        } else if (pos == NULL) {
            get_lsp_data(a, &vec);
        } else {
            return;
        }
    } else {
        tbl = (u16*)get_menu_lsp_tbl(0x8F);
        if (b == 0) {
            srcB = *(u32*)pos;
            put_lsp_anchor_offset(0x819, &vec, &srcB);
        } else if (pos == NULL) {
            get_lsp_data(b, &vec);
        } else {
            return;
        }
    }
    while (*tbl != 0xFFFF) {
        spr_data_copy(&spr, get_lsp_data(*tbl, NULL));
        spr.color = color;
        draw_sprite(spr, &vec);
        tbl++;
    }
    draw_sprite_ary(((u16**)get_menu_lsp_tbl(0x95))[(u8)sel], &vec);
    switch ((u8)sel) {
    case 1:
        sprintf(buf, "%s", get_str_tbl(0x32)[2]);
        draw_font_idx(0x811, buf, 4, &vec);
        sprintf(buf, "%d", 1);
        draw_font_idx(0x812, buf, 4, &vec);
        break;
    case 2:
        sprintf(buf, "%s", get_str_tbl(0x32)[2]);
        draw_font_idx(0x814, buf, 4, &vec);
        sprintf(buf, "%d%s", 0xF, get_str_tbl(0x32)[3]);
        draw_font_idx(0x815, buf, 4, &vec);
        break;
    case 3:
        sprintf(buf, "%s", get_str_tbl(0x32)[2]);
        draw_font_idx(0x81C, buf, 4, &vec);
        sprintf(buf, "%d", 1);
        draw_font_idx(0x81D, buf, 4, &vec);
        break;
    case 4:
        sprintf(buf, "%s", get_str_tbl(0x32)[2]);
        draw_font_idx(0x81F, buf, 4, &vec);
        sprintf(buf, "%d%s", 0xF, get_str_tbl(0x32)[3]);
        draw_font_idx(0x820, buf, 4, &vec);
        sprintf(buf, "%s", get_str_tbl(0x32)[2]);
        draw_font_idx(0x81C, buf, 4, &vec);
        sprintf(buf, "%d", 1);
        draw_font_idx(0x81D, buf, 4, &vec);
        break;
    }
}

/* ---- the sword/bowgun "basis" dispatch helpers, 0x8030D6A8..0x8030D980 ---- */

/* 0x8030D6A8 - the sword-detail colour entry: force the sub-index to 0 and tail into the shared
 * `gun_basis_entry`. */
void equip_detail_bowgun_draw(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    gun_basis_entry(s0, s1, NULL, a, b);
}

/* 0x8030F374 - the same zero-sub-index entry for the `deco_skills_page_draw` band. */
void equip_detail_kind6_draw(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    deco_skills_page_draw(s0, s1, 0, a, b);
}

/* 0x8030E784 - the `armor_stats_page_draw` band's zero-sub-index entry. */
void armor_stats_entry_zero(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    armor_stats_page_draw(s0, s1, 0, a, b);
}

/* 0x8030FC40 - the `armor_skills_entry` band's zero-sub-index entry. */
void armor_skills_entry_zero(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    armor_skills_entry(s0, s1, 0, a, b);
}

/* 0x8030FEF4 - the `weapon_skills_page_draw` band's zero-sub-index entry. */
void weapon_skills_entry_zero(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    weapon_skills_page_draw(s0, s1, 0, a, b);
}

/* 0x80310548 - the `deco_page_entry` band's zero-sub-index entry. */
void deco_page_entry_zero(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    deco_page_entry(s0, s1, 0, a, b);
}

/* 0x80310E40 - the `active_skills_entry` band's zero-sub-index entry. */
void active_skills_entry_zero(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    active_skills_entry(s0, s1, 0, a, b);
}

/* 0x80311540 - the `ammo_panel_entry` band's zero-sub-index entry (its third argument's top two bits
 * are cleared on the way through). */
void ammo_panel_entry_zero(_PLW* s0, _EQUIP* s1, u16 a, u8 c, u8 d) {
    c = (u8)(c & ~6);
    ammo_panel_entry(s0, s1, NULL, a, c, d);
}

/* 0x8030FB60 / 0x8030FB6C - the two fixed-kind entries into `ammo_kind_label_draw`. */
void ammo_kind_label6_draw(u16 a) {
    ammo_kind_label_draw(a, 6);
}

void ammo_kind_label7_draw(u16 a) {
    ammo_kind_label_draw(a, 7);
}

/* 0x80312DF0 - draw the sprite run `get_menu_lsp_tbl(0xCD)` names at the row `put_lsp_anchor_offset`
 * positions from the caller's value. */
void gun_stats_footer_draw(u32* p) {
    _mh_ivec2_ pos;
    u32 v = *p;
    put_lsp_anchor_offset(0x9A5, &pos, &v);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xCD), &pos);
}

/* 0x8030D6C0 - the shared sword/small-blade branch: resolve the caller's variant into a stack
 * record with `equip_variant_resolve` and hand off to the dispatcher (unless it reports "done"). */
u8 gun_basis_entry(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    u8 r = equip_variant_resolve(s0, s1, s2, &spb, &out);
    if (r == 1) {
        return r;
    }
    return gun_basis_panel_draw(s0, &out, a, spb);
}

/* 0x8030FB78 - the bowgun/ammo row: derive the offset from the 0x8A7 anchor (or zero), then draw
 * the `get_str_tbl(0x2E)[kind]` label at entry 0x8A8. */
void ammo_kind_label_draw(u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ d;
    if (a != 0) {
        get_lsp_data(0x8A7, &pos);
        get_lsp_data(a, &d);
        d.x -= pos.x;
        d.y -= pos.y;
    } else {
        d.x = 0;
        d.y = 0;
    }
    u32 tmp = *(u32*)&d;
    put_lsp_anchor_offset(0x8A7, &pos, &tmp);
    draw_font_idx(0x8A8, (s8*)((u8**)get_str_tbl(0x2E))[b], 0, &pos);
}

/* 0x8030D808 - the weapon detail's shared body: positions the panel from the 0x84E anchor (or the caller's offset) and
 * dispatches on `equip_wear_check`'s row: one label, the caller's value, or the three bowgun panels and a text row. */
u8 gun_basis_panel_draw(_PLW* s0, _EQUIP_INDEX* s1, u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    if (a != 0) {
        get_lsp_data(0x84E, &pos);
        get_lsp_data(a, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    u32 src = *(u32*)&off;
    put_lsp_anchor_offset(0x84E, &pos, &src);
    u8 r = equip_wear_check(s0, s1, b);
    switch (r) {
    case 1:
        return r;
    case 2:
        draw_font_idx(0x8D2, (s8*)((u8**)get_str_tbl(0x2E))[1], 0, &pos);
        return r;
    }
    shown_skills_set(s0, s1, b);
    Put_equip_dtl_basis_bowgun_gan1(s0, s1, 0, b, &off);
    Put_equip_dtl_basis_bowgun_gan2(s0, s1, 0, b, &off);
    Put_equip_dtl_basis_bowgun_gan_lv(s0, s1, 0, b, &off);
    shown_skills_clear(s0, b);
    return 0;
}

/* 0x8030D728 - the second small-weapon branch entry (resolves through `equip_variant_resolve_ex`). */
u8 put_equip_row_variant(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_ex(s0, p1, p2, p3, b, &out);
    if (r == 1) {
        return r;
    }
    return gun_basis_panel_draw(s0, &out, a, b);
}

/* ---- the same resolve-then-dispatch shape, one family per weapon sub-band ----
 * Each `weapon_desc_entry`-style entry resolves the caller's variant into a stack record with
 * `equip_variant_resolve` and hands it to its band's dispatcher; `put_equip_row_variant_ex`-style resolves through
 * `equip_variant_resolve_ex` (six registers plus a slot); `armor_skills_entry_pair`-style through `equip_variant_resolve_pair`, whose
 * ninth (stack) argument is the record. */

/* 0x8030F7B0 - the 0x8030F87C band's `equip_variant_resolve` entry. */
void weapon_desc_entry(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, s2, &spb, &out) == 1) {
        return;
    }
    weapon_desc_page_draw(s0, &out, a, spb);
}

/* 0x8030F814 - the `weapon_desc_page_draw` band's variant whose variant word is forced to 0. */
void equip_detail_slots_draw(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    if (equip_variant_resolve(s0, s1, 0, &spb, &out) == 1) {
        return;
    }
    weapon_desc_page_draw(s0, &out, a, spb);
}

/* 0x8030FC58 - the 0x8030FDA0 band's `equip_variant_resolve` entry. */
u8 armor_skills_entry(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    u8 r = equip_variant_resolve(s0, s1, s2, &spb, &out);
    if (r == 1) {
        return r;
    }
    return armor_skills_page_draw(s0, &out, a, spb);
}

/* 0x8030FCC0 - the 0x8030FDA0 band's `equip_variant_resolve_ex` entry. */
u8 put_equip_row_variant_ex(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_ex(s0, p1, p2, p3, b, &out);
    if (r == 1) {
        return r;
    }
    return armor_skills_page_draw(s0, &out, a, b);
}

/* 0x8030FD30 - the 0x8030FDA0 band's `equip_variant_resolve_pair` entry. */
u8 armor_skills_entry_pair(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, _EQUIP* p4, _EQUIP* p5, _EQUIP* p6, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_pair(s0, p1, p2, p3, p4, p5, p6, &b, &out);
    if (r == 1) {
        return r;
    }
    return armor_skills_page_draw(s0, &out, a, b);
}

/* 0x80310560 - the 0x803106A8 band's `equip_variant_resolve` entry. */
u8 deco_page_entry(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    u8 r = equip_variant_resolve(s0, s1, s2, &spb, &out);
    if (r == 1) {
        return r;
    }
    return deco_page_panel_draw(s0, &out, a, spb);
}

/* 0x803105C8 - the 0x803106A8 band's `equip_variant_resolve_ex` entry. */
u8 put_equip_row_variant2(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_ex(s0, p1, p2, p3, b, &out);
    if (r == 1) {
        return r;
    }
    return deco_page_panel_draw(s0, &out, a, b);
}

/* 0x80310638 - the 0x803106A8 band's `equip_variant_resolve_pair` entry. */
u8 deco_page_entry_pair(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, _EQUIP* p4, _EQUIP* p5, _EQUIP* p6, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_pair(s0, p1, p2, p3, p4, p5, p6, &b, &out);
    if (r == 1) {
        return r;
    }
    return deco_page_panel_draw(s0, &out, a, b);
}

/* 0x80310E58 - the 0x80310F30 band's `equip_variant_resolve` entry. */
u8 active_skills_entry(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 spb;
    spb = b;
    u8 r = equip_variant_resolve(s0, s1, s2, &spb, &out);
    if (r == 1) {
        return r;
    }
    return active_skills_page_draw(s0, &out, a, spb);
}

/* 0x80310EC0 - the 0x80310F30 band's `equip_variant_resolve_ex` entry. */
u8 active_skills_entry_ex(_PLW* s0, _EQUIP* p1, _EQUIP* p2, _EQUIP* p3, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 r = equip_variant_resolve_ex(s0, p1, p2, p3, b, &out);
    if (r == 1) {
        return r;
    }
    return active_skills_page_draw(s0, &out, a, b);
}

/* 0x80311560 - the 0x803116D8 band's `equip_variant_resolve` entry (its resolved byte is masked before it
 * is handed on). */
void ammo_panel_entry(_PLW* s0, _EQUIP* s1, _EQUIP* s2, u16 a, u8 b, u8 c) {
    _EQUIP_INDEX out;
    if (equip_variant_resolve(s0, s1, s2, &b, &out) == 1) {
        return;
    }
    b = (u8)(b & ~6);
    put_equip_panel_row_ex(s0, &out, a, b, c);
}

/* 0x803106A8 - the 0x8031077C band's body: position the panel from the 0x8F0 anchor (or the
 * caller's offset) and hand the resulting anchor to `deco_page_draw`. */
u8 deco_page_panel_draw(_PLW* s0, _EQUIP_INDEX* s1, u16 a, u8 b) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    if (a != 0) {
        get_lsp_data(0x8F0, &pos);
        get_lsp_data(a, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    u32 src = *(u32*)&off;
    put_lsp_anchor_offset(0x8F0, &pos, &src);
    u32 copy = *(u32*)&pos;
    return deco_page_draw(s0, s1, b, (_mh_ivec2_*)&copy);
}

/* 0x80310CF8 (0x88): Returns the `index`-th skill id a decoration carries (0 when it has none). */
u8 deco_skill_id_get(u16 id, s8 index) {
    ItemSpeciesRecord* rec = get_item_species_data(id);
    s32 n = 0;

    if (rec == NULL) {
        return 0;
    }
    if (rec->kind != 0) {
        if (index == 0) {
            return rec->kind;
        }
        n = 1;
    }
    if (rec->kind2 != 0 && n == index) {
        return rec->kind2;
    }
    return 0;
}

/* 0x80310D80 (0x94): Returns the points of the `index`-th skill a decoration carries (0 when it has none). */
s8 deco_skill_points_get(u16 id, s8 index) {
    ItemSpeciesRecord* rec = get_item_species_data(id);
    s32 n = 0;

    if (rec == NULL) {
        return 0;
    }
    if (rec->kind != 0) {
        if (index == 0) {
            return (s8)rec->kind_points;
        }
        n = 1;
    }
    if (rec->kind2 != 0 && n == index) {
        return (s8)rec->kind2_points;
    }
    return 0;
}

/* 0x80310E14 (0x2C): Returns the rarity colour of a decoration's item. */
u32 deco_rare_color_get(u16 id) {
    return item_rare_color_get(get_item_species_data(id)->item_id);
}

/* 0x80314F90 (0x2E4): Collects up to three decoration ids of a piece (and a barrel's stock) and sorts them by slot
 * size into `ids` (three one-slot, then a two- and a three-slot one); returns one bit per filled entry. */
u8 equip_decos_sort(_EQUIP* equip, _EQUIP* sub, u16* ids) {
    u16 own[3] = { 0, 0, 0 };
    u16 sub_ids[3] = { 0, 0, 0 };
    u16 list[3] = { 0, 0, 0 };
    u8 ret = 0;
    s32 n;
    s32 i;
    s8 small;

    equip_slot_count_get(equip, sub);
    equip_deco_ids_get(equip, own, 1);
    if (equip->kind == 0xC && sub != NULL && sub->kind == 0xD) {
        equip_deco_ids_get(sub, sub_ids, 1);
    }
    n = 0;
    for (i = 0; i < 3; i++) {
        if (own[i] != 0 && n < 3) {
            list[n] = own[i];
            n++;
        }
    }
    for (i = 0; i < 3; i++) {
        if (sub_ids[i] != 0 && n < 3) {
            list[n] = sub_ids[i];
            n++;
        }
    }
    if (ids != NULL) {
        ids[0] = 0;
        ids[1] = 0;
        ids[2] = 0;
        ids[3] = 0;
        ids[4] = 0;
    }
    for (i = 0; i < 3; i++) {
        if (list[i] != 0 && item_species_class_get(list[i]) == 3) {
            ret |= 0x10;
            if (ids != NULL) {
                ids[4] = list[i];
            }
        }
    }
    for (i = 0; i < 3; i++) {
        if (list[i] != 0 && item_species_class_get(list[i]) == 2) {
            ret |= 8;
            if (ids != NULL) {
                ids[3] = list[i];
            }
        }
    }
    small = 0;
    for (i = 0; i < 3; i++) {
        if (list[i] != 0 && item_species_class_get(list[i]) == 1) {
            if (small == 0) {
                if (ret & 8) {
                    ret |= 4;
                    if (ids != NULL) {
                        ids[2] = list[i];
                    }
                } else {
                    ret |= 1;
                    if (ids != NULL) {
                        ids[0] = list[i];
                    }
                    small++;
                }
            } else if (small == 1) {
                ret |= 2;
                if (ids != NULL) {
                    ids[1] = list[i];
                }
                small++;
            } else if (small == 2) {
                ret |= 4;
                if (ids != NULL) {
                    ids[2] = list[i];
                }
                small++;
            }
        }
    }
    return ret;
}

/* 0x8030BF34 (0x80): Draws the equipment detail page of one piece by its kind: armour, kind 6, the blade weapons and
 * the bowgun parts. */
void equip_detail_page_draw(_PLW* plw, _EQUIP* equip, s8 page) {
    switch (equip->kind) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        equip_detail_armor_page_draw(plw, equip, page);
        break;
    case 6:
        equip_detail_kind6_page_draw(plw, equip, page);
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        equip_detail_blade_page_draw(plw, equip, page);
        break;
    case 11:
    case 12:
    case 13:
        put_equip_category_panel(plw, equip, page);
        break;
    default:
        nw4r::db::Panic("menu_infomation.cpp", 2335, "NW4R:Failed assertion 0");
        break;
    }
}

/* 0x8030BFB4 (0x120): Draws an armour piece's detail page (its stats, its skills or its slot row) and the page's
 * frame, name and icon. */
void equip_detail_armor_page_draw(_PLW* plw, _EQUIP* equip, s8 page) {
    _mh_ivec2_ pos;

    switch (page) {
    case 0:
        armor_stats_entry_zero(plw, equip, 0, 6);
        equip_detail_slots_draw(plw, equip, 0, 6);
        break;
    case 1:
        weapon_skills_entry_zero(plw, equip, 0, 6);
        deco_page_entry_zero(plw, equip, 0, 6);
        break;
    case 2:
        put_lsp_sprite_offset(0x914);
        active_skills_entry_zero(plw, equip, 0x913, 4);
        break;
    }
    get_lsp_data(0x7E2, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x89), &pos);
    get_lsp_data(0x7E6, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(6), &pos);
    draw_font_idx(0x13C, (s8*)GetEquipName(equip->kind, equip->item_id), 0, &pos);
    draw_equipicon_idx(0x13D, equip, &pos);
}

/* 0x8030C0D4 (0x120): Draws a kind-6 piece's detail page and the page's frame, name and icon. */
void equip_detail_kind6_page_draw(_PLW* plw, _EQUIP* equip, s8 page) {
    _mh_ivec2_ pos;

    switch (page) {
    case 0:
        equip_detail_kind6_draw(plw, equip, 0, 6);
        equip_detail_slots_draw(plw, equip, 0, 6);
        break;
    case 1:
        weapon_skills_entry_zero(plw, equip, 0, 6);
        deco_page_entry_zero(plw, equip, 0, 6);
        break;
    case 2:
        put_lsp_sprite_offset(0x914);
        active_skills_entry_zero(plw, equip, 0x913, 4);
        break;
    }
    get_lsp_data(0x7E2, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x89), &pos);
    get_lsp_data(0x7E6, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(6), &pos);
    draw_font_idx(0x13C, (s8*)GetEquipName(equip->kind, equip->item_id), 0, &pos);
    draw_equipicon_idx(0x13D, equip, &pos);
}

/* 0x8030C1F4 (0x120): Draws a blade weapon's detail page and the page's frame, name and icon. */
void equip_detail_blade_page_draw(_PLW* plw, _EQUIP* equip, s8 page) {
    _mh_ivec2_ pos;

    switch (page) {
    case 0:
        equip_detail_melee_draw(plw, equip, 0, 6);
        equip_detail_slots_draw(plw, equip, 0, 6);
        break;
    case 1:
        armor_skills_entry_zero(plw, equip, 0, 6);
        deco_page_entry_zero(plw, equip, 0, 6);
        break;
    case 2:
        put_lsp_sprite_offset(0x914);
        active_skills_entry_zero(plw, equip, 0x913, 4);
        break;
    }
    get_lsp_data(0x7E2, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x89), &pos);
    get_lsp_data(0x7E6, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(6), &pos);
    draw_font_idx(0x13C, (s8*)GetEquipName(equip->kind, equip->item_id), 0, &pos);
    draw_equipicon_idx(0x13D, equip, &pos);
}

/* 0x8030C314 (0x208): Draws a bowgun part's detail page against the hunter's worn bowgun (its stats, the whole gun,
 * its slot row, its ammo panels) and the page's frame, name and the gun's weapon icon. */
void put_equip_category_panel(_PLW* plw, _EQUIP* equip, s8 page) {
    _mh_ivec2_ pos;
    _EQUIP* frame = &plw->equipB;
    _EQUIP* barrel = &plw->equipC;
    _EQUIP* stock = &plw->equipD;
    u8 type;

    switch (page) {
    case 0:
        put_equip_row_variant(plw, equip, barrel, stock, 0, 6);
        equip_detail_slots_draw(plw, equip, 0, 6);
        break;
    case 1:
        put_equip_panel_row(plw, equip, barrel, stock, 0, 6);
        break;
    case 2:
        put_equip_row_variant_ex(plw, frame, barrel, stock, 0, 6);
        put_equip_row_variant2(plw, frame, barrel, stock, 0, 6);
        break;
    case 3:
        put_lsp_sprite_offset(0x914);
        active_skills_entry_zero(plw, equip, 0x913, 4);
        break;
    case 4:
        put_equip_panel_kind4(plw, frame, barrel, stock, 0, 6, 0xFF);
        break;
    case 5:
    case 6:
        put_equip_panel_kind5(plw, frame, barrel, stock, 0, 6, page - 5);
        break;
    }
    switch (page) {
    case 0:
    case 2:
    case 3:
        get_lsp_data(0x7E2, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x89), &pos);
    case 1:
        get_lsp_data(0x7E6, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(6), &pos);
        draw_font_idx(0x13C, (s8*)GetEquipName(equip->kind, equip->item_id), 0, &pos);
        type = Get_pl_type(&plw->equipB, &plw->equipC);
        draw_weaponicon_idx(0x13D, type, get_rare_color(Get_equip_rare(&plw->equipB)), &pos);
        break;
    }
}

/* 0x80313B3C (0xC8): Draws the bowgun parts panel at `pos`: its frame and the frame, barrel and stock names. */
void gun_parts_names_draw(const _mh_ivec2_* pos, _EQUIP_INDEX* view) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _mh_ivec2_ at;
    u32 in;

    in = *(const u32*)pos;
    put_lsp_anchor_offset(0x98B, &at, &in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xCC), &at);
    draw_font_idx(0x997, (s8*)GetEquipName(frame->kind, frame->item_id), 5, &at);
    draw_font_idx(0x990, (s8*)GetEquipName(barrel->kind, barrel->item_id), 5, &at);
    draw_font_idx(0x99E, (s8*)GetEquipName(stock->kind, stock->item_id), 5, &at);
}

/* 0x80313C04 (0x158): Draws the bowgun ammo panel for the parts the caller gives: resolves them against the worn
 * gun, positions the panel at the 0x9AA anchor and draws the ammo rows and the panel frame. */
void put_equip_panel_kind5(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, u16 anchor, u8 flags, u8 page) {
    _EQUIP_INDEX view;
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;
    u32 rows_at;
    u32 frame_at;

    if (equip_variant_resolve_ex(plw, frame, barrel, stock, flags, &view) == 1) {
        return;
    }
    switch (equip_wear_check(plw, &view, flags)) {
    case 1:
        return;
    }
    if (anchor != 0) {
        get_lsp_data(0x9AA, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    shown_skills_set(plw, &view, flags);
    in = *(u32*)&off;
    put_lsp_anchor_offset(0x9AA, &pos, &in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xCE), &pos);
    rows_at = *(u32*)&off;
    ammo_table_draw(plw, &view, flags, page, (const _mh_ivec2_*)&rows_at);
    frame_at = *(u32*)&off;
    ammo_panel_frame_draw(plw, &view, flags, page, (const _mh_ivec2_*)&frame_at);
    shown_skills_clear(plw, flags);
}

/* 0x80313D5C (0xC8): Draws the ammo panel's frame pieces (0xD1, 0xCF, the 0x89 run and 0xCD) at `pos`. */
void ammo_panel_frame_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8 page, const _mh_ivec2_* pos) {
    _mh_ivec2_ at;
    _mh_ivec2_ inner;
    u32 in_a;
    u32 in_b;
    u32 in_c;
    u32 in_d;

    in_a = *(const u32*)pos;
    put_lsp_anchor_offset(0x9C8, &at, &in_a);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xD1), &at);
    in_b = *(const u32*)pos;
    put_lsp_anchor_offset(0x9AA, &at, &in_b);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xCF), &at);
    in_c = *(u32*)&at;
    put_lsp_anchor_offset(0x9BC, &inner, &in_c);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x89), &inner);
    in_d = *(const u32*)pos;
    put_lsp_anchor_offset(0x9A5, &at, &in_d);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xCD), &at);
}

/* 0x803128A0 (0x254): Draws the bowgun stats panel for the parts the caller gives: resolves them against the worn
 * gun, positions it at the 0x955 anchor and draws its tabs, the frame, barrel and stock rows, the whole gun's row
 * and the parts list. */
void put_equip_panel_kind4(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, u16 anchor, u8 flags, u8 tab) {
    u8 flags_base = flags;
    _EQUIP_INDEX view;
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 tabs_at;
    u32 frame_at;
    u32 barrel_at;
    u32 stock_at;
    u32 gun_at;
    u32 runs_at;
    u32 parts_at;
    u32 footer_at;

    if (equip_variant_resolve_ex(plw, frame, barrel, stock, flags_base, &view) == 1) {
        return;
    }
    switch (equip_wear_check(plw, &view, flags)) {
    case 1:
        return;
    }
    if (anchor != 0) {
        get_lsp_data(0x955, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    shown_skills_set(plw, &view, flags);
    tabs_at = *(u32*)&off;
    gun_stats_tabs_draw((const _mh_ivec2_*)&tabs_at, tab);
    flags = flags_base;
    if (equip_variant_resolve(plw, frame, NULL, &flags, &view) != 1) {
        frame_at = *(u32*)&off;
        gun_stats_column_draw(plw, &view, flags, (const _mh_ivec2_*)&frame_at);
    }
    flags = flags_base;
    if (equip_variant_resolve(plw, barrel, NULL, &flags, &view) != 1) {
        barrel_at = *(u32*)&off;
        gun_stats_column_draw(plw, &view, flags, (const _mh_ivec2_*)&barrel_at);
    }
    flags = flags_base;
    if (equip_variant_resolve(plw, stock, NULL, &flags, &view) != 1) {
        stock_at = *(u32*)&off;
        gun_stats_column_draw(plw, &view, flags, (const _mh_ivec2_*)&stock_at);
    }
    flags = flags_base;
    if (equip_variant_resolve_ex(plw, frame, barrel, stock, flags_base, &view) != 1) {
        gun_at = *(u32*)&off;
        gun_stats_column_draw(plw, &view, flags, (const _mh_ivec2_*)&gun_at);
    }
    runs_at = *(u32*)&off;
    put_lsp_sprite_runs(&runs_at);
    parts_at = *(u32*)&off;
    gun_parts_names_draw((const _mh_ivec2_*)&parts_at, &view);
    footer_at = *(u32*)&off;
    gun_stats_footer_draw(&footer_at);
    shown_skills_clear(plw, flags);
    flfntFlush();
    flfntStackReset();
}

/* 0x80312E34 (0xE4): Draws the stats panel's three tabs at `pos`, the selected one animated (held still while the
 * pad hold gate is up or the game is paused), and the tab bar's cap. */
void gun_stats_tabs_draw(const _mh_ivec2_* pos, u8 tab) {
    _mh_ivec2_ at;
    u32 in;
    u16* ids;
    s32 i;
    u16 frame;

    in = *(const u32*)pos;
    put_lsp_anchor_offset(0x955, &at, &in);
    ids = (u16*)get_menu_lsp_tbl(0xC6);
    for (i = 0; i < 3; i++) {
        if (tab == i) {
            frame = 0;
            if (pad_hold_ck() == 0 && system_w.field_0x05 == 0) {
                frame = system_w.field_0x0c % 20;
            }
            draw_sprite_anim_idx(ids[i], frame, &at);
        } else {
            draw_sprite_idx(ids[i + 3], &at);
        }
    }
    draw_sprite_idx(0x961, &at);
}

/* 0x80312F84 (0xBB8): Draws one column of the bowgun stats panel (a frame, barrel or stock, or the whole gun): its
 * nine rows - attack, affinity, slots, defence (and the gun's size), deviation, reload, recoil, ammo text and the
 * slot-bonus mark - each tinted by its comparison with the worn gun, and the row labels. */
void gun_stats_column_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, const _mh_ivec2_* pos) {
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _EQUIP* frame = view->shown[0];
    _SPR_DATA_ spr;
    _SPR_DATA_ label;
    s8 text[16];
    _mh_ivec2_ column;
    _mh_ivec2_ label_base;
    _mh_ivec2_ cell;
    u32 column_in;
    u32 label_in;
    u32 cell_in;
    u32 bonus_at;
    u32 label_at;
    s32 kind;
    s32 i;
    s32 skill;
    s32 colour;
    s32 hide;
    u8 state;
    u8 slots;
    u8** strs;
    u16* ids;

    if (frame != NULL && barrel != NULL && stock != NULL) {
        kind = 3;
    } else {
        if (frame->kind == 11) {
            kind = 0;
        } else if (frame->kind == 12) {
            kind = 1;
        } else if (frame->kind == 13) {
            kind = 2;
        } else {
            nw4r::db::Panic("menu_infomation.cpp", 6394, "NW4R:Failed assertion 0");
        }
    }
    ids = (u16*)get_menu_lsp_tbl(0xCB);
    column_in = *(const u32*)pos;
    put_lsp_anchor_offset(ids[kind], &column, &column_in);
    label_in = *(const u32*)pos;
    put_lsp_anchor_offset(0x955, &label_base, &label_in);
    skill = flags & 4;
    colour = flags & 8;
    hide = flags & 0x20;
    for (i = 0; i < 9; i++) {
        ids = (u16*)get_menu_lsp_tbl(0xCA);
        cell_in = *(u32*)&column;
        put_lsp_anchor_offset(ids[i], &cell, &cell_in);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xC9), &cell);
        state = 0;
        switch (i) {
        case 0: {
            u16 attack = 0;
            state = gun_attack_compare(plw, view, flags, &attack);
            if (state & 1) {
                sprintf(text, "%d", attack);
                spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                draw_font(spr, text, 1, &cell);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                spr.color = 0x878787FF;
                draw_font(spr, (s8*)get_str_tbl(0x30)[0], 1, &cell);
            }
            break;
        }
        case 1: {
            s16 affinity = 0;
            state = gun_affinity_compare(plw, view, flags, &affinity);
            if (state & 1) {
                sprintf(text, "%d%s", affinity, get_str_tbl(0x32)[1]);
                spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                draw_font(spr, text, 1, &cell);
            }
            break;
        }
        case 2:
            slots = 0xFF;
            switch (kind) {
            case 1:
                slots = equip_slot_count_get(frame, NULL);
                break;
            case 2:
                slots = equip_slot_count_get(frame, NULL);
                break;
            case 3:
                slots = equip_slot_count_get(barrel, stock);
                break;
            }
            if (slots != 0xFF) {
                sprintf(text, "%d", slots);
                draw_font_idx(0x973, text, 1, &cell);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                spr.color = 0x878787FF;
                draw_font(spr, (s8*)get_str_tbl(0x30)[0], 1, &cell);
            }
            break;
        case 3: {
            u32 defense = 0;
            u8 size = 0;
            state = gun_defense_bonus_get(plw, view, flags, &defense);
            if (state & 1) {
                sprintf(text, "%d", defense);
                spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                draw_font(spr, text, 1, &cell);
            }
            if (kind == 3) {
                state = gun_size_get((u32)plw, view, flags, &size);
                if (state & 1) {
                    switch (size) {
                    case 6:
                        draw_sprite_idx(0x976, &cell);
                        break;
                    case 5:
                        draw_sprite_idx(0x977, &cell);
                        break;
                    case 4:
                        draw_sprite_idx(0x978, &cell);
                        break;
                    }
                }
            }
            break;
        }
        case 4: {
            f32 deviation = 0.0f;
            state = gun_deviation_compare(plw, view, flags, &deviation);
            if (state & 1) {
                sprintf(text, "%1.2f%s", deviation, get_str_tbl(0x32)[0]);
                spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                draw_font(spr, text, 1, &cell);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                spr.color = 0x878787FF;
                draw_font(spr, (s8*)get_str_tbl(0x30)[0], 1, &cell);
            }
            break;
        }
        case 5: {
            u8 reload = 0;
            state = gun_reload_compare(plw, view, flags, &reload);
            if (state & 1) {
                strs = get_str_tbl(0x33);
                spr_data_copy(&spr, get_lsp_data(0x975, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                draw_font(spr, (s8*)strs[reload], 1, &cell);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x975, NULL));
                spr.color = 0x878787FF;
                draw_font(spr, (s8*)get_str_tbl(0x30)[0], 1, &cell);
            }
            break;
        }
        case 6: {
            s16 recoil = 0;
            state = gun_recoil_compare(plw, view, flags, &recoil);
            if (state & 1) {
                strs = get_str_tbl(0x34);
                spr_data_copy(&spr, get_lsp_data(0x975, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                draw_font(spr, (s8*)strs[recoil], 1, &cell);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x975, NULL));
                spr.color = 0x878787FF;
                draw_font(spr, (s8*)get_str_tbl(0x30)[0], 1, &cell);
            }
            break;
        }
        case 7:
            state = gun_recoil_text_get(plw, view, flags, text);
            if (state & 1) {
                spr_data_copy(&spr, get_lsp_data(0x975, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                draw_font(spr, text, 1, &cell);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x975, NULL));
                spr.color = 0x878787FF;
                draw_font(spr, (s8*)get_str_tbl(0x30)[0], 1, &cell);
            }
            break;
        case 8:
            if (hide) {
                if (kind == 0) {
                    spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                    spr.color = 0x878787FF;
                    draw_font(spr, (s8*)get_str_tbl(0x30)[0], 1, &cell);
                } else {
                    state = gun_bonus_flag_ck(plw, view, flags);
                    if (state & 1) {
                        draw_sprite_idx(0x979, &cell);
                    } else {
                        u8 bonus = 0;
                        state = gun_slot_bonus_get(plw, view, flags, &bonus);
                        if (state & 1) {
                            bonus_at = *(u32*)&cell;
                            value_badge_draw(0x97A, bonus, (_mh_ivec2_*)&bonus_at);
                        }
                    }
                }
            } else {
                spr_data_copy(&spr, get_lsp_data(0x973, NULL));
                spr.color = 0x878787FF;
                draw_font(spr, (s8*)get_str_tbl(0x30)[0], 1, &cell);
            }
            break;
        }
        if (!hide) {
            ids = (u16*)get_menu_lsp_tbl(0xC8);
            label_at = *(u32*)&label_base;
            put_lsp_anchor_offset(ids[i], &cell, &label_at);
            draw_sprite_idx(0x971, &cell);
            spr_data_copy(&label, get_lsp_data(0x970, NULL));
            if (skill && (state & 0x18)) {
                label.color = 0x50CD46FF;
            }
            if (state & 0x20) {
                label.color = 0x878787FF;
            }
            draw_font(label, (s8*)get_str_tbl(0x31)[i], 4, &cell);
        }
    }
}

/* 0x8030E6E4 (0xA0): Draws a value badge: the 0x889 frame at `pos` moved by sprite `id`'s anchor, and the value
 * printed with its sign in it. */
void value_badge_draw(u16 id, s32 value, _mh_ivec2_* pos) {
    s8 text[8];
    _SPR_DATA_ spr;

    spr_data_copy(&spr, get_lsp_data(id, NULL));
    pos->x += spr.pos.x;
    pos->y += spr.pos.y;
    draw_sprite_idx(0x889, pos);
    sprintf(text, "%+d", value);
    draw_font_idx(0x88A, text, 0, pos);
}

/* 0x80311C3C (0xC64): Draws the bowgun's ammo grid (`grid` 0: the single-column kinds, 1: the two-level kinds, 2: the
 * three-level kinds; `mode` bits 1/2 halve it, bit 2 picks the second half): per row the loadable mark and clip size
 * of each level, tinted by its comparison with the worn gun, and the ammo name. */
void ammo_grid_draw(_PLW* plw, const _mh_ivec2_* pos, _EQUIP_INDEX* view, u8 flags, u8 grid, u8 mode) {
    s32 rows;
    _mh_ivec2_ cell;
    _mh_ivec2_ base;
    u32 in_a;
    u32 in_b;
    u32 in_c;
    u32 in_d;
    s8 text[16];
    _SPR_DATA_ spr;
    u32 cell_in0;
    u32 cell_in1;
    u32 cell_in2;
    u8 count;
    s32 i;
    s32 row;
    u32 second;
    s32 skill;
    s32 colour;
    u8 state;
    u8 total;
    u8* ammo;
    u16* ids;

    if (mode & 8) {
        switch (grid) {
        case 2:
            in_a = *(const u32*)pos;
            put_lsp_anchor_offset(0x93C, &base, &in_a);
            break;
        case 1:
            in_b = *(const u32*)pos;
            put_lsp_anchor_offset(0x943, &base, &in_b);
            break;
        case 0:
            in_c = *(const u32*)pos;
            put_lsp_anchor_offset(0x94C, &base, &in_c);
            break;
        }
    } else {
        in_d = *(const u32*)pos;
        put_lsp_anchor_offset(0x922, &base, &in_d);
    }
    rows = 0;
    switch (grid) {
    case 2:
        rows = 6;
        if (mode & 6) {
            rows = 3;
        }
        break;
    case 0:
    case 1:
        rows = 8;
        if (mode & 6) {
            rows = 4;
        }
        break;
    }
    second = mode & 4;
    skill = flags & 4;
    colour = flags & 8;
    for (i = 0; i < rows; i++) {
        u32 skilled = 0;
        row = i;
        if (second) {
            row = rows + i;
        }
        switch (grid) {
        case 0:
            ids = (u16*)get_menu_lsp_tbl(0xC5);
            cell_in0 = *(u32*)&base;
            put_lsp_anchor_offset(ids[i], &cell, &cell_in0);
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0xC2), &cell);
            state = gun_ammo_usable_ck(plw, view, ammo_grid_single[row], flags);
            spr_data_copy(&spr, get_lsp_data(0x93B, NULL));
            if (state & 1) {
                spr.color = 0xAA825A7D;
            }
            draw_sprite(spr, &cell);
            state = gun_clip_compare(plw, view, ammo_grid_single[row], flags, &count);
            if (count != 0 && (state & 1)) {
                spr_data_copy(&spr, get_lsp_data(0x937, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                    skilled = 1;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                if (count == 0) {
                    spr.color = 0x878787FF;
                }
                sprintf(text, "%d", count);
                draw_font(spr, text, 1, &cell);
            }
            spr_data_copy(&spr, get_lsp_data(0x936, NULL));
            if (skilled == 1) {
                spr.color = 0x50CD46FF;
            }
            if (count == 0) {
                spr.color = 0x878787FF;
            }
            draw_font(spr, (s8*)get_str_tbl(0x3A)[row], 0, &cell);
            break;
        case 1:
            ids = (u16*)get_menu_lsp_tbl(0xC4);
            cell_in1 = *(u32*)&base;
            put_lsp_anchor_offset(ids[i], &cell, &cell_in1);
            if (row == 7) {
                draw_sprite_ary((u16*)get_menu_lsp_tbl(0xC2), &cell);
                state = gun_ammo_usable_ck(plw, view, ammo_grid_double[0][row], flags);
                spr_data_copy(&spr, get_lsp_data(0x933, NULL));
                if (state & 1) {
                    spr.color = 0xAA825A7D;
                }
                draw_sprite(spr, &cell);
                state = gun_clip_compare(plw, view, ammo_grid_double[0][row], flags, &count);
                if (count != 0 && (state & 1)) {
                    spr_data_copy(&spr, get_lsp_data(0x92E, NULL));
                    if (skill && (state & 0x18)) {
                        spr.color = 0x50CD46FF;
                        skilled = 1;
                    }
                    if (colour) {
                        if (state & 2) {
                            spr.color = 0x509BF0FF;
                        }
                        if (state & 4) {
                            spr.color = 0xE63737FF;
                        }
                    }
                    if (count == 0) {
                        spr.color = 0x878787FF;
                    }
                    sprintf(text, "%d", count);
                    draw_font(spr, text, 1, &cell);
                }
                total = count;
            } else {
                draw_sprite_ary((u16*)get_menu_lsp_tbl(0xC1), &cell);
                ammo = &ammo_grid_double[0][row];
                state = gun_ammo_usable_ck(plw, view, ammo[0], flags);
                spr_data_copy(&spr, get_lsp_data(0x933, NULL));
                if (state & 1) {
                    spr.color = 0xAA825A7D;
                }
                draw_sprite(spr, &cell);
                state = gun_clip_compare(plw, view, ammo[0], flags, &count);
                if (count != 0 && (state & 1)) {
                    spr_data_copy(&spr, get_lsp_data(0x92E, NULL));
                    if (skill && (state & 0x18)) {
                        spr.color = 0x50CD46FF;
                        skilled = 1;
                    }
                    if (colour) {
                        if (state & 2) {
                            spr.color = 0x509BF0FF;
                        }
                        if (state & 4) {
                            spr.color = 0xE63737FF;
                        }
                    }
                    if (count == 0) {
                        spr.color = 0x878787FF;
                    }
                    sprintf(text, "%d", count);
                    draw_font(spr, text, 1, &cell);
                }
                total = count;
                state = gun_ammo_usable_ck(plw, view, ammo[8], flags);
                spr_data_copy(&spr, get_lsp_data(0x934, NULL));
                if (state & 1) {
                    spr.color = 0xAA825A7D;
                }
                draw_sprite(spr, &cell);
                state = gun_clip_compare(plw, view, ammo[8], flags, &count);
                if (count != 0 && (state & 1)) {
                    spr_data_copy(&spr, get_lsp_data(0x92F, NULL));
                    if (skill && (state & 0x18)) {
                        spr.color = 0x50CD46FF;
                        skilled = 1;
                    }
                    if (colour) {
                        if (state & 2) {
                            spr.color = 0x509BF0FF;
                        }
                        if (state & 4) {
                            spr.color = 0xE63737FF;
                        }
                    }
                    if (count == 0) {
                        spr.color = 0x878787FF;
                    }
                    sprintf(text, "%d", count);
                    draw_font(spr, text, 1, &cell);
                }
                total += count;
            }
            spr_data_copy(&spr, get_lsp_data(0x92D, NULL));
            if (skilled == 1) {
                spr.color = 0x50CD46FF;
            }
            if (total == 0) {
                spr.color = 0x878787FF;
            }
            draw_font(spr, (s8*)get_str_tbl(0x39)[row], 0, &cell);
            break;
        case 2:
            ids = (u16*)get_menu_lsp_tbl(0xC3);
            cell_in2 = *(u32*)&base;
            put_lsp_anchor_offset(ids[i], &cell, &cell_in2);
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0xC0), &cell);
            ammo = &ammo_grid_triple[0][row];
            state = gun_ammo_usable_ck(plw, view, ammo[0], flags);
            spr_data_copy(&spr, get_lsp_data(0x929, NULL));
            if (state & 1) {
                spr.color = 0xAA825A7D;
            }
            draw_sprite(spr, &cell);
            state = gun_clip_compare(plw, view, ammo[0], flags, &count);
            if (count != 0 && (state & 1)) {
                spr_data_copy(&spr, get_lsp_data(0x924, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                    skilled = 1;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                if (count == 0) {
                    spr.color = 0x878787FF;
                }
                sprintf(text, "%d", count);
                draw_font(spr, text, 1, &cell);
            }
            total = count;
            state = gun_ammo_usable_ck(plw, view, ammo[6], flags);
            spr_data_copy(&spr, get_lsp_data(0x92A, NULL));
            if (state & 1) {
                spr.color = 0xAA825A7D;
            }
            draw_sprite(spr, &cell);
            state = gun_clip_compare(plw, view, ammo[6], flags, &count);
            if (count != 0 && (state & 1)) {
                spr_data_copy(&spr, get_lsp_data(0x925, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                    skilled = 1;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                if (count == 0) {
                    spr.color = 0x878787FF;
                }
                sprintf(text, "%d", count);
                draw_font(spr, text, 1, &cell);
            }
            total += count;
            state = gun_ammo_usable_ck(plw, view, ammo[12], flags);
            spr_data_copy(&spr, get_lsp_data(0x92B, NULL));
            if (state & 1) {
                spr.color = 0xAA825A7D;
            }
            draw_sprite(spr, &cell);
            state = gun_clip_compare(plw, view, ammo[12], flags, &count);
            if (count != 0 && (state & 1)) {
                spr_data_copy(&spr, get_lsp_data(0x926, NULL));
                if (skill && (state & 0x18)) {
                    spr.color = 0x50CD46FF;
                    skilled = 1;
                }
                if (colour) {
                    if (state & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (state & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                if (count == 0) {
                    spr.color = 0x878787FF;
                }
                sprintf(text, "%d", count);
                draw_font(spr, text, 1, &cell);
            }
            total += count;
            spr_data_copy(&spr, get_lsp_data(0x923, NULL));
            if (skilled == 1) {
                spr.color = 0x50CD46FF;
            }
            if (total == 0) {
                spr.color = 0x878787FF;
            }
            draw_font(spr, (s8*)get_str_tbl(0x38)[row], 0, &cell);
            break;
        }
    }
}


/* 0x80311AE8 (0x154): Draws the two level gauges (0x919 and 0x921 anchors, or sprite `id`'s position) filled to
 * `level` (0..2). */
void ammo_level_gauges_draw(const _mh_ivec2_* pos, u8 id, u8 level) {
    _mh_ivec2_ at;
    u32 in_a;
    u32 in_b;

    if (id == 0) {
        in_a = *(const u32*)pos;
        put_lsp_anchor_offset(0x919, &at, &in_a);
    } else {
        get_lsp_data(id, &at);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xBF), &at);
    switch (level) {
    case 2:
        draw_sprite_idx(0x91D, &at);
    case 1:
        draw_sprite_idx(0x91C, &at);
    case 0:
        draw_sprite_idx(0x91B, &at);
    }
    if (id == 0) {
        in_b = *(const u32*)pos;
        put_lsp_anchor_offset(0x921, &at, &in_b);
    } else {
        get_lsp_data(id, &at);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xBF), &at);
    switch (level) {
    case 2:
        draw_sprite_idx(0x91D, &at);
    case 1:
        draw_sprite_idx(0x91C, &at);
    case 0:
        draw_sprite_idx(0x91B, &at);
    }
}

/* 0x80312AF4 (0x2FC): Draws the bowgun stats panel for a column array of equipment (its frame, barrel and stock at
 * columns 6..8): copies the skills it activates into the hunter's shown-skill list, then the tabs, the part and
 * whole-gun columns and the parts list. */
void gun_stats_columns_draw(_PLW* plw, _EQUIP* columns, u16 anchor) {
    _EQUIP_INDEX view;
    u16 skill_ids[8];
    s8 skill_points[8];
    u8 skill_trees[8];
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 tabs_at;
    u32 frame_at;
    u32 barrel_at;
    u32 stock_at;
    u32 gun_at;
    u32 runs_at;
    u32 parts_at;
    u8 flags;
    s32 i;
    s32 count;

    if (anchor != 0) {
        get_lsp_data(0x955, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    equip_columns_skills_get(columns, skill_ids, skill_points, skill_trees);
    count = 0;
    for (i = 0; i < 8; i++) {
        if (skill_ids[i] != 0 && skill_trees[i] != 0) {
            plw->shown_skill_id[count] = skill_ids[i];
            plw->shown_skill_tree[count] = skill_trees[i];
            count++;
        }
    }
    tabs_at = *(u32*)&off;
    gun_stats_tabs_draw((const _mh_ivec2_*)&tabs_at, 0xFF);
    flags = 7;
    if (equip_variant_resolve(plw, &columns[6], NULL, &flags, &view) != 1) {
        frame_at = *(u32*)&off;
        gun_stats_column_draw(plw, &view, flags, (const _mh_ivec2_*)&frame_at);
    }
    flags = 7;
    if (equip_variant_resolve(plw, &columns[7], NULL, &flags, &view) != 1) {
        barrel_at = *(u32*)&off;
        gun_stats_column_draw(plw, &view, flags, (const _mh_ivec2_*)&barrel_at);
    }
    flags = 7;
    if (equip_variant_resolve(plw, &columns[8], NULL, &flags, &view) != 1) {
        stock_at = *(u32*)&off;
        gun_stats_column_draw(plw, &view, flags, (const _mh_ivec2_*)&stock_at);
    }
    flags = 7;
    if (equip_variant_resolve_ex(plw, &columns[6], &columns[7], &columns[8], 7, &view) != 1) {
        gun_at = *(u32*)&off;
        gun_stats_column_draw(plw, &view, flags, (const _mh_ivec2_*)&gun_at);
    }
    runs_at = *(u32*)&off;
    put_lsp_sprite_runs(&runs_at);
    parts_at = *(u32*)&off;
    gun_parts_names_draw((const _mh_ivec2_*)&parts_at, &view);
    shown_skills_clear(plw, flags);
}

/* 0x8031165C (0x7C): The `put_equip_panel_row_ex` panel's entry for two bowgun triples (resolved by
 * `equip_variant_resolve_pair`). */
void ammo_panel_entry_pair(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, _EQUIP* other_frame,
                 _EQUIP* other_barrel, _EQUIP* other_stock, u16 anchor, u8 flags, u8 grid) {
    _EQUIP_INDEX view;

    if (equip_variant_resolve_pair(plw, frame, barrel, stock, other_frame, other_barrel, other_stock, &flags, &view) == 1) {
        return;
    }
    put_equip_panel_row_ex(plw, &view, anchor, flags, grid);
}

/* 0x803116D8 (0x168): Draws one ammo grid panel at the 0x917 anchor: the "no ammo" text for an empty piece, else
 * the panel frame, the level gauges and the ammo grid `grid`. */
void put_equip_panel_row_ex(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, u8 grid) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;
    u32 frame_at;
    u32 gauge_at;
    u32 grid_at;

    if (anchor != 0) {
        get_lsp_data(0x917, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(0x917, &pos, &in);
    switch (equip_wear_check(plw, view, flags)) {
    case 1:
        return;
    case 2:
        draw_font_idx(0x8D2, (s8*)get_str_tbl(0x2E)[1], 0, &pos);
        return;
    default:
        shown_skills_set(plw, view, flags);
        frame_at = *(u32*)&off;
        put_lsp_sprite_at_anchor(&frame_at, 0);
        gauge_at = *(u32*)&off;
        ammo_level_gauges_draw((const _mh_ivec2_*)&gauge_at, 0, grid);
        grid_at = *(u32*)&off;
        ammo_grid_draw(plw, (const _mh_ivec2_*)&grid_at, view, flags, grid, 0);
        shown_skills_clear(plw, flags);
        break;
    }
}

/* 0x80311920 (0x168): Draws the full ammo panel at the 0x917 anchor: the panel frame, the level gauges and all
 * three ammo grids. */
void put_equip_panel_row_base(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 frame_at;
    u32 gauge_at;
    u32 single_at;
    u32 double_at;
    u32 triple_at;

    switch (equip_wear_check(plw, view, flags)) {
    case 1:
        return;
    }
    if (anchor != 0) {
        get_lsp_data(0x917, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    shown_skills_set(plw, view, flags);
    frame_at = *(u32*)&off;
    put_lsp_sprite_at_anchor(&frame_at, 0);
    gauge_at = *(u32*)&off;
    ammo_level_gauges_draw((const _mh_ivec2_*)&gauge_at, 0, 2);
    single_at = *(u32*)&off;
    ammo_grid_draw(plw, (const _mh_ivec2_*)&single_at, view, flags, 0, 8);
    double_at = *(u32*)&off;
    ammo_grid_draw(plw, (const _mh_ivec2_*)&double_at, view, flags, 1, 8);
    triple_at = *(u32*)&off;
    ammo_grid_draw(plw, (const _mh_ivec2_*)&triple_at, view, flags, 2, 8);
    shown_skills_clear(plw, flags);
}

/* 0x80310F30 (0x2DC): Draws the active-skills panel for the shown equipment (`active_skills_list`): eight rows of skill
 * names, red for a negative skill, gold for a new one and grey for one the class cannot use, each filed as a help
 * record; "no skills" when there are none.  Returns 1 on a missing record, 4 for no skills, else 0. */
u8 active_skills_page_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags) {
    _SPR_DATA_ spr;
    u16 ids[8];
    PlaceRec rec;
    u8 marks[8];
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _mh_ivec2_ off;
    u32 in;
    u32 row_in;
    u8 r;
    u8 count;
    s32 i;
    u16* rows;
    s16 x;
    s16 y;
    s16 width;
    s16 height;
    s16 extra;

    if (anchor != 0) {
        get_lsp_data(1, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(1, &pos, &in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xBC), &pos);
    r = equip_wear_check(plw, view, flags);
    if ((s32)r == 1) {
        return r;
    }
    count = active_skills_list(plw, view, flags, ids, marks);
    if (count == 0) {
        draw_font_idx(3, (s8*)get_str_tbl(0x2E)[5], 0, &pos);
        return 4;
    }
    ainpc_page_mark_a_draw(12, &pos);
    rows = (u16*)get_menu_lsp_tbl(0xBD);
    for (i = 0; i < 8; i++) {
        row_in = *(u32*)&pos;
        put_lsp_anchor_offset(rows[i], &at, &row_in);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xBE), &at);
        if (i < count && ids[i] != 0) {
            spr_data_copy(&spr, get_lsp_data(14, NULL));
            if (flags & 4) {
                if (marks[i] & 0x10) {
                    spr.color = 0xE63737FF;
                }
                if (marks[i] & 8) {
                    spr.color = 0xEBC855FF;
                }
                if (marks[i] & 0x20) {
                    spr.color = 0x878787FF;
                }
            }
            draw_font(spr, (s8*)skill_name_get(ids[i]), 1, &at);
            place_rec_skill_set(&rec, ids[i]);
            spr_data_copy(&spr, get_lsp_data(15, NULL));
            x = spr.pos.x + at.x;
            y = spr.pos.y + at.y;
            width = spr.width;
            height = spr.height;
            spr_data_copy(&spr, get_lsp_data(16, NULL));
            extra = spr.width;
            width += extra < 0 ? -extra : extra;
            place_rec_alloc(&rec, x, y, width, height, 12, 9);
        }
    }
    return 0;
}

/* 0x8031120C (0x334): Draws the active-skills panel for a column array of equipment (`equip_columns_skills_get`):
 * eight rows of skill names, red for a negative skill and grey for one the class cannot use, each filed as a help
 * record; "no skills" when there are none. */
void column_skills_page_draw(_PLW* plw, _EQUIP* columns, u16 anchor) {
    _SPR_DATA_ spr;
    u16 ids[8];
    PlaceRec rec;
    s8 levels[8];
    u8 trees[8];
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    _mh_ivec2_ off;
    u32 in;
    u32 row_in;
    s32 count;
    s32 i;
    u16* rows;
    u8 mark;
    s16 x;
    s16 y;
    s16 width;
    s16 height;
    s16 extra;

    if (anchor != 0) {
        get_lsp_data(1, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(1, &pos, &in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xBC), &pos);
    ainpc_page_mark_a_draw(12, &pos);
    equip_columns_skills_get(columns, ids, levels, trees);
    count = 0;
    for (i = 0; i < 8; i++) {
        if (ids[i] != 0) {
            count++;
        }
    }
    if (count == 0) {
        draw_font_idx(3, (s8*)get_str_tbl(0x2E)[5], 0, &pos);
        return;
    }
    rows = (u16*)get_menu_lsp_tbl(0xBD);
    for (i = 0; i < 8; i++) {
        row_in = *(u32*)&pos;
        put_lsp_anchor_offset(rows[i], &at, &row_in);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xBE), &at);
        if (ids[i] != 0) {
            mark = 0;
            if (equip_columns_tree_points_get(columns, trees[i]) < 0) {
                mark |= 0x10;
            }
            if (pl_equip_skill_id_valid_ck(plw, &columns[6], &columns[7], ids[i]) == 0) {
                mark |= 0x20;
            }
            spr_data_copy(&spr, get_lsp_data(14, NULL));
            if (mark & 0x10) {
                spr.color = 0xE63737FF;
            }
            if (mark & 0x20) {
                spr.color = 0x878787FF;
            }
            draw_font(spr, (s8*)skill_name_get(ids[i]), 1, &at);
            place_rec_skill_set(&rec, ids[i]);
            spr_data_copy(&spr, get_lsp_data(15, NULL));
            x = spr.pos.x + at.x;
            y = spr.pos.y + at.y;
            width = spr.width;
            height = spr.height;
            spr_data_copy(&spr, get_lsp_data(16, NULL));
            extra = spr.width;
            width += extra < 0 ? -extra : extra;
            place_rec_alloc(&rec, x, y, width, height, 12, 9);
        }
    }
}

/* 0x8030FDA0 (0x154): Draws the armour skills page at the 0x8C4 anchor: "empty" for an empty piece, else the skill
 * rows (`skill_rows_draw`, six rows). */
u8 armor_skills_page_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;
    u32 rows_at;
    u8 r;

    if (anchor != 0) {
        get_lsp_data(0x8C4, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(0x8C4, &pos, &in);
    draw_sprite_idx(0x8C5, &pos);
    ainpc_page_mark_a_draw(0x8CF, &pos);
    r = equip_wear_check(plw, view, flags);
    switch (r) {
    case 1:
        return r;
    case 2:
        draw_font_idx(0x8D2, (s8*)get_str_tbl(0x2E)[1], 0, &pos);
        return r;
    }
    rows_at = *(u32*)&pos;
    return skill_rows_draw(plw, view, flags, (u16*)get_menu_lsp_tbl(0xB6), 0, (const _mh_ivec2_*)&rows_at);
}

/* 0x8030FF0C (0x224): Draws a weapon's skills page at the 0x8D0 anchor: "empty", the class warnings (red, unless
 * flag bit 6) or the skill rows (`skill_rows_draw`, ten rows). */
u8 weapon_skills_page_draw(_PLW* plw, _EQUIP* equip, _EQUIP* other, u16 anchor, u8 flags) {
    _SPR_DATA_ spr;
    _EQUIP_INDEX view;
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;
    u32 rows_at;
    u8 r;

    r = equip_variant_resolve(plw, equip, other, &flags, &view);
    if (r == 1) {
        return r;
    }
    if (anchor != 0) {
        get_lsp_data(0x8D0, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(0x8D0, &pos, &in);
    draw_sprite_idx(0x8D1, &pos);
    ainpc_page_mark_a_draw(0x8DF, &pos);
    r = equip_wear_check(plw, &view, flags);
    switch (r) {
    case 6:
        if (!(flags & 0x40)) {
            spr_data_copy(&spr, get_lsp_data(0x8D2, NULL));
            spr.color = 0xE63737FF;
            draw_font(spr, (s8*)get_str_tbl(0x2E)[10], 0, &pos);
            return r;
        }
        break;
    case 7:
        if (!(flags & 0x40)) {
            spr_data_copy(&spr, get_lsp_data(0x8D2, NULL));
            spr.color = 0xE63737FF;
            draw_font(spr, (s8*)get_str_tbl(0x2E)[9], 0, &pos);
            return r;
        }
        break;
    case 2:
        draw_font_idx(0x8D2, (s8*)get_str_tbl(0x2E)[1], 0, &pos);
        return r;
    case 1:
        return r;
    }
    rows_at = *(u32*)&pos;
    return skill_rows_draw(plw, &view, flags, (u16*)get_menu_lsp_tbl(0xB7), 1, (const _mh_ivec2_*)&rows_at);
}

/* 0x80310130 (0x418): Draws the skill rows of the shown piece (six or ten by `page`): each skill's tree name filed as a
 * help record, its points and its decorations' points, gold for a new skill and grey for one the class cannot use,
 * and the decoration markers of the two columns. */
u8 skill_rows_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* rows, u8 page, const _mh_ivec2_* pos) {
    _SPR_DATA_ spr;
    u8 skills[10];
    s8 points[10];
    s8 deco_points[10];
    u8 marks[10];
    PlaceRec rec;
    s8 text[8];
    _mh_ivec2_ at;
    u32 in;
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    s32 count = 0;
    s32 left_marked = 0;
    s32 right_marked = 0;
    s32 i;
    u16* cells;

    if ((frame != NULL && barrel != NULL) || stock != NULL) {
        if (frame->item_id == 0 || barrel->item_id == 0 || stock->item_id == 0) {
            return 2;
        }
    } else if (frame != NULL && frame->item_id == 0) {
        return 2;
    }
    if (equip_skill_trees_list(plw, view, flags, skills, points, deco_points, marks) == 0) {
        draw_font_idx(0x8D2, (s8*)get_str_tbl(0x2E)[2], 0, pos);
        return 4;
    }
    if (page == 0) {
        count = 6;
    }
    if (page == 1) {
        count = 10;
    }
    for (i = 0; i < count; i++) {
        in = *(const u32*)pos;
        put_lsp_anchor_offset(rows[i], &at, &in);
        if (i & 2) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0xB9), &at);
            cells = (u16*)get_menu_lsp_tbl(0xBB);
        } else {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0xB8), &at);
            cells = (u16*)get_menu_lsp_tbl(0xBA);
        }
        if (skills[i] != 0) {
            spr_data_copy(&spr, get_lsp_data(cells[0], NULL));
            if (marks[i] & 8) {
                spr.color = 0xEBC855FF;
            }
            if (marks[i] & 0x20) {
                spr.color = 0x878787FF;
            }
            draw_font(spr, (s8*)item_kind_name_get(skills[i]), 0, &at);
            menu_row_rec_init(&rec, skills[i]);
            place_rec_alloc_idx(&rec, 0x8E4, &at, -4, 9);
            if (points[i] != 0 && skills[i] != 1) {
                sprintf(text, "%+d", points[i]);
                spr_data_copy(&spr, get_lsp_data(cells[1], NULL));
                if (flags & 8) {
                    if (marks[i] & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (marks[i] & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                if (marks[i] & 8) {
                    spr.color = 0xEBC855FF;
                }
                if (marks[i] & 0x20) {
                    spr.color = 0x878787FF;
                }
                draw_font(spr, text, 1, &at);
            }
            if (deco_points[i] != 0) {
                draw_sprite_idx(cells[2], &at);
                sprintf(text, "%+d", deco_points[i]);
                spr_data_copy(&spr, get_lsp_data(cells[3], NULL));
                draw_font(spr, text, 0, &at);
                if (i & 1) {
                    if (right_marked == 0) {
                        right_marked = 1;
                        if (page == 0) {
                            draw_sprite_idx(0x8C8, pos);
                        }
                        if (page == 1) {
                            draw_sprite_idx(0x8D4, pos);
                        }
                    }
                } else if (left_marked == 0) {
                    left_marked = 1;
                    if (page == 0) {
                        draw_sprite_idx(0x8C7, pos);
                    }
                    if (page == 1) {
                        draw_sprite_idx(0x8D3, pos);
                    }
                }
            }
        }
    }
    font_flush();
    return 0;
}

/* 0x8031077C (0x57C): Draws the decorations page at `pos`: the class warnings (red, unless flag bit 6), "empty",
 * else the slot frames and, per set decoration, its name (cut to six characters) and the two skills it adds,
 * each filed as a help record; with no decoration the slot count text.  Returns 0, 1, 2/6/7 (warnings), 4 or 5. */
u8 deco_page_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, const _mh_ivec2_* pos) {
    _SPR_DATA_ warn;
    _SPR_DATA_ spr;
    s8 text[16];
    u16 ids[5];
    PlaceRec rec;
    u8 marks[5];
    _mh_ivec2_ at;
    u32 frame_in;
    u32 row_in;
    u8 r;
    u8 slots;
    u8 filled;
    s32 drawn;
    s32 j;
    s32 k;
    char* cut;
    u8 skill;
    s32 points;

    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xB3), pos);
    ainpc_page_mark_a_draw(0x905, pos);
    r = equip_wear_check(plw, view, flags);
    switch (r) {
    case 6:
        if (!(flags & 0x40)) {
            spr_data_copy(&warn, get_lsp_data(0x901, NULL));
            warn.color = 0xE63737FF;
            draw_font(warn, (s8*)get_str_tbl(0x2E)[10], 0, pos);
            return r;
        }
        break;
    case 7:
        if (!(flags & 0x40)) {
            spr_data_copy(&warn, get_lsp_data(0x901, NULL));
            warn.color = 0xE63737FF;
            draw_font(warn, (s8*)get_str_tbl(0x2E)[9], 0, pos);
            return r;
        }
        break;
    case 2:
        draw_font_idx(0x901, (s8*)get_str_tbl(0x2E)[1], 0, pos);
        return r;
    case 1:
        return r;
    }
    slots = deco_slots_view_draw(0x8F0, view, flags, pos);
    if (slots == 0) {
        draw_font_idx(0x901, (s8*)get_str_tbl(0x2E)[3], 0, pos);
        return 4;
    }
    filled = equip_decos_compare(plw, view, flags, ids, marks);
    drawn = 0;
    for (j = 0; j < 5; j++) {
        if (ids[j] != 0 && ids[j] < 0x84 && (filled & (1 << j)) && (marks[j] & 1)) {
            if (drawn == 0) {
                for (k = 0; k < slots; k++) {
                    if (((filled & 8) == 0 || k != 1) && ((filled & 0x10) == 0 || (k != 1 && k != 2))) {
                        u16* frames = (u16*)get_menu_lsp_tbl(0xB4);
                        frame_in = *(const u32*)pos;
                        put_lsp_anchor_offset(frames[k], &at, &frame_in);
                        draw_sprite_ary((u16*)get_menu_lsp_tbl(0xB5), &at);
                    }
                }
            }
            drawn = 1;
            {
                u16* frames = (u16*)get_menu_lsp_tbl(0xB4);
                row_in = *(const u32*)pos;
                put_lsp_anchor_offset(frames[j], &at, &row_in);
            }
            strcpy((char*)text, (const char*)ItemName(get_item_species_data(ids[j])->item_id));
            if (flfntStrLen((char*)text) >= 9) {
                cut = flKnjMsgNumPtr((char*)text, 6);
                if (cut != NULL) {
                    *cut = 0;
                    strcat((char*)text, "...");
                }
            }
            draw_font_idx(0x907, text, 0, &at);
            skill = deco_skill_id_get(ids[j], 0);
            points = deco_skill_points_get(ids[j], 0);
            spr_data_copy(&spr, get_lsp_data(0x908, NULL));
            draw_font(spr, (s8*)item_kind_name_get(skill), 0, &at);
            if (skill != 0) {
                menu_row_rec_init(&rec, skill);
                place_rec_alloc_idx(&rec, 0x90E, &at, -4, 9);
            }
            if (points != 0) {
                sprintf(text, "%+d", points);
                spr_data_copy(&spr, get_lsp_data(0x909, NULL));
                draw_font(spr, text, 0, &at);
            }
            skill = deco_skill_id_get(ids[j], 1);
            points = deco_skill_points_get(ids[j], 1);
            spr_data_copy(&spr, get_lsp_data(0x90A, NULL));
            draw_font(spr, (s8*)item_kind_name_get(skill), 0, &at);
            if (skill != 0) {
                menu_row_rec_init(&rec, skill);
                place_rec_alloc_idx(&rec, 0x911, &at, -4, 9);
            }
            if (points != 0) {
                sprintf(text, "%+d", points);
                spr_data_copy(&spr, get_lsp_data(0x90B, NULL));
                draw_font(spr, text, 0, &at);
            }
        }
    }
    font_flush();
    if (drawn == 0) {
        if (slots == 0) {
            draw_font_idx(0x901, (s8*)get_str_tbl(0x2E)[3], 0, pos);
            return 4;
        }
        sprintf(text, "%s%d", get_str_tbl(0x2E)[4], slots);
        draw_font_idx(0x901, text, 0, pos);
        return 5;
    }
    return 0;
}

/* 0x8030F38C (0x148): Draws a piece's decoration-skills page at the 0x88B anchor: "empty" for an empty piece, else
 * `deco_skills_rows_draw`'s skill and slot rows. */
void deco_skills_page_draw(_PLW* plw, _EQUIP* equip, _EQUIP* other, u16 anchor, u8 flags) {
    _EQUIP_INDEX view;
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;

    if (anchor != 0) {
        get_lsp_data(0x88B, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    if (equip_variant_resolve(plw, equip, other, &flags, &view) == 1) {
        return;
    }
    switch (equip_wear_check(plw, &view, flags)) {
    case 2:
        in = *(u32*)&off;
        put_lsp_anchor_offset(0x88B, &pos, &in);
        draw_font_idx(0x8D2, (s8*)get_str_tbl(0x2E)[1], 0, &pos);
        return;
    case 1:
        return;
    }
    deco_skills_rows_draw(plw, &view, anchor, flags, &off);
}

/* 0x8030F4D4 (0x2DC): Draws the two decoration skills of the shown piece at `pos`: each skill's tree name filed as a
 * help record and its points, gold for a new skill and grey for one the class cannot use; "none" texts for no skill
 * and no slot, and the slot frames. */
void deco_skills_rows_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, const _mh_ivec2_* pos) {
    _SPR_DATA_ spr;
    PlaceRec rec;
    s8 text[8];
    _mh_ivec2_ at;
    u16 ids[2];
    u32 in;
    s8 levels[2];
    u8 marks[2];
    u8 count;
    s32 i;
    u16** frames;

    in = *(const u32*)pos;
    put_lsp_anchor_offset(0x88B, &at, &in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xA1), &at);
    ainpc_page_mark_a_draw(0x8A6, &at);
    count = equip_deco_skills_compare(plw, view, flags, ids, levels, marks);
    for (i = 0; i < 2; i++) {
        if (ids[i] != 0) {
            frames = (u16**)get_menu_lsp_tbl(0xA4);
            draw_sprite_ary(frames[i], &at);
            spr_data_copy(&spr, get_lsp_data(((u16*)get_menu_lsp_tbl(0xA5))[i], NULL));
            if (flags & 4) {
                if (marks[i] & 8) {
                    spr.color = 0xEBC855FF;
                }
                if (marks[i] & 0x20) {
                    spr.color = 0x878787FF;
                }
            }
            draw_font(spr, (s8*)item_kind_name_get(ids[i]), 0, &at);
            menu_row_rec_init(&rec, ids[i]);
            place_rec_alloc_idx(&rec, frames[i][1], &at, 0, 9);
            if (levels[i] != 0) {
                spr_data_copy(&spr, get_lsp_data(((u16*)get_menu_lsp_tbl(0xA6))[i], NULL));
                if (flags & 8) {
                    if (marks[i] & 2) {
                        spr.color = 0x509BF0FF;
                    }
                    if (marks[i] & 4) {
                        spr.color = 0xE63737FF;
                    }
                }
                if (flags & 4) {
                    if (marks[i] & 8) {
                        spr.color = 0xEBC855FF;
                    }
                    if (marks[i] & 0x20) {
                        spr.color = 0x878787FF;
                    }
                }
                sprintf(text, "%+d", levels[i]);
                draw_font(spr, text, 1, &at);
            }
        }
    }
    if (count == 0) {
        draw_font_idx(0x88E, (s8*)get_str_tbl(0x2E)[0], 0, &at);
    }
    if (deco_slots_view_draw(0x8A5, view, flags, &at) == 0) {
        draw_font_idx(0x894, (s8*)get_str_tbl(0x2E)[0], 1, &at);
    }
}

/* 0x8030F87C (0x244): Draws a weapon's description page at the 0x8A7 anchor: the class warnings (red, unless flag
 * bit 6), "empty", else the description text and, for a switch axe, its phial. */
void weapon_desc_page_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags) {
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;
    _EQUIP* shown = view->shown[0];
    PlSkillRec* rec;

    if (anchor != 0) {
        get_lsp_data(0x8A7, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(0x8A7, &pos, &in);
    switch (equip_wear_check(plw, view, flags)) {
    case 6:
        if (!(flags & 0x40)) {
            spr_data_copy(&spr, get_lsp_data(0x8D2, NULL));
            spr.color = 0xE63737FF;
            draw_font(spr, (s8*)get_str_tbl(0x2E)[10], 0, &pos);
            return;
        }
        break;
    case 7:
        if (!(flags & 0x40)) {
            spr_data_copy(&spr, get_lsp_data(0x8D2, NULL));
            spr.color = 0xE63737FF;
            draw_font(spr, (s8*)get_str_tbl(0x2E)[9], 0, &pos);
            return;
        }
        break;
    case 2:
        draw_font_idx(0x8D2, (s8*)get_str_tbl(0x2E)[1], 0, &pos);
        return;
    case 1:
        return;
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xA7), &pos);
    draw_font_idx(0x8A8, (s8*)equip_info_text_get(shown->kind, shown->item_id), 0, &pos);
    if (shown->kind == 15) {
        rec = equip_blade_rec_get(shown);
        if (rec != NULL) {
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0xA8), &pos);
            s8** strs = (s8**)get_str_tbl(0x2F);
            draw_font_idx(0x8A9, strs[rec->phial_type], 1, &pos);
        }
    }
}

/* 0x8030E5D8 (0x10C): Draws the three bowgun part tabs at `pos`, the shown part's tab (frame, barrel or stock) in its
 * selected frame; class 3/4/7/8 (the whole gun) selects all three. */
void gun_part_tabs_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, const _mh_ivec2_* pos) {
    _EQUIP* shown = NULL;
    u16 frame_tab = 3;
    u16 barrel_tab = 3;
    u16 stock_tab = 3;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
    case 5:
    case 6:
        shown = view->shown[0];
        break;
    case 3:
    case 4:
    case 7:
    case 8:
        shown = NULL;
        frame_tab = 0;
        barrel_tab = 0;
        stock_tab = 0;
        break;
    }
    if (shown != NULL) {
        switch (shown->kind) {
        case 11:
            frame_tab = 0;
            break;
        case 12:
            barrel_tab = 0;
            break;
        case 13:
            stock_tab = 0;
            break;
        }
    }
    draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(0x97), frame_tab, pos);
    draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(0x98), barrel_tab, pos);
    draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(0x99), stock_tab, pos);
}

/* 0x8030E79C (0x26C): Draws an armour piece's stats page at the 0x826 anchor: the class warnings (red, unless flag
 * bit 6), "empty", else the defence/level block and the resistances. */
void armor_stats_page_draw(_PLW* plw, _EQUIP* equip, _EQUIP* other, u16 anchor, u8 flags) {
    _SPR_DATA_ spr;
    _EQUIP_INDEX view;
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in_a;
    u32 in_b;
    u32 in_c;

    if (anchor != 0) {
        get_lsp_data(0x826, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    if (equip_variant_resolve(plw, equip, other, &flags, &view) == 1) {
        return;
    }
    switch (equip_wear_check(plw, &view, flags)) {
    case 6:
        if (!(flags & 0x40)) {
            in_a = *(u32*)&off;
            put_lsp_anchor_offset(0x826, &pos, &in_a);
            spr_data_copy(&spr, get_lsp_data(0x8D2, NULL));
            spr.color = 0xE63737FF;
            draw_font(spr, (s8*)get_str_tbl(0x2E)[10], 0, &pos);
            return;
        }
        break;
    case 7:
        if (!(flags & 0x40)) {
            in_b = *(u32*)&off;
            put_lsp_anchor_offset(0x826, &pos, &in_b);
            spr_data_copy(&spr, get_lsp_data(0x8D2, NULL));
            spr.color = 0xE63737FF;
            draw_font(spr, (s8*)get_str_tbl(0x2E)[9], 0, &pos);
            return;
        }
        break;
    case 2:
        in_c = *(u32*)&off;
        put_lsp_anchor_offset(0x826, &pos, &in_c);
        draw_font_idx(0x8D2, (s8*)get_str_tbl(0x2E)[1], 0, &pos);
        return;
    case 1:
        return;
    }
    shown_skills_set(plw, &view, flags);
    armor_defense_block_draw(plw, &view, 0, flags, &off);
    armor_resist_block_draw(plw, &view, 0, flags, &off);
    shown_skills_clear(plw, flags);
}

/* 0x8030EA08 (0x2DC): Draws an armour piece's defence and upgrade level (white at its cap, with the cap mark), its
 * blademaster/gunner marks and its slot row, at the 0x826 anchor or at sprite `anchor`. */
void armor_defense_block_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, const _mh_ivec2_* off) {
    _SPR_DATA_ spr;
    s8 text[8];
    _mh_ivec2_ pos;
    u32 in;
    u16 defense;
    u8 level;
    u8 state;
    u8 level_state;
    s32 capped;
    PlSlotRec* rec;

    if (anchor == 0) {
        in = *(const u32*)off;
        put_lsp_anchor_offset(0x826, &pos, &in);
    } else if (off == NULL) {
        get_lsp_data(anchor, &pos);
    } else {
        return;
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x9D), &pos);
    defense = 0;
    state = equip_defense_compare(plw, view, flags, &defense);
    if (state & 1) {
        spr_data_copy(&spr, get_lsp_data(0x82B, NULL));
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", defense);
        draw_font(spr, text, 1, &pos);
    }
    level = 0;
    level_state = equip_level_compare(plw, view, flags, &level);
    if (level_state & 1) {
        capped = level_state & 0x40;
        if (capped) {
            spr_data_copy(&spr, get_lsp_data(0x82C, NULL));
            spr.color = 0xF0F0F0FF;
        } else {
            spr_data_copy(&spr, get_lsp_data(0x82D, NULL));
        }
        if (flags & 8) {
            if (level_state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (level_state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", level);
        draw_font(spr, text, 1, &pos);
        if (capped) {
            draw_sprite_idx(0x827, &pos);
        }
    }
    rec = equip_slot_rec_get_c1(view->shown[0]);
    if (rec != NULL) {
        if (rec->wear_flags & 4) {
            draw_sprite_idx(0x83C, &pos);
        }
        if (rec->wear_flags & 8) {
            draw_sprite_idx(0x83D, &pos);
        }
    }
    if (deco_slots_view_draw(0x83B, view, flags, &pos) == 0) {
        draw_font_idx(0x82E, (s8*)get_str_tbl(0x2E)[0], 1, &pos);
    }
}

/* 0x8030ECE4 (0x690): Draws an armour piece's five resistances (fire, water, dragon, thunder, ice) at the 0x83E
 * anchor or at sprite `anchor`: each value and its element icon, green when a skill changes it and blue/red against
 * the worn piece's. */
void armor_resist_block_draw(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, const _mh_ivec2_* off) {
    _SPR_DATA_ spr;
    s8 text[8];
    _mh_ivec2_ pos;
    u32 in;
    s16 value;
    u8 state;

    equip_slot_rec_get_c1(view->shown[0]);
    value = 0;
    if (anchor == 0) {
        in = *(const u32*)off;
        put_lsp_anchor_offset(0x83E, &pos, &in);
    } else if (off == NULL) {
        get_lsp_data(anchor, &pos);
    } else {
        return;
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x9E), &pos);
    value = 0;
    state = equip_resist_compare(plw, view, flags, 1, &value);
    if (state & 1) {
        s32 skill;

        spr_data_copy(&spr, get_lsp_data(0x844, NULL));
        skill = flags & 4;
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", value);
        draw_font(spr, text, 2, &pos);
        spr_data_copy(&spr, get_lsp_data(0x83F, NULL));
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_sprite(spr, &pos);
    }
    value = 0;
    state = equip_resist_compare(plw, view, flags, 2, &value);
    if (state & 1) {
        s32 skill;

        spr_data_copy(&spr, get_lsp_data(0x845, NULL));
        skill = flags & 4;
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", value);
        draw_font(spr, text, 2, &pos);
        spr_data_copy(&spr, get_lsp_data(0x840, NULL));
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_sprite(spr, &pos);
    }
    value = 0;
    state = equip_resist_compare(plw, view, flags, 5, &value);
    if (state & 1) {
        s32 skill;

        spr_data_copy(&spr, get_lsp_data(0x846, NULL));
        skill = flags & 4;
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", value);
        draw_font(spr, text, 2, &pos);
        spr_data_copy(&spr, get_lsp_data(0x841, NULL));
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_sprite(spr, &pos);
    }
    value = 0;
    state = equip_resist_compare(plw, view, flags, 3, &value);
    if (state & 1) {
        s32 skill;

        spr_data_copy(&spr, get_lsp_data(0x847, NULL));
        skill = flags & 4;
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", value);
        draw_font(spr, text, 2, &pos);
        spr_data_copy(&spr, get_lsp_data(0x842, NULL));
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_sprite(spr, &pos);
    }
    value = 0;
    state = equip_resist_compare(plw, view, flags, 4, &value);
    if (state & 1) {
        s32 skill;

        spr_data_copy(&spr, get_lsp_data(0x848, NULL));
        skill = flags & 4;
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", value);
        draw_font(spr, text, 2, &pos);
        spr_data_copy(&spr, get_lsp_data(0x843, NULL));
        if (skill && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_sprite(spr, &pos);
    }
}

/* 0x8030D798 (0x70): The `gun_basis_panel_draw` panel's entry for two bowgun triples (resolved by `equip_variant_resolve_pair`). */
u8 gun_basis_entry_pair(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, _EQUIP* other_frame, _EQUIP* other_barrel,
               _EQUIP* other_stock, u16 anchor, u8 flags) {
    _EQUIP_INDEX view;
    u8 r = equip_variant_resolve_pair(plw, frame, barrel, stock, other_frame, other_barrel, other_stock, &flags, &view);
    if (r == 1) {
        return r;
    }
    return gun_basis_panel_draw(plw, &view, anchor, flags);
}

/* 0x8030D980 (0x360): Draws a bowgun's attack and affinity (green when a skill changes them, blue/red against the
 * worn gun) with their icons, its slot row and its part tabs, at the 0x84E anchor or at sprite `anchor`. */
void Put_equip_dtl_basis_bowgun_gan1(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, _mh_ivec2_* off) {
    _SPR_DATA_ spr;
    s8 text[8];
    _mh_ivec2_ pos;
    u32 in;
    u16 attack;
    s16 affinity;
    u8 state;

    if (anchor == 0) {
        in = *(u32*)off;
        put_lsp_anchor_offset(0x84E, &pos, &in);
    } else if (off == NULL) {
        get_lsp_data(anchor, &pos);
    } else {
        return;
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x96), &pos);
    attack = 0;
    state = gun_attack_compare(plw, view, flags, &attack);
    if (state & 1) {
        sprintf(text, "%d", attack);
        spr_data_copy(&spr, get_lsp_data(0x852, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, text, 1, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x84F, NULL));
    if ((flags & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    if (state & 0x20) {
        spr.color = 0x878787FF;
    }
    draw_sprite(spr, &pos);
    affinity = 0;
    state = gun_affinity_compare(plw, view, flags, &affinity);
    if (state & 1) {
        sprintf(text, "%d%s", affinity, get_str_tbl(0x32)[1]);
        spr_data_copy(&spr, get_lsp_data(0x853, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, text, 1, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x850, NULL));
    if ((flags & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    draw_sprite(spr, &pos);
    if (deco_slots_view_draw(0x861, view, flags, &pos) == 0) {
        draw_font_idx(0x854, (s8*)get_str_tbl(0x2E)[0], 1, &pos);
    }
    gun_part_tabs_draw(plw, view, flags, &pos);
}

/* 0x8030DCE0 (0x794): Draws a bowgun's defence, size, deviation, reload, recoil and ammo text with their icons (green
 * when a skill changes them, blue/red against the worn gun, grey when absent), its slot-bonus marks, at the 0x86B
 * anchor or at sprite `anchor`. */
void Put_equip_dtl_basis_bowgun_gan2(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, _mh_ivec2_* off) {
    _SPR_DATA_ spr;
    s8 text[8];
    _mh_ivec2_ pos;
    u32 defense;
    f32 deviation;
    u32 in;
    u32 bonus_at;
    s16 recoil;
    u8 size;
    u8 reload;
    u8 bonus;
    u8 state;
    s32 ok;
    s32 shown;
    u8** strs;

    if (anchor == 0) {
        in = *(u32*)off;
        put_lsp_anchor_offset(0x86B, &pos, &in);
    } else if (off == NULL) {
        get_lsp_data(anchor, &pos);
    } else {
        get_lsp_data(0x86B, &pos);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x9A), &pos);
    defense = 0;
    state = gun_defense_bonus_get(plw, view, flags, &defense);
    ok = state & 1;
    if (ok) {
        sprintf(text, "%d", defense);
        spr_data_copy(&spr, get_lsp_data(0x874, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, text, 0, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x86C, NULL));
    if ((flags & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    if (!ok) {
        spr.color = 0x878787FF;
    }
    draw_sprite(spr, &pos);
    size = 0;
    if (gun_size_get((u32)plw, view, flags, &size) & 1) {
        switch (size) {
        case 6:
            draw_sprite_idx(0x871, &pos);
            break;
        case 5:
            draw_sprite_idx(0x872, &pos);
            break;
        case 4:
            draw_sprite_idx(0x873, &pos);
            break;
        }
    }
    deviation = 0.0f;
    state = gun_deviation_compare(plw, view, flags, &deviation);
    ok = state & 1;
    if (ok) {
        sprintf(text, "%1.2f%s", deviation, get_str_tbl(0x32)[0]);
        spr_data_copy(&spr, get_lsp_data(0x875, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, text, 0, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x86D, NULL));
    if ((flags & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    if (!ok) {
        spr.color = 0x878787FF;
    }
    draw_sprite(spr, &pos);
    reload = 0;
    state = gun_reload_compare(plw, view, flags, &reload);
    shown = state & 1;
    if (shown) {
        strs = get_str_tbl(0x33);
        spr_data_copy(&spr, get_lsp_data(0x876, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, (s8*)strs[reload], 0, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x86E, NULL));
    if ((flags & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    if (!shown) {
        spr.color = 0x878787FF;
    }
    draw_sprite(spr, &pos);
    recoil = 0;
    state = gun_recoil_compare(plw, view, flags, &recoil);
    shown = state & 1;
    if (shown) {
        strs = get_str_tbl(0x34);
        spr_data_copy(&spr, get_lsp_data(0x877, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, (s8*)strs[recoil], 0, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x86F, NULL));
    if ((flags & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    if (!shown) {
        spr.color = 0x878787FF;
    }
    draw_sprite(spr, &pos);
    state = gun_recoil_text_get(plw, view, flags, text);
    shown = state & 1;
    if (shown) {
        spr_data_copy(&spr, get_lsp_data(0x878, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, text, 0, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x870, NULL));
    if ((flags & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    if (!shown) {
        spr.color = 0x878787FF;
    }
    draw_sprite(spr, &pos);
    if (gun_bonus_flag_ck(plw, view, flags) & 1) {
        draw_sprite_idx(0x87E, &pos);
    }
    bonus = 0;
    if (gun_slot_bonus_get(plw, view, flags, &bonus) & 1) {
        bonus_at = *(u32*)&pos;
        value_badge_draw(0x87F, bonus, (_mh_ivec2_*)&bonus_at);
    }
}

/* 0x8030E474 (0x164): Draws a bowgun frame's upgrade level (blue/red against the worn frame, the cap mark at its
 * cap) at the 0x880 anchor or at sprite `anchor`. */
void Put_equip_dtl_basis_bowgun_gan_lv(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, u8 flags, _mh_ivec2_* off) {
    _SPR_DATA_ spr;
    s8 text[8];
    _mh_ivec2_ pos;
    u32 in;
    u8 level;
    u8 state;

    if (anchor == 0) {
        in = *(u32*)off;
        put_lsp_anchor_offset(0x880, &pos, &in);
    } else if (off == NULL) {
        get_lsp_data(anchor, &pos);
    } else {
        return;
    }
    level = 0;
    state = gun_frame_level_compare(plw, view, flags, &level);
    if (state & 1) {
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x9B), &pos);
        spr_data_copy(&spr, get_lsp_data(0x882, NULL));
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", level);
        draw_font(spr, text, 1, &pos);
        if (state & 0x40) {
            draw_sprite_idx(0x887, &pos);
        }
    }
}

/* 0x8030CA68 (0x174): Draws a blade weapon's basis page at the 0x7E7 anchor: "empty" for an empty piece, else its
 * attack/element/affinity block, its sharpness and slot block and its colour panel. */
void blade_basis_page_draw(_PLW* plw, _EQUIP* equip, _EQUIP* other, u16 anchor, u8 flags) {
    _EQUIP_INDEX view;
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;

    if (equip_variant_resolve(plw, equip, other, &flags, &view) == 1) {
        return;
    }
    if (anchor != 0) {
        get_lsp_data(0x7E7, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(0x7E7, &pos, &in);
    switch (equip_wear_check(plw, &view, flags)) {
    case 1:
        return;
    case 2:
        draw_font_idx(0x8D2, (s8*)get_str_tbl(0x2E)[1], 0, &pos);
        return;
    }
    shown_skills_set(plw, &view, flags);
    Put_equip_dtl_basis_sword_ken1(plw, &view, 0, &off, flags);
    Put_equip_dtl_basis_sword_ken2(plw, &view, 0, &off, flags);
    Put_equip_dtl_basis_sword_colorX(plw, &view, 0, 0, &off, flags);
    shown_skills_clear(plw, flags);
}

/* 0x8030CBDC (0x644): Draws a blade weapon's attack, element (or status) and affinity with their icons (green when a
 * skill changes them, blue/red against the worn weapon) and its slot row, at the 0x7E7 anchor or at sprite
 * `anchor`. */
void Put_equip_dtl_basis_sword_ken1(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, _mh_ivec2_* off, u8 flags) {
    _SPR_DATA_ spr;
    s8 text[16];
    _mh_ivec2_ pos;
    u32 in;
    u16 attack;
    s16 element_value;
    s16 status_value;
    s16 affinity;
    u8 element_type;
    u8 status_type;
    u8 state;
    u8 element_state;
    u8 status_state;
    s8** strs;

    if (anchor == 0) {
        in = *(u32*)off;
        put_lsp_anchor_offset(0x7E7, &pos, &in);
    } else if (off == NULL) {
        get_lsp_data(anchor, &pos);
    } else {
        return;
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x8C), &pos);
    attack = 0;
    state = weapon_attack_compare(plw, view, flags, &attack);
    if (state & 1) {
        sprintf(text, "%d", attack);
        spr_data_copy(&spr, get_lsp_data(0x7EC, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, text, 1, &pos);
        spr_data_copy(&spr, get_lsp_data(0x7E8, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_sprite(spr, &pos);
    }
    element_state = weapon_element_compare(plw, view, flags, &element_type, &element_value);
    status_state = weapon_status_compare(plw, view, flags, &status_type, &status_value);
    if (element_state & 1) {
        strs = (s8**)get_str_tbl(0x26);
        spr_data_copy(&spr, get_lsp_data(0x7ED, NULL));
        if ((flags & 4) && (element_state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_font(spr, strs[element_type - 7], 1, &pos);
        spr_data_copy(&spr, get_lsp_data(0x7EE, NULL));
        if ((flags & 4) && (element_state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (element_state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (element_state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", element_value);
        draw_font(spr, text, 1, &pos);
    } else if (status_state & 1) {
        strs = (s8**)get_str_tbl(0x27);
        spr_data_copy(&spr, get_lsp_data(0x7ED, NULL));
        if ((flags & 4) && (status_state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_font(spr, strs[status_type - 12], 1, &pos);
        spr_data_copy(&spr, get_lsp_data(0x7EE, NULL));
        if ((flags & 4) && (status_state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (status_state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (status_state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        sprintf(text, "%d", status_value);
        draw_font(spr, text, 1, &pos);
    } else {
        draw_font_idx(0x7F1, (s8*)get_str_tbl(0x26)[5], 1, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x7E9, NULL));
    if (flags & 4) {
        if (element_state & 0x18) {
            spr.color = 0x50CD46FF;
        }
        if (status_state & 0x18) {
            spr.color = 0x50CD46FF;
        }
    }
    draw_sprite(spr, &pos);
    state = weapon_affinity_compare(plw, view, flags, &affinity);
    if (state & 1) {
        sprintf(text, "%d%s", affinity, get_str_tbl(0x32)[1]);
        spr_data_copy(&spr, get_lsp_data(0x7F2, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        if (flags & 8) {
            if (state & 2) {
                spr.color = 0x509BF0FF;
            }
            if (state & 4) {
                spr.color = 0xE63737FF;
            }
        }
        draw_font(spr, text, 1, &pos);
        spr_data_copy(&spr, get_lsp_data(0x7EA, NULL));
        if ((flags & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        draw_sprite(spr, &pos);
    }
    if (deco_slots_view_draw(0x806, view, flags, &pos) == 0) {
        draw_font_idx(0x7F3, (s8*)get_str_tbl(0x2E)[0], 1, &pos);
    }
}

/* 0x8030D220 (0x118): Draws a blade weapon's sharpness gauge (with the sharpness-skill mark) and its slot badge, at
 * the 0x807 anchor or at sprite `anchor`. */
void Put_equip_dtl_basis_sword_ken2(_PLW* plw, _EQUIP_INDEX* view, u16 anchor, _mh_ivec2_* off, u8 flags) {
    _mh_ivec2_ pos;
    u32 in;
    u32 badge_at;
    u8 slots;

    if (anchor == 0) {
        in = *(u32*)off;
        put_lsp_anchor_offset(0x807, &pos, &in);
    } else if (off == NULL) {
        get_lsp_data(anchor, &pos);
    } else {
        return;
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x8D), &pos);
    if ((flags & 2) && Pl_Skill_ck(plw, 23) == 1) {
        draw_sprite_idx(0x809, &pos);
    }
    sharpness_gauge_draw(plw, view->shown[0], &pos, flags);
    slots = 0;
    if (weapon_slot_count_get(plw, view, flags, &slots) & 1) {
        badge_at = *(u32*)&pos;
        value_badge_draw(0x80D, slots, (_mh_ivec2_*)&badge_at);
    }
}

/* 0x8030C51C (0x310): Draws the hunter's equipment list at the 0x795 anchor: the seven pieces of `columns` (the
 * weapon with its second column, then the armour), the cursor on row `cursor`, and when a dyeable piece cannot be
 * recoloured and the dye flag is set, the dye panel with colour `color`. */
void equip_list_draw(_EQUIP* columns, s8 cursor, u8 color, u16 anchor, u16 grey_mask) {
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    PlRgba rgba;
    u32 in;
    s32 dyeable;
    s32 i;
    u16* ids;
    _EQUIP* piece;
    bool grey;

    dyeable = 1;
    if (anchor != 0) {
        get_lsp_data(0x795, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(0x795, &pos, &in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x82), &pos);
    ids = (u16*)get_menu_lsp_tbl(0x87);
    for (i = 0; i < 7; i++) {
        grey = false;
        piece = NULL;
        switch (i) {
        case 0:
            piece = &columns[6];
            if (grey_mask & 0x40) {
                grey = true;
            }
            break;
        case 1:
            piece = &columns[4];
            if (grey_mask & 0x10) {
                grey = true;
            }
            break;
        case 2:
            piece = &columns[0];
            if (grey_mask & 1) {
                grey = true;
            }
            break;
        case 3:
            piece = &columns[1];
            if (grey_mask & 2) {
                grey = true;
            }
            break;
        case 4:
            piece = &columns[2];
            if (grey_mask & 4) {
                grey = true;
            }
            break;
        case 5:
            piece = &columns[3];
            if (grey_mask & 8) {
                grey = true;
            }
            break;
        case 6:
            piece = &columns[5];
            if (grey_mask & 0x20) {
                grey = true;
            }
            break;
        }
        if (piece != NULL) {
            if (i == 0) {
                Put_equip_dtl_syo_item1(piece, &columns[7], i == cursor, grey, 0, off, ids[i]);
            } else {
                Put_equip_dtl_syo_item1(piece, NULL, i == cursor, grey, 0, off, ids[i]);
            }
            if ((u32)(i - 1) <= 4 && EnableChangeColor(piece) == 0) {
                dyeable = 0;
            }
        }
    }
    if (dyeable == 0 && (userdata_flag_ck(0x7D4) != 0 || userdata_flag_ck(0x36B1) != 0)) {
        u32 dye_in;

        dye_in = *(u32*)&off;
        put_lsp_anchor_offset(0x7B7, &pos, &dye_in);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x83), &pos);
        if (color != 0) {
            rgba.word = equip_color_rgba_get(color);
            spr_data_copy(&spr, get_lsp_data(0x7B9, NULL));
            spr.color = (rgba.channel[0] << 24) | (rgba.channel[1] << 16) | (rgba.channel[2] << 8) | 0xFF;
            draw_sprite(spr, &pos);
        }
    }
}

/* 0x8030C82C (0x224): Draws one row of the equipment list at sprite `id`: its frame (selected or not), the weapon or
 * piece icon, its name (grey when `grey`), its dye colour (the colour table's `color`, or the piece's own colour
 * when it can be recoloured) and the cursor. */
void Put_equip_dtl_syo_item1(_EQUIP* piece, _EQUIP* sub, bool selected, bool grey, u8 color, _mh_ivec2_ off,
                             u16 id) {
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    PlRgba rgba;
    PlRgba custom;
    u32 in;
    u16* frame;
    u8 type;

    in = *(u32*)&off;
    put_lsp_anchor_offset(id, &pos, &in);
    if (selected == 0) {
        frame = (u16*)get_menu_lsp_tbl(0x84);
    } else {
        frame = (u16*)get_menu_lsp_tbl(0x85);
    }
    draw_sprite_ary(frame, &pos);
    if (sub != NULL) {
        type = Get_pl_type(piece, sub);
        draw_weaponicon_idx(0x5FA, type, get_rare_color(Get_equip_rare(piece)), &pos);
    } else {
        draw_equipicon_idx(0x7C5, piece, &pos);
    }
    spr_data_copy(&spr, get_lsp_data(0x7BF, NULL));
    if (grey == 1) {
        spr.color = 0x878787FF;
    }
    draw_font(spr, (s8*)GetEquipName(piece->kind, piece->item_id), 0, &pos);
    if (color != 0) {
        draw_sprite_idx(0x7CB, &pos);
        rgba.word = equip_color_rgba_get(color);
        spr_data_copy(&spr, get_lsp_data(0x7CA, NULL));
        spr.color = (rgba.channel[0] << 24) | (rgba.channel[1] << 16) | (rgba.channel[2] << 8) | 0xFF;
        draw_sprite(spr, &pos);
    } else if (sub == NULL && EnableChangeColor(piece) == 1) {
        draw_sprite_idx(0x7CD, &pos);
        custom.word = equip_custom_color_rgba_get(piece);
        spr_data_copy(&spr, get_lsp_data(0x7CC, NULL));
        spr.color = (custom.channel[0] << 24) | (custom.channel[1] << 16) | (custom.channel[2] << 8) | 0xFF;
        draw_sprite(spr, &pos);
    }
    if (selected == 1) {
        put_menu_cursor((u16*)get_menu_lsp_tbl(0x86), 0, &pos);
    }
}

/* 0x8030BAF4 (0x2A4): Steps the equipment detail screen by the pad: B closes it (returns 2), up/down picks the
 * piece (keeping the page across empty pieces and the bowgun's longer page list), left/right turns the page. */
s32 equip_detail_screen_step(StatusScreenWork* self) {
    _PLW* plw = self->equip;
    s32 ret = 0;
    u16 input = self->pad_trig | (self->pad_repeat & 0xF);
    _EQUIP* piece;
    s8 prev_count;

    self->page_input = 0;
    if (input & 0x20) {
        ret = 2;
        sysSE_req(1);
    } else {
        if (input & 3) {
            self->piece_cursor = menu_cursor_step(self->piece_cursor, 7, input, 1, 2);
        }
        prev_count = self->page_count;
        switch (self->piece_cursor) {
        case 0:
            piece = &plw->equipB;
            if (piece->kind == 0xB) {
                self->page_count = 7;
            } else {
                self->page_count = 3;
            }
            break;
        case 1:
            piece = &plw->equipA[4];
            self->page_count = 3;
            break;
        case 2:
            piece = &plw->equipA[0];
            self->page_count = 3;
            break;
        case 3:
            piece = &plw->equipA[1];
            self->page_count = 3;
            break;
        case 4:
            piece = &plw->equipA[2];
            self->page_count = 3;
            break;
        case 5:
            piece = &plw->equipA[3];
            self->page_count = 3;
            break;
        case 6:
            piece = &plw->equipA[5];
            self->page_count = 3;
            break;
        }
        if (piece->item_id != 0 && piece->kind != 0 && prev_count == 1 && prev_count != self->page_count) {
            self->page = self->saved_page;
            prev_count = self->saved_page_count;
        }
        if (prev_count != self->page_count) {
            switch (self->page_count) {
            case 3:
                if (self->page > 3) {
                    self->page = 2;
                } else if (self->page > 0) {
                    self->page--;
                }
                break;
            case 7:
                if (self->page > 0) {
                    self->page++;
                }
                break;
            }
        }
        if (piece->item_id == 0 || piece->kind == 0) {
            if (prev_count != 1) {
                self->saved_page = self->page;
                self->saved_page_count = self->page_count;
            }
            self->page_count = 1;
        }
        if (input & 0xC) {
            self->page = menu_cursor_step(self->page, self->page_count, input, 4, 8);
            self->page_input = input;
        }
    }
    return ret;
}

/* 0x8030BD98 (0x19C): Draws the equipment detail screen: the hunter's equipment list with the cursor and dye, the
 * frame, and the selected piece's detail page with its page arrows ("empty" for an empty slot). */
void equip_detail_screen_draw(StatusScreenWork* self) {
    _EQUIP columns[9];
    _mh_ivec2_ pos;
    _PLW* plw = self->equip;
    _EQUIP* piece;

    Set_equip_column_arrangement(plw, NULL, columns);
    equip_list_draw(columns, self->piece_cursor, plw->field_0x25C[6], 0, 0);
    get_lsp_data(0x795, &pos);
    draw_sprite_idx(0x7A4, &pos);
    get_lsp_data(0x7D5, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x88), &pos);
    switch (self->piece_cursor) {
    case 0:
        piece = &plw->equipB;
        break;
    case 1:
        piece = &plw->equipA[4];
        break;
    case 2:
        piece = &plw->equipA[0];
        break;
    case 3:
        piece = &plw->equipA[1];
        break;
    case 4:
        piece = &plw->equipA[2];
        break;
    case 5:
        piece = &plw->equipA[3];
        break;
    case 6:
        piece = &plw->equipA[5];
        break;
    }
    if (piece->item_id != 0 && piece->kind != 0) {
        equip_detail_page_draw(plw, piece, self->page);
        get_lsp_data(0x7D4, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x8A), &pos);
        PutPageArrow((u16*)get_menu_lsp_tbl(0), self->page, self->page_count, self->page_input, &pos, 0);
        return;
    }
    draw_font_idx(0x7D6, (s8*)get_str_tbl(0x2D)[0], 5, &pos);
}

/* 0x8030B5E8 (0x1A8): Draws one skill row at sprite `anchor` (offset by `off` when given): its frame and, for a skill,
 * its name (in `color` when given) filed as a help record. */
void skill_name_row_draw(u16 anchor, u16 skill, u32 color, const _mh_ivec2_* off) {
    PlaceRec rec;
    _mh_ivec2_ at;
    u32 in;
    s16 x;
    s16 y;
    s16 width;
    s16 height;
    s16 extra;

    if (off != NULL) {
        in = *(const u32*)off;
        put_lsp_anchor_offset(anchor, &at, &in);
    } else {
        get_lsp_data(anchor, &at);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x81), &at);
    if (skill != 0xFFFF) {
        _SPR_DATA_ spr = *get_lsp_data(0x689, NULL);
        if (color != 0) {
            spr.color = color;
        }
        draw_font(spr, (s8*)skill_name_get(skill), 5, &at);
        place_rec_skill_set(&rec, skill);
        spr_data_copy(&spr, get_lsp_data(0x68A, NULL));
        x = spr.pos.x + at.x;
        y = spr.pos.y + at.y;
        width = spr.width;
        height = spr.height;
        spr_data_copy(&spr, get_lsp_data(0x68B, NULL));
        extra = spr.width;
        extra = extra < 0 ? -extra : extra;
        width += extra;
        place_rec_alloc(&rec, x, y, width, height, 4, 9);
    }
}

/* 0x8030B868 (0x1FC): Draws page `page` of the hunter's active skills (six rows from `pos`), or "no skills". */
void Put_status_invork_skill_sep(_PLW* plw, _mh_ivec2_ pos, s8 page) {
    _mh_ivec2_ at;
    s32 count;
    s32 i;
    s32 index;
    u16* rows;
    _SPR_DATA_* box;

    count = 0;
    for (i = 0; i < 8; i++) {
        if (plw->active_skill_id[i] != 0 && plw->active_skill_tree[i] != 0) {
            count++;
        }
    }
    rows = (u16*)get_menu_lsp_tbl(0x189);
    if (count == 0) {
        box = get_lsp_data(rows[2], NULL);
        at.x = pos.x + box->pos.x;
        at.y = pos.y + box->pos.y;
        draw_font_idx(0x1044, (s8*)get_str_tbl(0x28)[0], 5, &at);
        return;
    }
    for (i = 0; i < 6; i++) {
        box = get_lsp_data(rows[i], NULL);
        at.x = pos.x + box->pos.x;
        at.y = pos.y + box->pos.y;
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x188), &at);
        index = page * 6 + i;
        if (index < 8 && plw->active_skill_id[index] != 0 && plw->active_skill_tree[index] != 0) {
            draw_font_idx(0x1044, (s8*)str_tbl_skill_get(plw->active_skill_id[index]), 5, &at);
        }
    }
}

/* 0x8030B234 (0x3B4): Draws one skill tree's row of the skill table at sprite `anchor`: the tree name (filed as a help
 * record, "none" for tree 0), then per column (weapon, head, torso, arms, waist, legs, ... and the total) the points
 * its pieces add, "torso up" marks for tree 1; returns 1 when a torso-up column was drawn. */
u32 skill_table_row_draw(_PLW* plw, _EQUIP* columns, u16 anchor, u8 tree, u8 mode, const _mh_ivec2_* pos) {
    _SPR_DATA_ spr;
    PlaceRec rec;
    s8 points[8];
    s8 text[8];
    _mh_ivec2_ at;
    u32 in;
    u8 has;
    u32 torso;
    s32 i;

    has = 0;
    torso = 0;
    in = *(const u32*)pos;
    put_lsp_anchor_offset(anchor, &at, &in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x7D), &at);
    spr_data_copy(&spr, get_lsp_data(0x64A, NULL));
    if (tree == 0) {
        draw_font(spr, (s8*)get_str_tbl(0x28)[0], 0, &at);
    } else {
        points[0] = equip_skill_points_get(&columns[6], tree);
        points[0] += equip_skill_points_get(&columns[7], tree);
        points[0] += equip_skill_points_get(&columns[8], tree);
        points[1] = equip_skill_points_get(&columns[4], tree);
        points[2] = equip_columns_torso_points_get(columns, tree);
        points[3] = equip_skill_points_get(&columns[1], tree);
        points[4] = equip_skill_points_get(&columns[2], tree);
        points[5] = equip_skill_points_get(&columns[3], tree);
        points[6] = equip_skill_points_get(&columns[5], tree);
        points[7] = equip_columns_tree_points_get(columns, tree);
        if (equip_skill_tree_has_ck(&columns[6], tree) != 0 || equip_skill_tree_has_ck(&columns[7], tree) != 0 ||
            equip_skill_tree_has_ck(&columns[8], tree) != 0) {
            has |= 1;
        }
        if (equip_skill_tree_has_ck(&columns[4], tree) != 0) {
            has |= 2;
        }
        if (equip_skill_tree_has_ck(&columns[0], tree) != 0) {
            has |= 4;
        }
        if (equip_skill_tree_has_ck(&columns[1], tree) != 0) {
            has |= 8;
        }
        if (equip_skill_tree_has_ck(&columns[2], tree) != 0) {
            has |= 0x10;
        }
        if (equip_skill_tree_has_ck(&columns[3], tree) != 0) {
            has |= 0x20;
        }
        if (equip_skill_tree_has_ck(&columns[5], tree) != 0) {
            has |= 0x40;
        }
        if (equip_columns_tree_has_ck(columns, tree) != 0) {
            has |= 0x80;
        }
        draw_font(spr, (s8*)item_kind_name_get(tree), 0, &at);
        menu_row_rec_init(&rec, tree);
        place_rec_alloc_idx(&rec, 0x654, &at, 0, 9);
        for (i = 0; i < 8; i++) {
            if (has & (1 << i)) {
                if (tree == 1 && i < 7) {
                    draw_sprite_idx(((u16*)get_menu_lsp_tbl(0x7A))[i], &at);
                    torso = 1;
                }
                if (torso == 1) {
                    sprintf(text, "%s", get_str_tbl(0x28)[1]);
                } else if (points[i] != 0) {
                    sprintf(text, "%+d", points[i]);
                } else {
                    sprintf(text, "%d", points[i]);
                }
                if (i != 7 || torso != 1) {
                    spr_data_copy(&spr, get_lsp_data(((u16*)get_menu_lsp_tbl(0x7E))[i], NULL));
                    draw_font(spr, text, i < 7 ? 2 : 6, &at);
                }
            }
        }
    }
    return torso;
}

/* 0x8030A1DC (0x130): The number of ten-row skill-table pages the given pieces (or the worn set, or just a frame)
 * fill, never below one; 0 when they do not resolve. */
s32 skill_table_page_count(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock) {
    _EQUIP columns[9];
    u8 trees[0x55];
    s8 points[0x55];
    _EQUIP_INDEX set_view;
    _EQUIP_INDEX piece_view;
    u16 active_ids[8];
    u8 active_trees[8];
    s8 active_levels[8];
    u8 flags;
    s8 pages;

    if (frame != NULL) {
        if (barrel != NULL && stock != NULL) {
            if (equip_variant_resolve_ex(plw, frame, barrel, stock, 1, &set_view) == 1) {
                return 0;
            }
            Set_equip_column_arrangement(plw, &set_view, columns);
            equip_columns_gun_view_set(plw, &set_view, columns);
        } else {
            flags = 1;
            if (equip_variant_resolve(plw, frame, NULL, &flags, &piece_view) == 1) {
                return 0;
            }
            Set_equip_column_arrangement(plw, &piece_view, columns);
            equip_columns_gun_view_set(plw, &piece_view, columns);
        }
    } else {
        Set_equip_column_arrangement(plw, NULL, columns);
    }
    pages = (skill_table_trees_list(plw, columns, trees, points, active_trees, active_ids, active_levels) + 9) / 10;
    if (pages == 0) {
        pages = 1;
    }
    return pages;
}

/* 0x8030A694 (0x320): Lists the skill trees a column array of equipment adds points to (`trees`/`points`, sorted by
 * points then tree, the activated ones left out; tree 1 counts one point) and the activated skills; returns how many
 * trees have points. */
s32 skill_table_trees_list(_PLW* plw, _EQUIP* columns, u8* trees, s8* points, u8* active_trees, u16* active_ids,
                s8* active_levels) {
    s32 total;
    s32 count;
    s32 i;
    s32 j;
    s32 tree;
    s32 found;
    u32 has;
    s8 value;
    u16 skill_ids[8];
    s8 skill_levels[8];
    u8 skill_trees[8];

    count = 0;
    total = 0;
    for (i = 0; i < 8; i++) {
        skill_ids[i] = 0;
        skill_levels[i] = 0;
        skill_trees[i] = 0;
    }
    equip_columns_skills_get(columns, skill_ids, skill_levels, skill_trees);
    for (i = 0; i < 8; i++) {
        active_ids[i] = skill_ids[i];
        active_trees[i] = skill_trees[i];
        active_levels[i] = skill_levels[i];
    }
    for (tree = 0; tree < 0x55; tree++) {
        trees[tree] = 0;
        points[tree] = 0;
        if (tree != 0) {
            has = equip_columns_tree_has_ck(columns, tree);
            value = equip_columns_tree_points_get(columns, tree);
            if (value != 0 || has != 0) {
                if (tree == 1) {
                    value = 1;
                }
                total++;
                found = 0;
                for (i = 0; i < 8; i++) {
                    if (skill_trees[i] == (u8)tree) {
                        found = 1;
                    }
                }
                if (found == 0) {
                    trees[count] = tree;
                    points[count] = value;
                    count++;
                }
            }
        }
    }
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (points[i] < points[j] || (points[i] == points[j] && trees[i] < trees[j])) {
                s8 point_here = points[i];
                u8 tree_here = trees[i];

                points[i] = points[j];
                trees[i] = trees[j];
                points[j] = point_here;
                trees[j] = tree_here;
            }
        }
    }
    return total;
}

/* 0x8030A5BC (0xD8): Draws the skill table's page arrows at the 0x52A anchor, moved by sprite `anchor`'s offset from
 * the table's 0x609 anchor. */
void equip_detail_page_refresh(_PLW* plw, u16 anchor, s16 page, s16 count, u16 input) {
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;

    if (anchor != 0) {
        get_lsp_data(0x609, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    in = *(u32*)&off;
    put_lsp_anchor_offset(0x52A, &pos, &in);
    PutPageArrow((u16*)get_menu_lsp_tbl(0), page, count, input, &pos, 0);
}

/* 0x8030A05C (0x174): Draws a whole bowgun's three parts at sprite `id`: per part its icon tinted with its rarity
 * colour and its name. */
void info_gun_parts_draw(u16 id, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock) {
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    u32 in;
    s32 i;
    u16* rows;
    u32 color;
    u8 icon;
    _EQUIP* piece;

    if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
        get_lsp_data(id, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x72), &pos);
        rows = (u16*)get_menu_lsp_tbl(0x73);
        for (i = 0; i < 3; i++) {
            in = *(u32*)&pos;
            put_lsp_anchor_offset(rows[i], &at, &in);
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0x70), &at);
            icon = 0xFF;
            piece = NULL;
            switch (i) {
            case 0:
                icon = 0x60;
                piece = frame;
                color = get_rare_color(Get_equip_rare(frame));
                break;
            case 1:
                icon = 0x61;
                piece = barrel;
                color = get_rare_color(Get_equip_rare(barrel));
                break;
            case 2:
                icon = 0x62;
                piece = stock;
                color = get_rare_color(Get_equip_rare(stock));
                break;
            }
            draw_tex_icon_idx(0x5FA, icon, color, &at);
            draw_font_idx(0x5F4, (s8*)GetEquipName(piece->kind, piece->item_id), 0, &at);
        }
    }
}

/* 0x8030A328 (0x294): Draws page `page` of the skill table for the given pieces (or the worn set) at the 0x609
 * anchor, moved by sprite `anchor`, and marks the column of the shown piece's kind. */
void skill_table_page_draw(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, u16 anchor, s8 page) {
    u8 trees[0x55];
    s8 points[0x55];
    _EQUIP columns[9];
    _EQUIP_INDEX view;
    u16 active_ids[8];
    u8 active_trees[8];
    s8 active_levels[8];
    _mh_ivec2_ pos;
    _mh_ivec2_ mark;
    _mh_ivec2_ off;
    u32 table_at;
    u32 in;
    u8 flags;
    s32 column;

    if (anchor != 0) {
        get_lsp_data(0x609, &pos);
        get_lsp_data(anchor, &off);
        off.x -= pos.x;
        off.y -= pos.y;
    } else {
        off.x = 0;
        off.y = 0;
    }
    if (frame != NULL) {
        flags = 1;
        if (barrel != NULL && stock != NULL) {
            if (equip_variant_resolve_ex(plw, frame, barrel, stock, 1, &view) == 1) {
                return;
            }
            Set_equip_column_arrangement(plw, &view, columns);
        } else {
            if (equip_variant_resolve(plw, frame, NULL, &flags, &view) == 1) {
                return;
            }
            Set_equip_column_arrangement(plw, &view, columns);
            equip_columns_gun_set(plw, frame, barrel, stock, columns);
        }
    } else {
        Set_equip_column_arrangement(plw, NULL, columns);
    }
    skill_table_trees_list(plw, columns, trees, points, active_trees, active_ids, active_levels);
    table_at = *(u32*)&off;
    skill_table_body_draw(plw, columns, trees, points, active_trees, active_ids, active_levels, page,
                (const _mh_ivec2_*)&table_at);
    if (frame != NULL) {
        column = -1;
        switch (frame->kind) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            column = 0;
            break;
        case 1:
            column = 2;
            break;
        case 2:
            column = 3;
            break;
        case 3:
            column = 4;
            break;
        case 4:
            column = 5;
            break;
        case 5:
            column = 1;
            break;
        case 6:
            column = 6;
            break;
        default:
            nw4r::db::Panic("menu_infomation.cpp", 1455, "NW4R:Failed assertion 0");
            break;
        }
        if (column >= 0) {
            in = *(u32*)&off;
            put_lsp_anchor_offset(0x619, &pos, &in);
            get_lsp_data(((u16*)get_menu_lsp_tbl(0x7C))[column], &mark);
            mark.x += pos.x;
            mark.y += pos.y;
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0x79), &mark);
        }
    }
}

/* 0x803096D0 (0x3C0): Draws the status block of the hunter's sheet (with the given column array in the arena modes):
 * health, stamina, affinity, displayed attack, the element or status with its icon, and defence, at the 0x57E
 * anchor, at sprite `anchor`, or at `at`. */
void Put_status_equip_status_block(_PLW* plw, _EQUIP* columns, u16 anchor, _mh_ivec2_* at) {
    PlStatusSheet sheet;
    s8 text[32];
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;

    if (anchor != 0) {
        if (anchor != 0x57E) {
            get_lsp_data(0x57E, &pos);
            get_lsp_data(anchor, &off);
            off.x -= pos.x;
            off.y -= pos.y;
        } else {
            off.x = 0;
            off.y = 0;
        }
        in = *(u32*)&off;
        put_lsp_anchor_offset(0x57E, &pos, &in);
    } else if (at != NULL) {
        pos.x = at->x;
        pos.y = at->y;
    } else {
        get_lsp_data(0x57E, &pos);
    }
    if ((u32)(GameMode_ck() - 2) <= 1) {
        pl_status_sheet_get(plw, columns, &sheet);
    } else {
        pl_status_sheet_get(plw, NULL, &sheet);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x68), &pos);
    sprintf(text, "%d", sheet.health);
    draw_font_idx(0x587, text, 6, &pos);
    sprintf(text, "%d", sheet.stamina / 6);
    draw_font_idx(0x588, text, 6, &pos);
    sprintf(text, "%d", sheet.affinity);
    draw_font_idx(0x589, text, 6, &pos);
    sheet.attack = weapon_attack_display(columns[6].kind, sheet.attack);
    sprintf(text, "%d", sheet.attack);
    draw_font_idx(0x58A, text, 6, &pos);
    if (sheet.element_type != 0) {
        if (status_element_icon[sheet.element_type - 1] != 0xFF) {
            if (arena_idx_get() == 0) {
                draw_tex_icon_square(pos.x + 0x3E, pos.y + 0x5C, 0x14, 0xFFFFFFFF,
                                     status_element_icon[sheet.element_type - 1]);
            } else {
                draw_tex_icon_square_arena(pos.x + 0x3E, pos.y + 0x5C, 0x14, 0xFFFFFFFF,
                                           status_element_icon[sheet.element_type - 1]);
            }
        }
        sprintf(text, "%d", sheet.element_value * 10);
        draw_font_idx(0x58B, text, 6, &pos);
    } else if (sheet.status_type != 0) {
        if (status_ailment_icon[sheet.status_type - 12] != 0xFF) {
            if (arena_idx_get() == 0) {
                draw_tex_icon_square(pos.x + 0x3E, pos.y + 0x5C, 0x14, 0xFFFFFFFF,
                                     status_ailment_icon[sheet.status_type - 12]);
            } else {
                draw_tex_icon_square_arena(pos.x + 0x3E, pos.y + 0x5C, 0x14, 0xFFFFFFFF,
                                           status_ailment_icon[sheet.status_type - 12]);
            }
        }
        sprintf(text, "%d", sheet.status_value * 10);
        draw_font_idx(0x58B, text, 6, &pos);
    } else {
        draw_font_idx(0x58D, (s8*)get_str_tbl(0x26)[5], 5, &pos);
    }
    sprintf(text, "%d", sheet.defense);
    draw_font_idx(0x58E, text, 6, &pos);
}

/* 0x80309A90 (0x1F0): Draws the resistances block of the hunter's sheet (with the given column array in the arena
 * modes) at the 0x57E anchor, at sprite `anchor`, or at `at`. */
void Put_status_equip_element_block(_PLW* plw, _EQUIP* columns, u16 anchor, _mh_ivec2_* at) {
    PlStatusSheet sheet;
    s8 text[32];
    _mh_ivec2_ pos;
    _mh_ivec2_ off;
    u32 in;

    if (anchor != 0) {
        if (anchor != 0x57E) {
            get_lsp_data(0x57E, &pos);
            get_lsp_data(anchor, &off);
            off.x -= pos.x;
            off.y -= pos.y;
        } else {
            off.x = 0;
            off.y = 0;
        }
        in = *(u32*)&off;
        put_lsp_anchor_offset(0x57E, &pos, &in);
    } else if (at != NULL) {
        pos.x = at->x;
        pos.y = at->y;
    } else {
        get_lsp_data(0x57E, &pos);
    }
    if ((u32)(GameMode_ck() - 2) <= 1) {
        pl_status_sheet_get(plw, columns, &sheet);
    } else {
        pl_status_sheet_get(plw, NULL, &sheet);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x69), &pos);
    sprintf(text, "%d", sheet.resist[0]);
    draw_font_idx(0x5A3, text, 6, &pos);
    sprintf(text, "%d", sheet.resist[1]);
    draw_font_idx(0x5A4, text, 6, &pos);
    sprintf(text, "%d", sheet.resist[3]);
    draw_font_idx(0x5A5, text, 6, &pos);
    sprintf(text, "%d", sheet.resist[2]);
    draw_font_idx(0x5A6, text, 6, &pos);
    sprintf(text, "%d", sheet.resist[4]);
    draw_font_idx(0x5A7, text, 6, &pos);
}

/* 0x80309C80 (0x3DC): Draws the information screen's equipment page: a bowgun's three parts, then the seven worn pieces
 * (icon, name, dye colour) and, when a dyeable piece cannot be recoloured and the dye flag is set, the dye panel. */
void put_equip_info_piece_page(StatusScreenWork* self) {
    _SPR_DATA_ custom_spr;
    _SPR_DATA_ dye_spr;
    _EQUIP* pieces[7];
    _EQUIP* gun[3];
    _mh_ivec2_ pos;
    _mh_ivec2_ at;
    PlRgba custom;
    PlRgba dye;
    _PLW* plw = self->equip;
    s32 dyeable;
    u16* rows;
    s32 i;
    u32 color;
    u8 type;

    get_lsp_data(0x5CA, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x6D), &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x6E), &pos);
    pieces[0] = &plw->equipB;
    pieces[1] = &plw->equipA[4];
    pieces[2] = &plw->equipA[0];
    pieces[3] = &plw->equipA[1];
    pieces[4] = &plw->equipA[2];
    pieces[5] = &plw->equipA[3];
    pieces[6] = &plw->equipA[5];
    if (pieces[0]->kind == 0xB) {
        gun[0] = pieces[0];
        gun[1] = &plw->equipC;
        gun[2] = &plw->equipD;
        get_lsp_data(0x5B2, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x72), &pos);
        rows = (u16*)get_menu_lsp_tbl(0x73);
        for (i = 0; i < 3; i++) {
            get_lsp_data(rows[i], &at);
            at.x += pos.x;
            at.y += pos.y;
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0x70), &at);
            color = get_rare_color(Get_equip_rare(gun[i]));
            switch (gun[i]->kind) {
            case 11:
                draw_tex_icon_idx(0x5FA, 0x60, color, &at);
                break;
            case 12:
                draw_tex_icon_idx(0x5FA, 0x61, color, &at);
                break;
            case 13:
                draw_tex_icon_idx(0x5FA, 0x62, color, &at);
                break;
            }
            draw_font_idx(0x5F4, (s8*)GetEquipName(gun[i]->kind, gun[i]->item_id), 0, &at);
        }
    }
    dyeable = 1;
    rows = (u16*)get_menu_lsp_tbl(0x71);
    for (i = 0; i < 7; i++) {
        get_lsp_data(rows[i], &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x70), &pos);
        if (i == 0) {
            if (pieces[0]->kind == 0) {
                draw_tex_icon_idx(0x5FA, 0x50, 0xDFDFDFFF, &pos);
            } else {
                type = Get_pl_type(&plw->equipB, &plw->equipC);
                draw_weaponicon_idx(0x5FA, type, get_rare_color(Get_equip_rare(&plw->equipB)), &pos);
            }
        } else {
            draw_equipicon_idx(0x7C5, pieces[i], &pos);
        }
        draw_font_idx(0x5F4, (s8*)GetEquipName(pieces[i]->kind, pieces[i]->item_id), 0, &pos);
        if ((u32)(i - 1) <= 4) {
            if (EnableChangeColor(pieces[i]) == 1) {
                draw_sprite_idx(0x5FC, &pos);
                custom.word = equip_custom_color_rgba_get(pieces[i]);
                spr_data_copy(&custom_spr, get_lsp_data(0x5FB, NULL));
                custom_spr.color =
                    (custom.channel[0] << 24) | (custom.channel[1] << 16) | (custom.channel[2] << 8) | 0xFF;
                draw_sprite(custom_spr, &pos);
            } else {
                dyeable = 0;
            }
        }
    }
    if (dyeable == 0 && (userdata_flag_ck(0x7D4) != 0 || userdata_flag_ck(0x36B1) != 0)) {
        get_lsp_data(0x5EC, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x6F), &pos);
        if (plw->field_0x25C[6] != 0) {
            dye.word = equip_color_rgba_get(plw->field_0x25C[6]);
            spr_data_copy(&dye_spr, get_lsp_data(0x5EE, NULL));
            dye_spr.color = (dye.channel[0] << 24) | (dye.channel[1] << 16) | (dye.channel[2] << 8) | 0xFF;
            draw_sprite(dye_spr, &pos);
        }
    }
}

/* 0x80309098 (0x638): Draws the information screen's hunter page: online, the hunter rank with its points and the
 * points to the next rank; offline, the resource points; then the name, the weapon class, the money, the status and
 * resistance blocks of the worn set, the four bonus skills (grey when the class cannot use them, each filed as a
 * help record) and the page arrows. */
void put_equip_info_upper_page(StatusScreenWork* self) {
    _EQUIP columns[9];
    _SPR_DATA_ spr;
    s8 text[32];
    PlaceRec rec;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _mh_ivec2_ cell;
    s32 rank_points;
    s32 next_points;
    u32 in;
    u16 rank;
    _PLW* plw = self->equip;
    u32 online;
    u32 shown;
    s32 to_next;
    s32 i;
    s32 count;
    u16* rows;
    u8 type;
    u8** classes;
    s16 x;
    s16 y;
    s16 width;
    s16 height;
    s16 extra;

    get_lsp_data(0x544, &base);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x62), &base);
    online = (game_ready_ck() == 1);
    if (online == 1) {
        get_lsp_data(0x557, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x64), &pos);
        draw_font_idx(0x563, (s8*)&plw->user_profile_0x5CA[0x11], 4, &pos);
        rank = 0;
        rank_points = 0;
        next_points = 0;
        userdata_hunter_rank_info_get(&rank, &rank_points, &next_points);
        shown = 1;
        if (next_points == 0) {
            if (rank == 1) {
                to_next = 0;
                shown = 0;
            } else {
                to_next = 0;
            }
        } else {
            to_next = next_points - rank_points;
            if (to_next < 0) {
                to_next = 0;
            }
        }
        if (shown == 1) {
            draw_sprite_idx(0x55C, &pos);
            if (rank < 100) {
                draw_number_2digit_ary((u16*)get_menu_lsp_tbl(0x65), rank, 0, &pos);
            } else {
                draw_number_idx(0x569, rank / 100, 0, &pos);
                draw_number_idx(0x568, rank / 10 % 10, 0, &pos);
                draw_number_idx(0x567, rank % 10, 0, &pos);
            }
            draw_font_idx(0x55D, (s8*)get_str_tbl(0x28)[4], 4, &pos);
            sprintf(text, "%d", rank_points);
            draw_font_idx(0x56A, text, 6, &pos);
            draw_font_idx(0x55E, (s8*)get_str_tbl(0x28)[5], 4, &pos);
            if (to_next == 0) {
                sprintf(text, "%s", get_str_tbl(0x28)[3]);
            } else {
                sprintf(text, "%d", to_next);
            }
            draw_font_idx(0x56B, text, 6, &pos);
        }
    } else {
        get_lsp_data(0x557, &pos);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x66), &pos);
        sprintf(text, "%d%s", ((Q_UserData*)lobby_world_block)->points_0x3F04, get_str_tbl(0x28)[2]);
        draw_font_idx(0x56C, text, 6, &pos);
    }
    sprintf(text, "%s", plw->user_profile_0x5CA);
    spr_data_copy(&spr, get_lsp_data(0x560, NULL));
    spr.pos.x += 6;
    draw_font_align(&spr, text, 4, &pos);
    type = Get_pl_type(&plw->equipB, &plw->equipC);
    classes = get_str_tbl(0x2C);
    spr_data_copy(&spr, get_lsp_data(0x561, NULL));
    spr.pos.x += 6;
    draw_font(spr, (s8*)classes[type], 4, &pos);
    sprintf(text, "%dz", ((Q_UserData*)lobby_world_block)->zenny_0x18);
    draw_font_idx(0x562, text, 6, &pos);
    Set_equip_column_arrangement(plw, NULL, columns);
    Put_status_equip_status_block(plw, columns, 0x57E, NULL);
    Put_status_equip_element_block(plw, columns, 0x59D, NULL);
    get_lsp_data(0x576, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x67), &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x63), &base);
    get_lsp_data(0x530, &pos);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x6B), &pos);
    ainpc_page_mark_a_draw(0x543, NULL);
    count = 0;
    rows = (u16*)get_menu_lsp_tbl(0x6C);
    for (i = 0; i < 4; i++) {
        if (plw->deco_skill_id[i] != 0) {
            in = *(u32*)&pos;
            put_lsp_anchor_offset(*rows, &cell, &in);
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0x6A), &cell);
            spr_data_copy(&spr, get_lsp_data(0x52C, NULL));
            if (pl_bonus_skill_valid_ck(plw, plw->deco_skill_id[i]) == 0) {
                spr.color = 0x878787FF;
            }
            draw_font(spr, (s8*)str_tbl_course_get(plw->deco_skill_id[i]), 5, &cell);
            place_rec_course_set(&rec, plw->deco_skill_id[i]);
            spr_data_copy(&spr, get_lsp_data(0x52D, NULL));
            x = spr.pos.x + cell.x;
            y = spr.pos.y + cell.y;
            width = spr.width;
            height = spr.height;
            spr_data_copy(&spr, get_lsp_data(0x52E, NULL));
            extra = spr.width;
            width += extra < 0 ? -extra : extra;
            spr_data_copy(&spr, get_lsp_data(0x52F, NULL));
            extra = spr.width;
            width += extra < 0 ? -extra : extra;
            place_rec_alloc(&rec, x, y, width, height, 4, 11);
            rows++;
            count++;
        }
    }
    if (count == 0) {
        draw_font_idx(0x531, (s8*)get_str_tbl(0x28)[0], 1, &pos);
    }
    get_lsp_data(0x52A, &pos);
    PutPageArrow((u16*)get_menu_lsp_tbl(0), self->field_0x1AE, self->field_0x1AF, self->pad_repeat, &pos, 0);
}

/* 0x8030A9B4 (0x880): Draws page `page` of the skill table at `pos`: the activated skills' trees (first page only),
 * then the other trees with points, the torso-up marks, the weapon (or bowgun) icon heading the weapon column and,
 * on the first page, the activated skills with their animated markers (red for a negative skill, grey for one the
 * class cannot use). */
void skill_table_body_draw(_PLW* plw, _EQUIP* columns, u8* trees, s8* points, u8* active_trees, u16* active_ids,
                 s8* active_levels, s8 page, const _mh_ivec2_* pos) {
    _SPR_DATA_ spr;
    _mh_ivec2_ table;
    _mh_ivec2_ header;
    _mh_ivec2_ mark;
    u32 table_in;
    u32 header_in;
    u32 active_in;
    u32 empty_in;
    u32 row_in;
    u32 none_in;
    u32 torso_in;
    u32 torso_mark_in;
    u32 frame_in;
    u32 weapon_in;
    u32 last_in;
    s32 shown;
    s32 drawn;
    s32 i;
    s32 start;
    s32 total;
    s32 torso;
    u16* rows;
    u8* tree;
    u32 normal_color;
    u32 bad_color;
    u32 color;
    u32 marker_color;
    u16 frame;

    shown = 0;
    table_in = *(const u32*)pos;
    put_lsp_anchor_offset(0x609, &table, &table_in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x74), &table);
    header_in = *(const u32*)pos;
    put_lsp_anchor_offset(0x619, &header, &header_in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x76), &header);
    drawn = 0;
    if (page == 0) {
        rows = (u16*)get_menu_lsp_tbl(0x197);
        for (i = 0; i < 8; i++) {
            if (active_trees[i] != 0) {
                active_in = *(const u32*)pos;
                skill_table_row_draw(plw, columns, rows[i], active_trees[i], 1, (const _mh_ivec2_*)&active_in);
                shown++;
                drawn++;
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            if (active_trees[i] != 0) {
                shown++;
            }
        }
    }
    start = page * 10 - shown;
    if (start < 0) {
        start = 0;
    }
    tree = trees + start;
    for (i = drawn; i < 10; i++) {
        rows = (u16*)get_menu_lsp_tbl(0x197);
        if (*tree == 0) {
            empty_in = *(const u32*)pos;
            put_lsp_anchor_offset(rows[i], &header, &empty_in);
            draw_sprite_ary((u16*)get_menu_lsp_tbl(0x7D), &header);
        } else {
            row_in = *(const u32*)pos;
            skill_table_row_draw(plw, columns, rows[i], *tree, 0, (const _mh_ivec2_*)&row_in);
        }
        tree++;
    }
    torso = 0;
    total = 0;
    for (i = 0; i < 8; i++) {
        if (active_trees[i] != 0) {
            total++;
        }
        if (active_trees[i] == 1) {
            torso++;
        }
    }
    for (i = 0; i < 0x55; i++) {
        if (trees[i] != 0) {
            total++;
        }
        if (trees[i] == 1) {
            torso++;
        }
    }
    if (total == 0) {
        rows = (u16*)get_menu_lsp_tbl(0x197);
        none_in = *(const u32*)pos;
        skill_table_row_draw(plw, columns, rows[0], 0, 0, (const _mh_ivec2_*)&none_in);
    }
    if (torso != 0) {
        torso_in = *(const u32*)pos;
        put_lsp_anchor_offset(0x619, &header, &torso_in);
        draw_sprite_idx(((u16*)get_menu_lsp_tbl(0x7B))[2], &header);
        rows = (u16*)get_menu_lsp_tbl(0x7C);
        torso_mark_in = *(const u32*)pos;
        put_lsp_anchor_offset(rows[2], &mark, &torso_mark_in);
        mark.x += header.x;
        mark.y += header.y;
        draw_sprite_idx(0x608, &mark);
    }
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x75), &table);
    frame_in = *(const u32*)pos;
    put_lsp_anchor_offset(0x619, &header, &frame_in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x77), &header);
    switch ((u32)columns[6].kind) {
    case 11:
        if (columns[7].kind == 0xC && columns[8].kind == 0xD) {
            if (columns[6].item_id != 0 && columns[7].item_id != 0 && columns[8].item_id != 0) {
                draw_weaponicon_idx(0x61C, Get_pl_type(&columns[6], &columns[7]), 0xDFDFDFFF, &header);
            } else if (columns[6].item_id != 0 && columns[7].item_id == 0 && columns[8].item_id == 0 &&
                       plw->equipB.kind == 0xB) {
                draw_equipicon_rare(get_lsp_data(0x61C, NULL), &columns[6], &header, 0);
            }
        } else if (columns[6].item_id != 0) {
            draw_equipicon_rare(get_lsp_data(0x61C, NULL), &columns[6], &header, 0);
        }
        break;
    case 12:
    case 13:
        draw_equipicon_rare(get_lsp_data(0x61C, NULL), &columns[6], &header, 0);
        break;
    default:
        draw_weaponicon_idx(0x61C, Get_pl_type(&columns[6], &columns[7]), 0xDFDFDFFF, &header);
        break;
    }
    weapon_in = *(const u32*)pos;
    put_lsp_anchor_offset(0x668, &header, &weapon_in);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x78), &header);
    if (page == 0 && drawn != 0) {
        rows = (u16*)get_menu_lsp_tbl(0x198);
        last_in = *(const u32*)pos;
        put_lsp_anchor_offset(rows[drawn - 1], &header, &last_in);
        draw_sprite_ary((u16*)get_menu_lsp_tbl(0x7F), &header);
    }
    if (page == 0 && shown != 0) {
        get_lsp_data(0x677, &header);
        draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(0x80), 7, &header);
        rows = (u16*)get_menu_lsp_tbl(0x199);
        spr_data_copy(&spr, get_lsp_data(rows[0], NULL));
        normal_color = spr.color;
        spr_data_copy(&spr, get_lsp_data(rows[1], NULL));
        bad_color = spr.color;
        get_lsp_data(0x65F, &header);
        for (i = 0; i < 8; i++) {
            if (active_ids[i] != 0 && active_trees[i] != 0) {
                if (equip_columns_tree_points_get(columns, active_trees[i]) < 0) {
                    color = 0xE63737FF;
                    marker_color = bad_color;
                } else {
                    color = 0;
                    marker_color = normal_color;
                }
                if (pl_equip_skill_valid_ck(plw, &columns[6], &columns[7], active_trees[i]) == 0) {
                    color = 0x878787FF;
                }
                skill_name_row_draw(((u16*)get_menu_lsp_tbl(0x19A))[i], active_ids[i], color, NULL);
                spr_data_copy(&spr, get_lsp_data(((u16*)get_menu_lsp_tbl(0x199))[i], NULL));
                spr.color = marker_color;
                frame = 0;
                if (pad_hold_ck() == 0 && system_w.field_0x05 == 0) {
                    frame = system_w.field_0x0c % 25;
                }
                spr_anim_draw(&spr, frame, &header);
            } else {
                skill_name_row_draw(((u16*)get_menu_lsp_tbl(0x19A))[i], 0xFFFF, 0, NULL);
            }
        }
    } else {
        get_lsp_data(0x677, &header);
        draw_sprite_anim_ary((u16*)get_menu_lsp_tbl(0x80), 7, &header);
        draw_font_idx(0x678, (s8*)get_str_tbl(0x28)[0], 1, &header);
    }
    ainpc_page_mark_a_draw(0x695, NULL);
    ainpc_page_mark_a_draw(0x696, NULL);
}


/* ---------------------------------------------------------------------------------------------------
 * 0x80313E24..0x8031A638 - the second half: the equipment columns, the slot blocks and the panel's
 * own cursor (13 bodies, the rest are placeholders - see the header's residual list)
 * --------------------------------------------------------------------------------------------------- */

extern u8 equip_panel_row_layout[22][3];

/* 0x80313E24 (0x4A4): Draws the equipment panel of one kind: for each of its eleven rows the row frame, every
 * column cell of each of the three shown pieces (resolved one by one), then the totals cell and the row label. */
void ammo_table_draw(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8 kind, const _mh_ivec2_* pos) {
    _EQUIP_INDEX resolved;
    u8 count;
    u8 piece_flags;
    u8 ok;
    u8 row;
    u8 piece;

    count = 0;
    for (row = 0; row < 11; row++) {
        u32 frame_pos = *(const u32*)pos;
        ammo_table_row_frame_draw(kind, row, (const _mh_ivec2_*)&frame_pos, 0, flags);
        u8* cols = equip_panel_row_layout[(u8)(row + kind * 11)];
        for (piece = 0; piece < 3; piece++) {
            piece_flags = flags & ~6;
            switch (piece) {
            case 0:
                ok = equip_variant_resolve(plw, view->shown[0], NULL, &piece_flags, &resolved);
                break;
            case 1:
                ok = equip_variant_resolve(plw, view->shown[1], NULL, &piece_flags, &resolved);
                break;
            case 2:
                ok = equip_variant_resolve(plw, view->shown[2], NULL, &piece_flags, &resolved);
                break;
            }
            if (ok == 1) {
#line 6824
                nw4r::db::Panic("menu_infomation.cpp", __LINE__, "NW4R:Failed assertion 0");
            } else {
                u8 col = cols[0];
                if (col != 0xFF) {
                    u32 better = (gun_ammo_usable_ck(plw, &resolved, col, piece_flags) & 1) != 0;
                    u8 state = gun_clip_compare(plw, &resolved, col, piece_flags, &count);
                    u32 at = *(const u32*)pos;
                    ammo_table_cell_draw(piece, row, 0, (const _mh_ivec2_*)&at, count, state, better, piece_flags);
                }
                col = cols[1];
                if (col != 0xFF) {
                    u32 better = (gun_ammo_usable_ck(plw, &resolved, col, piece_flags) & 1) != 0;
                    u8 state = gun_clip_compare(plw, &resolved, col, piece_flags, &count);
                    u32 at = *(const u32*)pos;
                    ammo_table_cell_draw(piece, row, 1, (const _mh_ivec2_*)&at, count, state, better, piece_flags);
                }
                col = cols[2];
                if (col != 0xFF) {
                    u32 better = (gun_ammo_usable_ck(plw, &resolved, col, piece_flags) & 1) != 0;
                    u8 state = gun_clip_compare(plw, &resolved, col, piece_flags, &count);
                    u32 at = *(const u32*)pos;
                    ammo_table_cell_draw(piece, row, 2, (const _mh_ivec2_*)&at, count, state, better, piece_flags);
                }
            }
        }
        u8 total_state = 0;
        u8 total = 0;
        u8 col = cols[0];
        if (col != 0xFF) {
            u32 better = (gun_ammo_usable_ck(plw, view, col, flags) & 1) != 0;
            total_state = gun_clip_compare(plw, view, col, flags, &count);
            u32 at = *(const u32*)pos;
            ammo_table_cell_draw(piece, row, 0, (const _mh_ivec2_*)&at, count, total_state, better, flags);
            total = count;
        }
        col = cols[1];
        if (col != 0xFF) {
            u32 better = (gun_ammo_usable_ck(plw, view, col, flags) & 1) != 0;
            u8 state = gun_clip_compare(plw, view, col, flags, &count);
            u32 at = *(const u32*)pos;
            ammo_table_cell_draw(piece, row, 1, (const _mh_ivec2_*)&at, count, state, better, flags);
            total_state |= state;
            total += count;
        }
        col = cols[2];
        if (col != 0xFF) {
            u32 better = (gun_ammo_usable_ck(plw, view, col, flags) & 1) != 0;
            u8 state = gun_clip_compare(plw, view, col, flags, &count);
            u32 at = *(const u32*)pos;
            ammo_table_cell_draw(piece, row, 2, (const _mh_ivec2_*)&at, count, state, better, flags);
            total_state |= state;
            total += count;
        }
        if (total == 0) {
            total_state |= 0x20;
        }
        u32 label_pos = *(const u32*)pos;
        ammo_table_row_label_draw(kind, row, (const _mh_ivec2_*)&label_pos, total_state, flags);
    }
}

/* 0x803142C8 - draw the base/selected run for one slot: resolve the `0x9AA` anchor against the
 * caller's position, then the `0xD0`/`0xD2` sprite lists at it. */
void ammo_table_row_frame_draw(u8 kind, u8 idx, const _mh_ivec2_* pos, u8 state, u8 mode) {
    u32 out1;
    u32 out2;
    u32 in = *(const u32*)pos;   /* the 4-byte `_mh_ivec2_` block, copied whole as the target does */
    u32 tmp;
    put_lsp_anchor_offset(0x9AA, &out1, &in);
    u16* tbl = (u16*)get_menu_lsp_tbl(0xD0);
    tmp = out1;
    put_lsp_anchor_offset(tbl[idx], &out2, &tmp);
    u16* tbl2 = (u16*)get_menu_lsp_tbl(0xD2);
    draw_sprite_ary(tbl2, (const _mh_ivec2_*)&out2);
}

/* 0x80314340 (0x160): Draws the slot's label from string table 0x3B or 0x3C at the `0xD0` row of the 0x9AA anchor,
 * green while the mode's bit 2 and a 0x18 state bit are set and grey for state bit 0x20. */
void ammo_table_row_label_draw(u8 kind, u8 idx, const _mh_ivec2_* pos, u8 state, u8 mode) {
    u32 base;
    u32 at;
    u32 in = *(const u32*)pos;
    u32 tmp;
    _SPR_DATA_ spr;
    u8** strs;

    put_lsp_anchor_offset(0x9AA, &base, &in);
    u16* tbl = (u16*)get_menu_lsp_tbl(0xD0);
    tmp = base;
    put_lsp_anchor_offset(tbl[idx], &at, &tmp);
    switch (kind) {
    case 0:
        strs = get_str_tbl(0x3B);
        break;
    case 1:
        strs = get_str_tbl(0x3C);
        break;
    default:
#line 6956
        nw4r::db::Panic("menu_infomation.cpp", __LINE__, "NW4R:Failed assertion 0");
        return;
    }
    spr_data_copy(&spr, get_lsp_data(0x9E3, NULL));
    if ((mode & 4) && (state & 0x18)) {
        spr.color = 0x50CD46FF;
    }
    if (state & 0x20) {
        spr.color = 0x878787FF;
    }
    draw_font(spr, (s8*)strs[idx], 4, (const _mh_ivec2_*)&at);
}

/* 0x803144A0 (0x164): Draws one slot cell: the `0xD3` icon at the `0xD6`/`0xD5` row and column anchors (tinted when
 * selected) and, for a non-zero value, the `0xD4` number sprite with the value printed in it. */
void ammo_table_cell_draw(u8 row, u8 col, u8 icon, const _mh_ivec2_* pos, u8 value, u8 state, u32 selected, u8 mode) {
    u32 base;
    u32 at;
    u32 in;
    u32 tmp;
    s8 buf[16];
    _SPR_DATA_ spr;

    u16* rows = (u16*)get_menu_lsp_tbl(0xD6);
    in = *(const u32*)pos;
    put_lsp_anchor_offset(rows[row], &base, &in);
    u16* cols = (u16*)get_menu_lsp_tbl(0xD5);
    tmp = base;
    put_lsp_anchor_offset(cols[col], &at, &tmp);
    u16* icons = (u16*)get_menu_lsp_tbl(0xD3);
    spr_data_copy(&spr, get_lsp_data(icons[icon], NULL));
    if (selected == 1) {
        spr.color = 0xAA825A7D;
    }
    draw_sprite(spr, (const _mh_ivec2_*)&at);
    if (value != 0) {
        spr_data_copy(&spr, get_lsp_data(((u16*)get_menu_lsp_tbl(0xD4))[icon], NULL));
        if ((mode & 4) && (state & 0x18)) {
            spr.color = 0x50CD46FF;
        }
        sprintf(buf, "%d", value);
        draw_font(spr, buf, 5, (const _mh_ivec2_*)&at);
    }
}

/* 0x80314604 - the next/previous slot step: the bowgun category (`Pl_Skill_slot_item_get(plw,0xB) == 0x17`)
 * clamps one higher than the rest. */
u8 sharpness_reach_get(_PLW* plw, s8 slot, u32 confirm) {
    u8 bowgun;
    bowgun = 0;
    if (confirm == 1 && Pl_Skill_slot_item_get(plw, 0xB) == 0x17) {
        bowgun++;
    }
    if (bowgun == 1) {
        slot++;
        if (slot > 6) {
            slot = 6;
        }
    } else {
        if (slot > 5) {
            slot = 5;
        }
    }
    return (u8)slot;
}

/* 0x8031468C (0x32C): Draws a blade's sharpness gauge at `pos`: the frame, the seven colour runs of its sharpness
 * level scaled into the 0x80B box, and a cover over the part past the sharpness the hunter reaches. */
void sharpness_gauge_draw(_PLW* plw, _EQUIP* equip, const _mh_ivec2_* pos, u8 flags) {
    _mh_ivec2_ origin;
    f32 scale[2];
    _mh_ivec2_ verts[7][4];
    _mh_tex_uv_ uvs[7][4];
    PlSkillRec* rec;
    s16* ends;
    _SPR_DATA_* box;
    s16 uv_width;
    s32 i;
    u8 reach;
    s16 cut;
    s16 width;
    s16 x;
    _mh_tex_uv_* tex;
    _mh_ivec2_* quad;
    s16 prev_x;
    s16 prev_u;

    rec = equip_blade_rec_get(equip);
    ends = (s16*)pl_sharpness_tbl[Get_pl_type(equip, NULL)];
    ends += rec->level_a * 7;
    draw_sprite_idx(0x80C, pos);
    box = get_lsp_data(0x80B, NULL);
    origin.x = pos->x + box->pos.x;
    origin.y = pos->y + box->pos.y;
    quad = verts[0];
    verts[0][0].x = verts[0][3].x = origin.x;
    verts[0][1].x = verts[0][2].x = origin.x + ends[0];
    verts[0][0].y = verts[0][1].y = origin.y;
    verts[0][2].y = verts[0][3].y = origin.y + box->height;
    tex = uvs[0];
    uv_width = box->uv1.u - box->uv0.u;
    uvs[0][0].u = uvs[0][3].u = box->uv0.u;
    uvs[0][1].u = uvs[0][2].u = box->uv0.u + uv_width * ends[0] / 450;
    uvs[0][0].v = uvs[0][1].v = box->uv0.v;
    uvs[0][2].v = uvs[0][3].v = box->uv1.v;
    for (i = 1; i < 7; i++) {
        prev_x = quad[1].x;
        prev_u = tex[1].u;
        quad += 4;
        tex += 4;
        ends++;
        quad[0].x = quad[3].x = prev_x;
        quad[1].x = quad[2].x = origin.x + *ends;
        quad[0].y = quad[1].y = verts[0][0].y;
        quad[2].y = quad[3].y = verts[0][2].y;
        tex[0].u = tex[3].u = prev_u;
        tex[1].u = tex[2].u = box->uv0.u + uv_width * *ends / 450;
        tex[0].v = tex[1].v = uvs[0][0].v;
        tex[2].v = tex[3].v = uvs[0][2].v;
    }
    scale[0] = (1.0f / 450.0f) * box->width;
    scale[1] = 1.0f;
    drawshape_init(3, 28);
    drawshape_set_vertex_array(&verts[0][0]);
    drawshape_set_color_array(sharpness_colors);
    drawshape_set_scale_ivec2(&origin.x, scale);
    drawshape_set_texture_array(box->tex_flag, &uvs[0][0]);
    drawshape_exec();
    reach = sharpness_reach_get(plw, rec->level_b, (flags & 2) != 0);
    if (reach < 6) {
        cut = box->width * (reach * 50 + 150) / 450;
        x = origin.x + cut;
        width = box->width - cut;
        drawshape_init(3, 0xFFFF);
        drawshape_set_vertex_rect(x, origin.y, width, box->height);
        drawshape_set_flat_color(0xFF);
        drawshape_exec();
    }
}

/* 0x80315274 (0x184): With flag bit 1, copies the skills the shown equipment would activate into the hunter's
 * shown-skill list; returns how many it copied. */
u8 shown_skills_set(_PLW* plw, _EQUIP_INDEX* view, u8 flags) {
    u8 count = 0;
    s32 i;
    u16 skill_ids[8];
    s8 skill_points[8];
    u8 skill_trees[8];
    _EQUIP columns[9];

    if (flags & 2) {
        Set_equip_column_arrangement(plw, view, columns);
        equip_columns_gun_view_set(plw, view, columns);
        equip_columns_skills_get(columns, skill_ids, skill_points, skill_trees);
        for (i = 0; i < 8; i++) {
            if (skill_ids[i] != 0 && skill_trees[i] != 0) {
                plw->shown_skill_id[count] = skill_ids[i];
                plw->shown_skill_tree[count] = skill_trees[i];
                count++;
            }
        }
    }
    return count;
}

/* 0x803149B8 (0xAC): Draws the decoration slots of the shown piece (class 1/2/5/6) or of the bowgun's barrel and
 * stock (3/4/7/8); returns the slot count. */
u8 deco_slots_view_draw(u16 anchor, _EQUIP_INDEX* view, u8 sel, const _mh_ivec2_* pos) {
    _EQUIP* v0 = view->shown[0];
    _EQUIP* v1 = view->shown[1];
    _EQUIP* v2 = view->shown[2];
    u8 kind = equip_compare_class(sel);
    switch (kind) {
    case 1:
    case 2:
    case 5:
    case 6:
        return deco_slots_draw(anchor, v0, NULL, pos);
    case 3:
    case 4:
    case 7:
    case 8:
        return deco_slots_draw(anchor, v1, v2, pos);
    }
    return 0;
}

/* 0x80314A64 (0x4B4): Draws a piece's decoration slots at anchor `anchor` (0x8F0: the caller's position as is): the
 * slot frame for its slot count and each filled slot's jewel icon tinted with the jewel's rarity colour; returns the
 * slot count. */
u8 deco_slots_draw(u16 anchor, _EQUIP* equip, _EQUIP* sub, const _mh_ivec2_* pos) {
    _mh_ivec2_ at;
    u32 in;
    u16 decos[5];
    _SPR_DATA_ spr;
    u8 slots;
    u8 filled;
    u16* icons;
    u32 color;

    slots = equip_slot_count_get(equip, sub);
    if (slots == 0) {
        return 0;
    }
    if (anchor != 0x8F0) {
        in = *(const u32*)pos;
        put_lsp_anchor_offset(anchor, &at, &in);
    } else {
        at.x = pos->x;
        at.y = pos->y;
    }
    if (anchor == 0x8F0) {
        icons = (u16*)get_menu_lsp_tbl(0xAC);
    } else {
        icons = (u16*)get_menu_lsp_tbl(0xB0);
    }
    draw_sprite_ary(((u16**)icons)[slots], &at);
    filled = equip_decos_sort(equip, sub, decos);
    if (anchor == 0x8F0) {
        icons = (u16*)get_menu_lsp_tbl(0xB1);
    } else {
        icons = (u16*)get_menu_lsp_tbl(0xB2);
    }
    if (filled & 1) {
        color = deco_rare_color_get(decos[0]);
        switch (slots) {
        case 1:
            spr_data_copy(&spr, get_lsp_data(icons[1], NULL));
            break;
        case 2:
            spr_data_copy(&spr, get_lsp_data(icons[3], NULL));
            break;
        case 3:
            spr_data_copy(&spr, get_lsp_data(icons[0], NULL));
            break;
        default:
            nw4r::db::Panic("menu_infomation.cpp", 7244, "NW4R:Failed assertion 0");
            color = 0;
            break;
        }
        if (color != 0) {
            spr.color = color;
            draw_sprite(spr, &at);
        }
    }
    if (filled & 2) {
        color = deco_rare_color_get(decos[1]);
        switch (slots) {
        case 2:
            spr_data_copy(&spr, get_lsp_data(icons[4], NULL));
            break;
        case 3:
            spr_data_copy(&spr, get_lsp_data(icons[1], NULL));
            break;
        default:
            nw4r::db::Panic("menu_infomation.cpp", 7268, "NW4R:Failed assertion 0");
            color = 0;
            break;
        }
        if (color != 0) {
            spr.color = color;
            draw_sprite(spr, &at);
        }
    }
    if (filled & 4) {
        spr_data_copy(&spr, get_lsp_data(icons[2], NULL));
        spr.color = deco_rare_color_get(decos[2]);
        draw_sprite(spr, &at);
    }
    if (filled & 8) {
        color = deco_rare_color_get(decos[3]);
        if (slots == 2) {
            spr_data_copy(&spr, get_lsp_data(icons[7], NULL));
            spr.color = color;
            draw_sprite(spr, &at);
            spr_data_copy(&spr, get_lsp_data(icons[3], NULL));
            spr.color = color;
            draw_sprite(spr, &at);
            spr_data_copy(&spr, get_lsp_data(icons[4], NULL));
            spr.color = color;
            draw_sprite(spr, &at);
        } else {
            spr_data_copy(&spr, get_lsp_data(icons[5], NULL));
            spr.color = color;
            draw_sprite(spr, &at);
            spr_data_copy(&spr, get_lsp_data(icons[0], NULL));
            spr.color = color;
            draw_sprite(spr, &at);
            spr_data_copy(&spr, get_lsp_data(icons[1], NULL));
            spr.color = color;
            draw_sprite(spr, &at);
        }
    }
    if (filled & 0x10) {
        color = deco_rare_color_get(decos[4]);
        spr_data_copy(&spr, get_lsp_data(icons[5], NULL));
        spr.color = color;
        draw_sprite(spr, &at);
        spr_data_copy(&spr, get_lsp_data(icons[6], NULL));
        spr.color = color;
        draw_sprite(spr, &at);
        spr_data_copy(&spr, get_lsp_data(icons[0], NULL));
        spr.color = color;
        draw_sprite(spr, &at);
        spr_data_copy(&spr, get_lsp_data(icons[1], NULL));
        spr.color = color;
        draw_sprite(spr, &at);
        spr_data_copy(&spr, get_lsp_data(icons[2], NULL));
        spr.color = color;
        draw_sprite(spr, &at);
    }
    return slots;
}

/* 0x80314F18 - sum the equipment "category weight" of two pieces: the second only counts when the
 * first is a weapon (kind 0xC) and the second is its matching form (kind 0xD). */
u8 equip_slot_count_get(_EQUIP* a, _EQUIP* b) {
    u8 total = equip_deco_slots_get(a);
    if (a->kind == 0xC && b != NULL && b->kind == 0xD) {
        total = (u8)(total + equip_deco_slots_get(b));
    }
    return total;
}



/* 0x803153F8 - clear the eight column entries of a hunter's panel. */
void shown_skills_clear(_PLW* self, u8 unused) {
    self->shown_skill_id[0] = 0;
    self->shown_skill_tree[0] = 0;
    self->shown_skill_id[1] = 0;
    self->shown_skill_tree[1] = 0;
    self->shown_skill_id[2] = 0;
    self->shown_skill_tree[2] = 0;
    self->shown_skill_id[3] = 0;
    self->shown_skill_tree[3] = 0;
    self->shown_skill_id[4] = 0;
    self->shown_skill_tree[4] = 0;
    self->shown_skill_id[5] = 0;
    self->shown_skill_tree[5] = 0;
    self->shown_skill_id[6] = 0;
    self->shown_skill_tree[6] = 0;
    self->shown_skill_id[7] = 0;
    self->shown_skill_tree[7] = 0;
}

/* 0x80315440 (0x17C): Resolves one piece into the view: the shown piece is the piece itself (flag bit 0), the
 * caller's second piece or the worn record of its kind, and the worn column is the second piece or that record. */
u8 equip_variant_resolve(_PLW* plw, _EQUIP* equip, _EQUIP* other, u8* flags, _EQUIP_INDEX* view) {
    u8 ret = 0;
    _EQUIP* worn;

    view->shown[0] = NULL;
    view->shown[1] = NULL;
    view->shown[2] = NULL;
    view->worn[0] = NULL;
    view->worn[1] = NULL;
    view->worn[2] = NULL;
    if (plw == NULL) {
        return 1;
    }
    if (equip == NULL) {
        return 1;
    }
    *flags |= 0x20;
    switch (equip->kind) {
    case 11:
        worn = &plw->equipB;
        break;
    case 12:
        if (plw->equipB.kind == 0xB) {
            worn = &plw->equipC;
        } else {
            worn = &plw->equipB;
        }
        break;
    case 13:
        if (plw->equipB.kind == 0xB) {
            worn = &plw->equipD;
        } else {
            worn = &plw->equipB;
        }
        break;
    case 1:
        worn = &plw->equipA[0];
        break;
    case 2:
        worn = &plw->equipA[1];
        break;
    case 3:
        worn = &plw->equipA[2];
        break;
    case 4:
        worn = &plw->equipA[3];
        break;
    case 5:
        worn = &plw->equipA[4];
        break;
    case 6:
        worn = &plw->equipA[5];
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        worn = &plw->equipB;
        break;
    default:
        ret = 1;
        break;
    }
    if (*flags & 1) {
        view->shown[0] = equip;
        if (worn->item_id == 0 && other == NULL) {
            *flags &= ~8;
        }
    } else if (other != NULL) {
        view->shown[0] = other;
    } else {
        view->shown[0] = worn;
    }
    if (other != NULL) {
        view->worn[0] = other;
        if (other->item_id != 0) {
            *flags |= 0x10;
        } else {
            *flags &= ~8;
        }
    } else {
        view->worn[0] = worn;
    }
    return ret;
}

/* 0x803155BC (0x174): Resolves a bowgun triple into the view: with flag bit 0 the caller's frame/barrel/stock
 * (a lone part completed from the worn set), otherwise the worn set; the worn column is always the worn set. */
u8 equip_variant_resolve_ex(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, u8 flags,
                            _EQUIP_INDEX* view) {
    u8 ret = 0;

    view->shown[0] = NULL;
    view->shown[1] = NULL;
    view->shown[2] = NULL;
    view->worn[0] = NULL;
    view->worn[1] = NULL;
    view->worn[2] = NULL;
    if (plw == NULL) {
        ret = 1;
    } else if (flags & 1) {
        if (frame == NULL) {
            ret = 1;
        } else if (barrel == NULL && stock == NULL) {
            switch (frame->kind) {
            case 11:
                view->shown[0] = frame;
                view->shown[1] = &plw->equipC;
                view->shown[2] = &plw->equipD;
                break;
            case 12:
                view->shown[0] = &plw->equipB;
                view->shown[1] = frame;
                view->shown[2] = &plw->equipD;
                break;
            case 13:
                view->shown[0] = &plw->equipB;
                view->shown[1] = &plw->equipC;
                view->shown[2] = frame;
                break;
            default:
                ret = 1;
                break;
            }
        } else {
            if (frame->kind == 0xB) {
                view->shown[0] = frame;
            } else {
                ret = 1;
            }
            if (barrel != NULL) {
                if (barrel->kind == 0xC) {
                    view->shown[1] = barrel;
                } else {
                    ret = 1;
                }
            } else {
                ret = 1;
            }
            if (stock != NULL) {
                if (stock->kind == 0xD) {
                    view->shown[2] = stock;
                } else {
                    ret = 1;
                }
            } else {
                ret = 1;
            }
        }
    } else {
        view->shown[0] = &plw->equipB;
        view->shown[1] = &plw->equipC;
        view->shown[2] = &plw->equipD;
    }
    view->worn[0] = &plw->equipB;
    view->worn[1] = &plw->equipC;
    view->worn[2] = &plw->equipD;
    return ret;
}

/* 0x80315730 (0x29C): Resolves a bowgun triple against a second triple: the shown column as the 0x803155BC
 * resolver picks it (the second triple when flag bit 0 is clear and it is complete), the worn column from the
 * second triple when it is a whole frame/barrel/stock set, else the worn set; flag bit 3 is cleared when the worn
 * column is not a whole, filled set. */
u8 equip_variant_resolve_pair(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, _EQUIP* other_frame,
               _EQUIP* other_barrel, _EQUIP* other_stock, u8* flags, _EQUIP_INDEX* view) {
    u8 ret = 0;

    view->shown[0] = NULL;
    view->shown[1] = NULL;
    view->shown[2] = NULL;
    view->worn[0] = NULL;
    view->worn[1] = NULL;
    view->worn[2] = NULL;
    if (plw == NULL) {
        ret = 1;
    } else if (*flags & 1) {
        if (frame == NULL) {
            ret = 1;
        } else if (barrel == NULL && stock == NULL) {
            switch (frame->kind) {
            case 11:
                view->shown[0] = frame;
                view->shown[1] = &plw->equipC;
                view->shown[2] = &plw->equipD;
                break;
            case 12:
                view->shown[0] = &plw->equipB;
                view->shown[1] = frame;
                view->shown[2] = &plw->equipD;
                break;
            case 13:
                view->shown[0] = &plw->equipB;
                view->shown[1] = &plw->equipC;
                view->shown[2] = frame;
                break;
            default:
                ret = 1;
                break;
            }
        } else {
            if (frame->kind == 0xB) {
                view->shown[0] = frame;
            } else {
                ret = 1;
            }
            if (barrel != NULL) {
                if (barrel->kind == 0xC) {
                    view->shown[1] = barrel;
                } else {
                    ret = 1;
                }
            } else {
                ret = 1;
            }
            if (stock != NULL) {
                if (stock->kind == 0xD) {
                    view->shown[2] = stock;
                } else {
                    ret = 1;
                }
            } else {
                ret = 1;
            }
        }
    } else if (other_frame != NULL && other_barrel != NULL && other_stock != NULL) {
        view->shown[0] = other_frame;
        view->shown[1] = other_barrel;
        view->shown[2] = other_stock;
    } else {
        view->shown[0] = &plw->equipB;
        view->shown[1] = &plw->equipC;
        view->shown[2] = &plw->equipD;
    }
    if (other_frame != NULL && other_barrel != NULL && other_stock != NULL) {
        if (other_frame->kind == 0xB && other_barrel->kind == 0xC && other_stock->kind == 0xD) {
            view->worn[0] = other_frame;
            view->worn[1] = other_barrel;
            view->worn[2] = other_stock;
        } else {
            *flags &= ~8;
            view->worn[0] = &plw->equipB;
            view->worn[1] = &plw->equipC;
            view->worn[2] = &plw->equipD;
        }
    } else if (plw->equipB.kind == 0xB && plw->equipC.kind == 0xC && plw->equipD.kind == 0xD) {
        view->worn[0] = &plw->equipB;
        view->worn[1] = &plw->equipC;
        view->worn[2] = &plw->equipD;
    } else {
        *flags &= ~8;
        view->worn[0] = &plw->equipB;
        view->worn[1] = &plw->equipC;
        view->worn[2] = &plw->equipD;
    }
    if (view->worn[0]->item_id == 0 || view->worn[1]->item_id == 0 || view->worn[2]->item_id == 0) {
        *flags &= ~8;
    }
    return ret;
}

/* 0x803159CC - unpack the three-bit equipment class from a flags byte: bit 0 selects the high/low
 * group, bit 5 the sub-group within it, bit 1 the last choice. */
u8 equip_compare_class(u8 flags) {
    u8 ret;
    if (flags & 1) {
        if (flags & 0x20) {
            if (flags & 2) {
                ret = 1;
            } else {
                ret = 2;
            }
        } else {
            if (flags & 2) {
                ret = 3;
            } else {
                ret = 4;
            }
        }
    } else {
        if (flags & 0x20) {
            if (flags & 2) {
                ret = 5;
            } else {
                ret = 6;
            }
        } else {
            if (flags & 2) {
                ret = 7;
            } else {
                ret = 8;
            }
        }
    }
    return ret;
}

/* 0x80315A60 (0x1A0): Checks whether the hunter can show the view's piece: 1 on a missing record, 2 for an empty
 * piece, else 0 or the reason the hunter's class cannot wear it (3, 6..9). */
u8 equip_wear_check(_PLW* plw, _EQUIP_INDEX* view, u8 flags) {
    _EQUIP* shown = view->shown[0];
    u8 ret = 0;

    if (plw == NULL) {
        nw4r::db::Panic("menu_infomation.cpp", 7959, "NW4R:Failed assertion 0");
        ret = 1;
    } else if (shown == NULL) {
        nw4r::db::Panic("menu_infomation.cpp", 7963, "NW4R:Failed assertion 0");
        ret = 1;
    } else if (shown->item_id == 0) {
        ret = 2;
    } else if (flags & 1) {
        switch (shown->kind) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            if (flags & 8) {
                PlSlotRec* rec = equip_slot_rec_get_c1(shown);
                if (plw->equipB.kind == 0xB) {
                    if (!(rec->wear_flags & 8)) {
                        ret = 6;
                    }
                } else if (!(rec->wear_flags & 4)) {
                    ret = 7;
                }
            }
            break;
        case 11:
        case 12:
        case 13:
            if ((flags & 8) && plw->equipB.kind != 0xB) {
                ret = 9;
            }
            break;
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
            if ((flags & 8) && plw->equipB.kind == 0xB) {
                ret = 8;
            }
            break;
        }
    } else if ((u32)(shown->kind - 0xB) <= 2 && plw->equipB.kind != 0xB) {
        ret = 3;
    }
    return ret;
}

/* 0x80315C00 (0x10C): Reports the shown armour piece's defence; bits 1/2 compare it with the worn piece's. */
u8 equip_defense_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* out_value) {
    u8 ret = 0;
    u16 worn_value = 0;
    u16 value = 0;
    u32 has_worn;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        has_worn = view->worn[0]->item_id != 0;
        if (has_worn) {
            worn_value = equip_defense_get_c1(view->worn[0]);
        }
        if (!has_worn) {
            flags &= ~8;
        }
        value = equip_defense_get_c1(view->shown[0]);
        ret |= 1;
        break;
    case 5:
    case 6:
        value = equip_defense_get_c1(view->shown[0]);
        ret |= 1;
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        *out_value = value;
    }
    return ret;
}

/* 0x80315D0C (0x128): Reports the shown piece's upgrade level (one-based; bit 6 at its cap); bits 1/2 compare it
 * with the worn piece's. */
u8 equip_level_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_level) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u8 ret = 0;
    u8 worn_level = 0;
    u8 level = 0;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if (worn->item_id != 0) {
            worn_level = worn->deco_count;
        }
        if (worn->item_id == 0) {
            flags &= ~8;
        }
        level = shown->deco_count;
        if (level == equip_level_cap_get(shown)) {
            ret |= 0x40;
        }
        ret |= 1;
        break;
    case 5:
    case 6:
        level = shown->deco_count;
        if (level == equip_level_cap_get(shown)) {
            ret |= 0x40;
        }
        ret |= 1;
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_level != level) {
            ret |= (worn_level < level) ? 2 : 4;
        }
        *out_level = level + 1;
    }
    return ret;
}

/* Reads one of an armour row's five resistances (1 fire, 2 water, 3 thunder, 4 ice, 5 dragon). */
#define EQUIP_RESIST(rec, element, out)  \
    switch (element) {                   \
    case 1:                              \
        out = (rec)->value_0x08;         \
        break;                           \
    case 2:                              \
        out = (rec)->value_0x09;         \
        break;                           \
    case 3:                              \
        out = (rec)->value_0x0A;         \
        break;                           \
    case 4:                              \
        out = (rec)->value_0x0C;         \
        break;                           \
    case 5:                              \
        out = (rec)->value_0x0B;         \
        break;                           \
    }

/* 0x80315E34 (0x224): Reports one resistance of the shown armour piece; bits 1/2 compare it with the worn piece's. */
u8 equip_resist_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s32 element, s16* out_value) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u8 ret = 0;
    s16 worn_value = 0;
    s16 value = 0;
    PlSlotRec* rec;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if (worn->item_id == 0) {
            flags &= ~8;
        } else {
            rec = equip_slot_rec_get_c1(worn);
            EQUIP_RESIST(rec, element, worn_value);
        }
        rec = equip_slot_rec_get_c1(shown);
        EQUIP_RESIST(rec, element, value);
        ret |= 1;
        break;
    case 5:
    case 6:
        rec = equip_slot_rec_get_c1(shown);
        EQUIP_RESIST(rec, element, value);
        ret |= 1;
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        *out_value = value;
    }
    return ret;
}

/* 0x80316058 (0x1D4): Reports the shown weapon's displayed attack; bits 1/2 compare it with the worn weapon's. */
u8 weapon_attack_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* out_value) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u16 value;
    u16 worn_value;
    u8 ret;
    u8 err;

    ret = 0;
    err = 0;
    value = 0;
    worn_value = 0;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if (flags & 8) {
            worn_value = weapon_attack_display(worn->kind, pl_weapon_stat_get_c1(plw, 6, 0, worn, NULL, NULL, &err));
        }
        err = 0;
        value = weapon_attack_display(shown->kind, pl_weapon_stat_get_c1(plw, 6, 0, shown, NULL, NULL, &err));
        if (err) {
            nw4r::db::Panic("menu_infomation.cpp", 8289, "NW4R:Failed assertion 0");
        }
        ret |= 1;
        break;
    case 5:
    case 6:
        value = weapon_attack_display(shown->kind, pl_weapon_stat_get_c1(plw, 6, 0, shown, NULL, NULL, &err));
        if (err) {
            nw4r::db::Panic("menu_infomation.cpp", 8302, "NW4R:Failed assertion 0");
        }
        ret |= 1;
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        *out_value = value;
    }
    return ret;
}

/* 0x8031622C (0x2A4): Reports the element A of the shown piece's weapon record: its type (biased by 6) and its
 * value times ten; a hidden (negative) value counts only with skill 0xC9, and bits 1/2 compare it with the worn one. */
u8 weapon_element_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_type, s16* out_value) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u8 ret = 0;
    s32 hidden = 0;
    u8 type = 0;
    s16 value = 0;
    u8 worn_type = 0;
    s16 worn_value = 0;
    PlSkillRec* rec;
    u32 skill;

    *out_type = 0;
    *out_value = 0;
    skill = 0;
    if ((flags & 2) && (flags & 4) && Pl_Skill_ck(plw, 0xC9) == 1) {
        skill = 1;
    }
    u8 kind = equip_compare_class(flags);
    switch (kind) {
    case 1:
    case 2:
        if ((u8)(worn->kind + 0xF5) <= 2) {
            worn_type = 0;
            worn_value = 0;
        } else {
            rec = equip_blade_rec_get(worn);
            if (rec != NULL && rec->element_a_type != 0) {
                if (rec->element_a_value > 0) {
                    worn_type = rec->element_a_type + 6;
                    worn_value = rec->element_a_value;
                } else if (skill == 1) {
                    worn_type = rec->element_a_type + 6;
                    worn_value = rec->element_a_value - rec->element_a_value * 2;
                }
            }
        }
        rec = equip_blade_rec_get(shown);
        if (rec != NULL && rec->element_a_type != 0) {
            if (rec->element_a_value > 0) {
                type = rec->element_a_type + 6;
                value = rec->element_a_value;
                ret |= 1;
            } else if (skill == 1) {
                type = rec->element_a_type + 6;
                value = rec->element_a_value - rec->element_a_value * 2;
                hidden = 1;
                ret |= 1;
            }
        }
        break;
    case 5:
    case 6:
        rec = equip_blade_rec_get(shown);
        if (rec != NULL && rec->element_a_type != 0) {
            if (rec->element_a_value > 0) {
                type = rec->element_a_type + 6;
                value = rec->element_a_value;
                ret |= 1;
            } else if (skill == 1) {
                type = rec->element_a_type + 6;
                value = rec->element_a_value - rec->element_a_value * 2;
                hidden = 1;
                ret |= 1;
            }
        }
        break;
    }
    worn_value *= 10;
    value *= 10;
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_type != 0 && worn_type == type && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        if (flags & 4) {
            switch (hidden) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_type = type;
        *out_value = value;
    }
    return ret;
}

/* 0x803164D0 (0x2B0): Reports the element B of the shown piece's weapon record: its type (biased by 11) and its
 * value times ten; a hidden (negative) value counts only with skill 0xC9, and bits 1/2 compare it with the worn one. */
u8 weapon_status_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_type, s16* out_value) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u8 ret = 0;
    s32 hidden = 0;
    u8 type = 0;
    s16 value = 0;
    u8 worn_type = 0;
    s16 worn_value = 0;
    PlSkillRec* rec;
    u32 skill;

    *out_type = 0;
    *out_value = 0;
    skill = 0;
    if ((flags & 2) && (flags & 4) && Pl_Skill_ck(plw, 0xC9) == 1) {
        skill = 1;
    }
    u8 kind = equip_compare_class(flags);
    switch (kind) {
    case 1:
    case 2:
        if ((u8)(worn->kind + 0xF5) <= 2) {
            worn_type = 0;
            worn_value = 0;
        } else {
            rec = equip_blade_rec_get(worn);
            if (rec != NULL && rec->element_b_type != 0) {
                if (rec->element_b_value > 0) {
                    worn_type = rec->element_b_type + 11;
                    worn_value = rec->element_b_value;
                } else if (skill == 1) {
                    worn_type = rec->element_b_type + 11;
                    worn_value = rec->element_b_value - rec->element_b_value * 2;
                }
            }
        }
        rec = equip_blade_rec_get(shown);
        if (rec != NULL && rec->element_b_type != 0) {
            if (rec->element_b_value > 0) {
                type = rec->element_b_type + 11;
                value = rec->element_b_value;
                ret |= 1;
            } else if (skill == 1) {
                type = rec->element_b_type + 11;
                value = rec->element_b_value - rec->element_b_value * 2;
                hidden = 1;
                ret |= 1;
            }
        }
        break;
    case 5:
    case 6:
        rec = equip_blade_rec_get(shown);
        if (rec != NULL && rec->element_b_type != 0) {
            if (rec->element_b_value > 0) {
                type = rec->element_b_type + 11;
                value = rec->element_b_value;
                ret |= 1;
            } else if (skill == 1) {
                type = rec->element_b_type + 11;
                value = rec->element_b_value - rec->element_b_value * 2;
                hidden = 1;
                ret |= 1;
            }
        }
        break;
    default:
        return 0;
    }
    worn_value *= 10;
    value *= 10;
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_type != 0 && worn_type == type && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        if (flags & 4) {
            switch (hidden) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_type = type;
        *out_value = value;
    }
    return ret;
}

/* 0x80316780 (0x1F8): Reports the shown weapon's affinity (the hunter's skills apply to the shown class 1/5 only);
 * bits 1/2 compare it with the worn weapon's, bits 3/4 flag a skill-raised or -lowered value. */
u8 weapon_affinity_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s16* out_value) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u8 ret = 0;
    u8 err = 0;
    s16 value = 0;
    s16 worn_value = 0;
    u8 kind = equip_compare_class(flags);

    switch (kind) {
    case 1:
    case 2:
        worn_value = pl_weapon_affinity_get_c1((u8)(worn->kind + 0xF5) <= 2 ? NULL : (kind == 1 ? plw : NULL), worn,
                                            NULL, NULL, &err);
        err = 0;
        plw = (kind == 1) ? plw : NULL;
        value = pl_weapon_affinity_get_c1(plw, shown, NULL, NULL, &err);
        if (plw == NULL && err) {
            nw4r::db::Panic("menu_infomation.cpp", 8638, "NW4R:Failed assertion 0");
            err = 0;
        }
        ret |= 1;
        break;
    case 5:
    case 6:
        value = pl_weapon_affinity_get_c1(kind == 5 ? plw : NULL, shown, NULL, NULL, &err);
        ret |= 1;
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        if (flags & 4) {
            switch (err) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_value = value;
    }
    return ret;
}

/* 0x80316978 - read the class byte at +0x0C of the `equip_blade_rec_get` record for the classes that
 * carry it (1/2/5/6); returns bit 0 set when the byte was written. */
u8 weapon_slot_count_get(_PLW* plw, _EQUIP_INDEX* view, u8 sel, u8* out) {
    u8 ret = 0;
    u8 v = 0;
    u8 kind = equip_compare_class(sel);
    if ((u32)(kind - 1) <= 1 || (u32)(kind - 5) <= 1) {
        PlSkillRec* q = equip_blade_rec_get(view->shown[0]);
        if (q != 0 && q->slot_count != 0) {
            v = q->slot_count;
            ret |= 1;
        }
    }
    *out = v;
    return ret;
}

/* 0x80316A14 (0x3C8): Reports the displayed attack of the shown bowgun (frame with its barrel; class 3/7 the whole
 * frame/barrel/stock set); bits 1/2 compare it with the worn one, bit 5 marks a stock, which has none. */
u8 gun_attack_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* out_value) {
    _EQUIP* frame;
    u16 value;
    u16 worn_value;
    _EQUIP* barrel;
    _EQUIP* worn_frame;
    u8 ret;
    _EQUIP* stock;
    _EQUIP* worn_barrel;
    u8 err;
    u16 raw;

    frame = view->shown[0];
    barrel = view->shown[1];
    stock = view->shown[2];
    worn_frame = view->worn[0];
    worn_barrel = view->worn[1];
    ret = 0;
    err = 0;
    value = 0;
    worn_value = 0;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if ((u8)(frame->kind + 0xF5) <= 1) {
            worn_value = weapon_attack_display(worn_frame->kind,
                                     pl_weapon_stat_get_c1(plw, 6, 0, worn_frame, NULL, NULL, &err));
            err = 0;
            value = weapon_attack_display(frame->kind, pl_weapon_stat_get_c1(plw, 6, 0, frame, NULL, NULL, &err));
            if (err) {
                nw4r::db::Panic("menu_infomation.cpp", 8755, "NW4R:Failed assertion 0");
            }
            ret |= 1;
        } else if (frame->kind == 0xD) {
            ret |= 0x20;
        }
        break;
    case 3:
    case 4:
        if (frame->kind == 0xB && barrel->kind == 0xC) {
            if (worn_frame->kind == 0xB) {
                raw = pl_weapon_stat_get_c1(plw, 6, 0, worn_frame, worn_barrel, NULL, &err);
            } else {
                raw = pl_weapon_stat_get_c1(plw, 6, 0, worn_frame, NULL, NULL, &err);
            }
            worn_value = weapon_attack_display(worn_frame->kind, raw);
            err = 0;
            value = weapon_attack_display(frame->kind, pl_weapon_stat_get_c1(plw, 6, 0, frame, barrel, NULL, &err));
            if (err) {
                nw4r::db::Panic("menu_infomation.cpp", 8780, "NW4R:Failed assertion 0");
            }
            ret |= 1;
        }
        break;
    case 5:
    case 6:
        switch ((u32)frame->kind) {
        case 11:
            value = pl_weapon_stat_get_c1(plw, 6, 0, frame, NULL, NULL, &err);
        case 12:
            if (frame->kind == 0xC) {
                value = pl_weapon_stat_get_c1(plw, 6, 0, NULL, frame, NULL, &err);
            }
            value = weapon_attack_display(frame->kind, value);
            if (err) {
                nw4r::db::Panic("menu_infomation.cpp", 8797, "NW4R:Failed assertion 0");
            }
            ret |= 1;
            break;
        case 13:
            ret |= 0x20;
            break;
        }
        break;
    case 7:
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            value = weapon_attack_display(frame->kind, pl_weapon_stat_get_c1(plw, 6, 0, frame, barrel, NULL, &err));
            if (err) {
                nw4r::db::Panic("menu_infomation.cpp", 8814, "NW4R:Failed assertion 0");
            }
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        *out_value = value;
    }
    return ret;
}

/* 0x80316DDC (0x2D8): Reports the shown weapon's affinity, a bowgun's summed over frame, barrel and stock (the
 * hunter's skills apply to the shown class 1/3/7 only); bits 1/2 compare it with the worn one, bits 3/4 flag a
 * skill-raised or -lowered value. */
u8 gun_affinity_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s16* out_value) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    _EQUIP* worn_stock = view->worn[2];
    u8 ret = 0;
    u8 err = 0;
    s16 value = 0;
    s16 worn_value = 0;
    u8 kind = equip_compare_class(flags);

    switch (kind) {
    case 1:
    case 2:
        if ((u8)(worn_frame->kind + 0xF5) <= 2) {
            plw = NULL;
        } else {
            plw = (kind == 1) ? plw : NULL;
        }
        worn_value = pl_weapon_affinity_get_c1(plw, worn_frame, NULL, NULL, &err);
        err = 0;
        value = pl_weapon_affinity_get_c1(NULL, frame, NULL, NULL, &err);
        if (plw == NULL && err) {
            nw4r::db::Panic("menu_infomation.cpp", 8881, "NW4R:Failed assertion 0");
            err = 0;
        }
        ret |= 1;
        break;
    case 3:
    case 4:
        plw = (kind == 3) ? plw : NULL;
        if (worn_frame->kind == 0xB) {
            worn_value = pl_weapon_affinity_get_c1(plw, worn_frame, worn_barrel, worn_stock, &err);
        } else {
            worn_value = pl_weapon_affinity_get_c1(plw, worn_frame, NULL, NULL, &err);
        }
        err = 0;
        value = pl_weapon_affinity_get_c1(plw, frame, barrel, stock, &err);
        if (plw == NULL && err) {
            nw4r::db::Panic("menu_infomation.cpp", 8904, "NW4R:Failed assertion 0");
            err = 0;
        }
        ret |= 1;
        break;
    case 5:
    case 6:
        value = pl_weapon_affinity_get_c1(NULL, frame, NULL, NULL, &err);
        ret |= 1;
        break;
    case 7:
    case 8:
        value = pl_weapon_affinity_get_c1(kind == 7 ? plw : NULL, frame, barrel, stock, &err);
        ret |= 1;
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        if (flags & 4) {
            switch (err) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_value = value;
    }
    return ret;
}

/* 0x803170B4 (0x1E8): Reports the shown weapon's defence bonus, a bowgun's summed over frame, barrel and stock. */
u8 gun_defense_bonus_get(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u32* out_value) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    _EQUIP* worn_stock = view->worn[2];
    u8 ret = 0;
    u32 value = 0;
    u32 worn_value = 0;
    PlWeaponRec* rec;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if ((u8)(worn_frame->kind + 0xF5) <= 2) {
            rec = equip_weapon_rec_get_c1(worn_frame);
            if (rec != NULL) {
                worn_value = rec->defense_bonus;
            }
        }
        rec = equip_weapon_rec_get_c1(frame);
        if (rec != NULL) {
            value = rec->defense_bonus;
        }
        ret |= 1;
        break;
    case 3:
    case 4:
        if (worn_frame->kind == 0xB) {
            rec = equip_weapon_rec_get_c1(worn_frame);
            if (rec != NULL) {
                worn_value = rec->defense_bonus;
            }
            rec = equip_weapon_rec_get_c1(worn_barrel);
            if (rec != NULL) {
                worn_value += rec->defense_bonus;
            }
            rec = equip_weapon_rec_get_c1(worn_stock);
            if (rec != NULL) {
                worn_value += rec->defense_bonus;
            }
        }
        rec = equip_weapon_rec_get_c1(frame);
        if (rec != NULL) {
            value = rec->defense_bonus;
        }
        rec = equip_weapon_rec_get_c1(barrel);
        if (rec != NULL) {
            value += rec->defense_bonus;
        }
        rec = equip_weapon_rec_get_c1(stock);
        if (rec != NULL) {
            value += rec->defense_bonus;
        }
        ret |= 1;
        break;
    case 5:
    case 6:
        if ((u8)(frame->kind + 0xF5) <= 2) {
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                value = rec->defense_bonus;
            }
            ret |= 1;
        }
        break;
    case 7:
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                value = rec->defense_bonus;
            }
            rec = equip_weapon_rec_get_c1(barrel);
            if (rec != NULL) {
                value += rec->defense_bonus;
            }
            rec = equip_weapon_rec_get_c1(stock);
            if (rec != NULL) {
                value += rec->defense_bonus;
            }
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        *out_value = value;
    }
    return ret;
}


/* 0x8031729C (0x128): For the bowgun classes (3/4/7/8) with a whole frame/barrel/stock view, stores the
 * `Get_pl_type` of the frame and copies of the barrel and stock; returns bit 0 set when it stored one. */
u8 gun_size_get(u32 unused, _EQUIP_INDEX* view, u8 sel, u8* out) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    u8 ret = 0;
    u8 type = 0;

    switch (equip_compare_class(sel)) {
    case 3:
    case 4:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            _EQUIP parts[2];
            equip_record_copy(&parts[0], barrel);
            equip_record_copy(&parts[1], stock);
            type = Get_pl_type(frame, parts);
            ret |= 1;
        }
        break;
    case 7:
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            _EQUIP parts[2];
            equip_record_copy(&parts[0], barrel);
            equip_record_copy(&parts[1], stock);
            type = Get_pl_type(frame, parts);
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        *out = type;
    }
    return ret;
}

/* 0x803173C4 (0x3C4): Reports the shown bowgun part's deviation rate (frame or barrel, or the frame/barrel pair for
 * class 3/7); bits 1/2 compare it in hundredths with the worn part's, bits 3/4 flag a skill-changed value. */
u8 gun_deviation_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, f32* out_value) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    u8 ret = 0;
    u8 err = 0;
    f32 value = 0.0f;
    f32 worn_value = value;
    u8 kind = equip_compare_class(flags);

    switch (kind) {
    case 1:
    case 2:
        plw = (kind == 1) ? plw : NULL;
        if (frame->kind == 0xB) {
            if (worn_frame->kind == 0xB) {
                worn_value = pl_gun_deviation_get_c1(plw, worn_frame, NULL, &err);
                err = 0;
            }
            value = pl_gun_deviation_get_c1(plw, frame, NULL, &err);
            ret |= 1;
        } else if (frame->kind == 0xC) {
            if (worn_frame->kind == 0xC) {
                worn_value = pl_gun_deviation_get_c1(plw, NULL, worn_frame, &err);
                err = 0;
            }
            value = pl_gun_deviation_get_c1(plw, NULL, frame, &err);
            ret |= 1;
        }
        if ((ret & 1) && (u8)(worn_frame->kind + 0xF5) <= 1) {
            if ((s32)(100.0f * worn_value) != (s32)(100.0f * value)) {
                ret |= ((s32)(100.0f * worn_value) < (s32)(100.0f * value)) ? 2 : 4;
            }
        }
        break;
    case 3:
    case 4:
        plw = (kind == 3) ? plw : NULL;
        if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC) {
            worn_value = pl_gun_deviation_get_c1(plw, worn_frame, worn_barrel, &err);
            err = 0;
        }
        value = pl_gun_deviation_get_c1(plw, frame, barrel, &err);
        ret |= 1;
        if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC) {
            if ((s32)(100.0f * worn_value) != (s32)(100.0f * value)) {
                ret |= ((s32)(100.0f * worn_value) < (s32)(100.0f * value)) ? 2 : 4;
            }
        }
        break;
    case 5:
    case 6:
        plw = (kind == 5) ? plw : NULL;
        if (frame->kind == 0xB) {
            value = pl_gun_deviation_get_c1(plw, frame, NULL, &err);
            ret |= 1;
        } else if (frame->kind == 0xC) {
            value = pl_gun_deviation_get_c1(plw, NULL, frame, &err);
            ret |= 1;
        }
        break;
    case 7:
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            value = pl_gun_deviation_get_c1(kind == 7 ? plw : NULL, frame, barrel, &err);
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        if (flags & 4) {
            switch (err) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_value = value;
    }
    return ret;
}

/* 0x80317788 (0x3FC): Reports the shown bowgun part's reload rating (a frame's offset by 10, a stock's by 13, the
 * whole set's from the hunter's skills for class 3/4/7/8); bits 1/2 compare it with the worn one, bits 3/4 flag
 * a skill-changed value. */
u8 gun_reload_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_value) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    _EQUIP* worn_stock = view->worn[2];
    u8 ret = 0;
    u8 err = 0;
    u8 value = 0;
    u8 worn_value = 0;
    PlWeaponRec* rec;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if (frame->kind == 0xB || frame->kind == 0xD) {
            if (worn_frame->kind == 0xB || worn_frame->kind == 0xD) {
                rec = equip_weapon_rec_get_c1(worn_frame);
                if (rec != NULL) {
                    worn_value = rec->bonus_0x09;
                    if (worn_frame->kind == 0xB) {
                        worn_value += 10;
                    } else if (worn_frame->kind == 0xD) {
                        worn_value += 13;
                    }
                }
            } else {
                flags &= ~8;
            }
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                value = rec->bonus_0x09;
            }
            ret |= 1;
            if (frame->kind == 0xB) {
                value += 10;
            } else if (frame->kind == 0xD) {
                value += 13;
            }
            if ((flags & 8) && (flags & 1) && worn_value != value && frame->kind == worn_frame->kind) {
                ret |= (worn_value < value) ? 2 : 4;
            }
        }
        break;
    case 3:
        if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
            worn_value = pl_gun_reload_get_c1(plw, worn_frame, worn_stock, &err);
            err = 0;
        } else {
            flags &= ~8;
        }
        value = pl_gun_reload_get_c1(plw, frame, stock, &err);
        ret |= 1;
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        break;
    case 4:
        if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
            worn_value = pl_gun_reload_get_c1(NULL, worn_frame, worn_stock, &err);
            err = 0;
        } else {
            flags &= ~8;
        }
        value = pl_gun_reload_get_c1(NULL, frame, stock, &err);
        ret |= 1;
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        break;
    case 5:
    case 6:
        if (frame->kind == 0xB || frame->kind == 0xD) {
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                value = rec->bonus_0x09;
            }
            ret |= 1;
            if (frame->kind == 0xB) {
                value += 10;
            } else if (frame->kind == 0xD) {
                value += 13;
            }
        }
        break;
    case 7:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            value = pl_gun_reload_get_c1(plw, frame, stock, &err);
            ret |= 1;
        }
        break;
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            value = pl_gun_reload_get_c1(NULL, frame, stock, &err);
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        if (flags & 4) {
            switch (err) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_value = value;
    }
    return ret;
}


/* 0x80317B84 (0x410): Reports the shown bowgun part's recoil rating (a frame's offset by 6, a stock's by 10, the
 * whole set's from the hunter's skills for class 3/4/7/8); bits 1/2 compare it with the worn one, bits 3/4 flag a
 * skill-changed value. */
u8 gun_recoil_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s16* out_value) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    _EQUIP* worn_stock = view->worn[2];
    u8 ret = 0;
    u8 err = 0;
    s16 value = 0;
    s16 worn_value = 0;
    PlWeaponRec* rec;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if (frame->kind == 0xB || frame->kind == 0xD) {
            if (worn_frame->kind == 0xB || worn_frame->kind == 0xD) {
                rec = equip_weapon_rec_get_c1(worn_frame);
                if (rec != NULL) {
                    worn_value = (u8)rec->bonus_0x07;
                    if (worn_frame->kind == 0xB) {
                        worn_value = worn_value + 6;
                    } else if (worn_frame->kind == 0xD) {
                        worn_value = worn_value + 10;
                    }
                }
            } else {
                flags &= ~8;
            }
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                value = (u8)rec->bonus_0x07;
            }
            ret |= 1;
            if (frame->kind == 0xB) {
                value = value + 6;
            } else if (frame->kind == 0xD) {
                value += 10;
            }
            if ((flags & 8) && (flags & 1) && worn_value != value && frame->kind == worn_frame->kind) {
                ret |= (worn_value < value) ? 2 : 4;
            }
        }
        break;
    case 3:
        if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
            worn_value = pl_gun_recoil_get_c1(plw, worn_frame, worn_stock, &err);
            err = 0;
        } else {
            flags &= ~8;
        }
        value = pl_gun_recoil_get_c1(plw, frame, stock, &err);
        ret |= 1;
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        break;
    case 4:
        if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
            worn_value = pl_gun_recoil_get_c1(NULL, worn_frame, worn_stock, &err);
            err = 0;
        } else {
            flags &= ~8;
        }
        value = pl_gun_recoil_get_c1(NULL, frame, stock, &err);
        ret |= 1;
        if ((flags & 8) && (flags & 1) && worn_value != value) {
            ret |= (worn_value < value) ? 2 : 4;
        }
        break;
    case 5:
    case 6:
        if (frame->kind == 0xB || frame->kind == 0xD) {
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                value = (u8)rec->bonus_0x07;
            }
            ret |= 1;
            if (frame->kind == 0xB) {
                value = value + 6;
            } else if (frame->kind == 0xD) {
                value = value + 10;
            }
        }
        break;
    case 7:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            value = pl_gun_recoil_get_c1(plw, frame, stock, &err);
            ret |= 1;
        }
        break;
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            value = pl_gun_recoil_get_c1(NULL, frame, stock, &err);
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        if (flags & 4) {
            switch (err) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_value = value;
    }
    return ret;
}

/* 0x80317F94 (0x270): Rates a bowgun's frame/barrel pair for the recoil display: the level (0..2) from the frame's
 * row minus the barrel's, its icon, and in `*changed` 1/2 when the hunter's skills lowered/raised the level. */
void gun_recoil_level_get(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, u8* level, u8* icon, u8* changed) {
    s32 rating = 0;
    PlWeaponRec* rec;

    *level = 0;
    *icon = 0;
    *changed = 0;
    rec = equip_weapon_rec_get_c1(frame);
    if (rec != NULL) {
        *icon = gun_frame_recoil_icon[rec->recoil_kind];
        rating = gun_frame_recoil_rating[rec->recoil_kind];
    }
    rec = equip_weapon_rec_get_c1(barrel);
    if (rec != NULL) {
        rating = (s8)(rating - gun_barrel_recoil_rating[rec->recoil_kind]);
    }
    if ((s8)rating <= 5) {
        *level = 0;
        *icon = 0;
    } else if ((s8)rating < 16) {
        *level = 1;
        if (*icon == 0) {
            *icon = 3;
        }
    } else {
        *level = 2;
        if (*icon == 0) {
            *icon = 3;
        }
    }
    if (plw != NULL) {
        if (Pl_Skill_ck(plw, 0xAC) == 1) {
            if (Pl_cat_skill_ck(plw, 0x1E) == 1) {
                if (*level != 0) {
                    *level -= 1;
                    *changed = 1;
                }
            } else if (*level != 0) {
                *level = 0;
                *changed = 1;
            }
        } else if (Pl_Skill_ck(plw, 0xAB) == 1) {
            if (Pl_cat_skill_ck(plw, 0x1E) == 0) {
                if (*level != 0) {
                    *level -= 1;
                    *changed = 1;
                }
            }
        } else if (Pl_Skill_ck(plw, 0xAD) == 1) {
            if (Pl_cat_skill_ck(plw, 0x1E) == 1) {
                if (*level != 2) {
                    *level = 2;
                    *changed = 2;
                }
            } else if (*level < 2) {
                *level += 1;
                *changed = 2;
            }
        } else if (Pl_cat_skill_ck(plw, 0x1E) == 1) {
            if (*level < 2) {
                *level += 1;
                *changed = 2;
            }
        }
        if (*level != 0 && *icon == 0) {
            *icon = 3;
        }
    }
}


/* 0x80318204 - map a packed equipment kind to its two display indices; 0 and 1 are not
 * representable and yield 0. */
u8 gun_recoil_kind_split(u8 kind, u8* out1, u8* out2) {
    *out1 = 0;
    *out2 = 0;
    switch (kind) {
    case 0:
        *out1 = 0;
        break;
    case 1:
        *out1 = 1;
        *out2 = 1;
        break;
    case 2:
        *out1 = 2;
        *out2 = 1;
        break;
    case 3:
        *out1 = 1;
        *out2 = 2;
        break;
    case 4:
        *out1 = 2;
        *out2 = 2;
        break;
    case 5:
        *out1 = 1;
        *out2 = 3;
        break;
    case 6:
        *out1 = 2;
        *out2 = 3;
        break;
    default:
        return 0;
    }
    return 1;
}

/* 0x803182D8 (0x4B8): Prints the shown bowgun part's recoil text (a frame's level and icon, a barrel's row, the
 * frame/barrel pair's skill-adjusted level for class 3/7); bits 1/2 compare it with the worn part's, bits 3/4 flag a
 * skill-changed level. */
u8 gun_recoil_text_get(_PLW* plw, _EQUIP_INDEX* view, u8 flags, s8* out_text) {
    _EQUIP* frame;
    u8 ret;
    u8 icon;
    u8 level;
    u8 worn_level;
    _EQUIP* barrel;
    _EQUIP* stock;
    _EQUIP* worn_frame;
    _EQUIP* worn_barrel;
    _EQUIP* worn_stock;
    u8 err;
    u32 barrel_text;
    PlWeaponRec* rec;
    u8** levels;
    u8** icons;

    ret = 0;
    icon = 0;
    level = 0;
    worn_level = 0;
    frame = view->shown[0];
    barrel = view->shown[1];
    stock = view->shown[2];
    worn_frame = view->worn[0];
    worn_barrel = view->worn[1];
    worn_stock = view->worn[2];
    err = 0;
    barrel_text = 0;
    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if (frame->kind == 0xB) {
            if (worn_frame->kind == 0xB) {
                rec = equip_weapon_rec_get_c1(worn_frame);
                if (rec != NULL) {
                    gun_recoil_kind_split(rec->recoil_kind, &worn_level, &icon);
                }
            }
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                gun_recoil_kind_split(rec->recoil_kind, &level, &icon);
                ret |= 1;
            }
        } else if (frame->kind == 0xC) {
            if (worn_frame->kind == 0xC) {
                rec = equip_weapon_rec_get_c1(worn_frame);
                if (rec != NULL) {
                    worn_level = rec->recoil_kind;
                }
            }
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                level = rec->recoil_kind;
                barrel_text = 1;
                ret |= 1;
            }
        }
        if ((ret & 1) && (flags & 8) && (flags & 1)) {
            if (frame->kind == 0xB && worn_frame->kind == 0xB) {
                if (level != worn_level) {
                    ret |= (worn_level < level) ? 4 : 2;
                }
            } else if (frame->kind == 0xC && worn_frame->kind == 0xC && level != worn_level) {
                ret |= (gun_barrel_recoil_rating[worn_level] < gun_barrel_recoil_rating[level]) ? 2 : 4;
            }
        }
        break;
    case 3:
    case 4:
        plw = (flags & 2) ? plw : NULL;
        if (frame->kind == 0xB && barrel->kind == 0xC) {
            if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
                gun_recoil_level_get(plw, worn_frame, worn_barrel, &worn_level, &icon, &err);
            }
            gun_recoil_level_get(plw, frame, barrel, &level, &icon, &err);
            ret |= 1;
            if ((ret & 1) && (flags & 8) && (flags & 1) && worn_frame->kind == 0xB && worn_barrel->kind == 0xC &&
                worn_stock->kind == 0xD && level != worn_level) {
                ret |= (worn_level < level) ? 4 : 2;
            }
        }
        break;
    case 5:
    case 6:
        if (frame->kind == 0xB) {
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                gun_recoil_kind_split(rec->recoil_kind, &level, &icon);
                ret |= 1;
            }
        } else if (frame->kind == 0xC) {
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL) {
                level = rec->recoil_kind;
                barrel_text = 1;
                ret |= 1;
            }
        }
        break;
    case 7:
    case 8:
        plw = (flags & 2) ? plw : NULL;
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            gun_recoil_level_get(plw, frame, barrel, &level, &icon, &err);
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        if (barrel_text == 1) {
            sprintf(out_text, "%s", get_str_tbl(0x37)[level]);
        } else {
            levels = get_str_tbl(0x35);
            icons = get_str_tbl(0x36);
            if (level == 0) {
                sprintf(out_text, "%s", levels[level]);
            } else {
                sprintf(out_text, "%s%s%s", levels[level], levels[3], icons[icon]);
            }
        }
        if (flags & 4) {
            switch (err) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
    }
    return ret;
}

/* 0x803188AC (0x164): Reports the shown weapon's slot bonus (+0x06), a bowgun's summed over its parts. */
u8 gun_slot_bonus_get(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_value) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    u8 ret = 0;
    u8 value = 0;
    PlWeaponRec* rec;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
    case 5:
    case 6:
        if ((u8)(frame->kind + 0xF5) <= 2) {
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL && rec->bonus_0x06 != 0) {
                value = rec->bonus_0x06;
                ret |= 1;
            }
        }
        break;
    case 3:
    case 4:
    case 7:
    case 8:
        if (frame->kind == 0xB) {
            rec = equip_weapon_rec_get_c1(frame);
            if (rec != NULL && rec->bonus_0x06 != 0) {
                value = rec->bonus_0x06;
                ret |= 1;
            }
        }
        if (barrel->kind == 0xC) {
            rec = equip_weapon_rec_get_c1(barrel);
            if (rec != NULL && rec->bonus_0x06 != 0) {
                value += rec->bonus_0x06;
                ret |= 1;
            }
        }
        if (stock->kind == 0xD) {
            rec = equip_weapon_rec_get_c1(stock);
            if (rec != NULL && rec->bonus_0x06 != 0) {
                value += rec->bonus_0x06;
                ret |= 1;
            }
        }
        if (value != 0) {
            ret |= 1;
        }
        break;
    }
    *out_value = value;
    return ret;
}

/* 0x80318790 - true when the slot's class carries a filled "option" record: kinds 1/2/5/6 test
 * one item, 3/4/7/8 test the primary/secondary/tertiary items in turn. */
u8 gun_bonus_flag_ck(_PLW* plw, _EQUIP_INDEX* view, u8 sel) {
    _EQUIP* p = view->shown[0];
    _EQUIP* q = view->shown[1];
    _EQUIP* r = view->shown[2];
    u8 ret = 0;
    u8 kind = equip_compare_class(sel);
    switch (kind) {
    case 1:
    case 2:
    case 5:
    case 6:
        if ((u8)(p->kind + 0xF5) <= 2) {
            if (equip_gun_bonus_ck(p) == 1) {
                ret |= 1;
            }
        }
        break;
    case 3:
    case 4:
    case 7:
    case 8:
        if (p->kind == 0xB) {
            if (equip_gun_bonus_ck(p) == 1) {
                ret |= 1;
            }
        }
        if (q->kind == 0xC) {
            if (equip_gun_bonus_ck(q) == 1) {
                ret |= 1;
            }
        }
        if (r->kind == 0xD) {
            if (equip_gun_bonus_ck(r) == 1) {
                ret |= 1;
            }
        }
        break;
    default:
        break;
    }
    return ret;
}


/* 0x80318A10 (0x3F8): Reports the shown bowgun's clip size for one ammo kind (with the hunter's skills for class
 * 1/3/5/7, the bare parts' otherwise); bits 1/2 compare it with the worn gun's, bits 3/4 flag a skill-changed value. */
u8 gun_clip_compare(_PLW* plw, _EQUIP_INDEX* view, u8 ammo, u8 flags, u8* out_count) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    _EQUIP* worn_stock = view->worn[2];
    u8 ret = 0;
    u8 err = 0;
    u8 count = 0;
    u8 worn_count = 0;
    u32 has_worn = 0;

    *out_count = 0;
    switch (equip_compare_class(flags)) {
    case 1:
        if ((flags & 0x10) && (u8)(worn_frame->kind + 0xF5) <= 2) {
            worn_count = pl_gun_clip_get_c1(plw, ammo, worn_frame, NULL, NULL, &err);
            has_worn = 1;
        }
        count = pl_gun_clip_get_c1(plw, ammo, frame, NULL, NULL, &err);
        ret |= 1;
        break;
    case 2:
        if ((flags & 0x10) && (u8)(worn_frame->kind + 0xF5) <= 2) {
            worn_count = equip_gun_clip_get_c1(worn_frame, ammo);
            has_worn = 1;
        }
        count = equip_gun_clip_get_c1(frame, ammo);
        ret |= 1;
        break;
    case 3:
        if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
            worn_count = pl_gun_clip_get_c1(plw, ammo, worn_frame, worn_barrel, worn_stock, &err);
            has_worn = 1;
        }
        count = pl_gun_clip_get_c1(plw, ammo, frame, barrel, stock, &err);
        ret |= 1;
        break;
    case 4:
        if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
            worn_count = equip_gun_clip_get_c1(worn_frame, ammo);
            worn_count += equip_gun_clip_get_c1(worn_barrel, ammo);
            worn_count += equip_gun_clip_get_c1(worn_stock, ammo);
            has_worn = 1;
        }
        count = equip_gun_clip_get_c1(frame, ammo);
        count += equip_gun_clip_get_c1(barrel, ammo);
        count += equip_gun_clip_get_c1(stock, ammo);
        ret |= 1;
        break;
    case 5:
        if ((u8)(frame->kind + 0xF5) <= 2) {
            count = pl_gun_clip_get_c1(plw, ammo, frame, NULL, NULL, &err);
            ret |= 1;
        }
        break;
    case 6:
        if ((u8)(frame->kind + 0xF5) <= 2) {
            count = equip_gun_clip_get_c1(frame, ammo);
            ret |= 1;
        }
        break;
    case 7:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            count = pl_gun_clip_get_c1(plw, ammo, frame, barrel, stock, &err);
            ret |= 1;
        }
        break;
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            count = equip_gun_clip_get_c1(frame, ammo);
        count += equip_gun_clip_get_c1(barrel, ammo);
            count += equip_gun_clip_get_c1(stock, ammo);
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && has_worn == 1 && worn_count != count) {
            ret |= (worn_count < count) ? 2 : 4;
        }
        if (flags & 4) {
            switch (err) {
            case 1:
                ret |= 8;
                break;
            case 2:
                ret |= 0x10;
                break;
            }
        }
        *out_count = count;
    }
    return ret;
}

/* 0x80318E08 (0x1A8): Reports whether the shown bowgun can load one ammo kind (its frame, or its frame or barrel
 * for a whole set). */
u8 gun_ammo_usable_ck(_PLW* plw, _EQUIP_INDEX* view, u8 ammo, u8 flags) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    u8 ret = 0;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if ((u8)(frame->kind + 0xF5) <= 1) {
            if (gun_ammo_entry_find(equip_weapon_rec_get_c1(frame), ammo) != NULL) {
                ret |= 1;
            }
        }
        break;
    case 3:
    case 4:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            if (gun_ammo_entry_find(equip_weapon_rec_get_c1(frame), ammo) != NULL) {
                ret |= 1;
            }
            if (gun_ammo_entry_find(equip_weapon_rec_get_c1(barrel), ammo) != NULL) {
                ret |= 1;
            }
        }
        break;
    case 5:
    case 6:
        if ((u8)(frame->kind + 0xF5) <= 1) {
            if (gun_ammo_entry_find(equip_weapon_rec_get_c1(frame), ammo) != NULL) {
                ret |= 1;
            }
        }
        break;
    case 7:
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            if (gun_ammo_entry_find(equip_weapon_rec_get_c1(frame), ammo) != NULL) {
                ret |= 1;
            }
            if (gun_ammo_entry_find(equip_weapon_rec_get_c1(barrel), ammo) != NULL) {
                ret |= 1;
            }
        }
        break;
    }
    return ret;
}


/* 0x80318FB0 (0x1C8): Reports the shown bowgun frame's upgrade level (one-based; bit 6 at its cap); bits 1/2 compare
 * it with the worn frame's. */
u8 gun_frame_level_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* out_level) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* worn_frame = view->worn[0];
    u8 ret = 0;
    u8 worn_level = 0;
    u8 level = 0;
    u32 has_worn = 0;

    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        if (frame->kind == 0xB) {
            if (worn_frame->kind == 0xB) {
                worn_level = worn_frame->deco_count;
                has_worn = 1;
            }
            level = frame->deco_count;
            if (level == gun_frame_level_cap_get(frame)) {
                ret |= 0x40;
            }
            ret |= 1;
        }
        break;
    case 3:
    case 4:
        if (frame->kind == 0xB) {
            if (worn_frame->kind == 0xB) {
                worn_level = worn_frame->deco_count;
                has_worn = 1;
            }
            level = frame->deco_count;
            if (level == gun_frame_level_cap_get(frame)) {
                ret |= 0x40;
            }
            ret |= 1;
        }
        break;
    case 5:
    case 6:
        if (frame->kind == 0xB) {
            level = frame->deco_count;
            if (level == gun_frame_level_cap_get(frame)) {
                ret |= 0x40;
            }
            ret |= 1;
        }
        break;
    case 7:
    case 8:
        if (frame->kind == 0xB) {
            level = frame->deco_count;
            if (level == gun_frame_level_cap_get(frame)) {
                ret |= 0x40;
            }
            ret |= 1;
        }
        break;
    }
    if (ret & 1) {
        if ((flags & 8) && (flags & 1) && has_worn == 1 && worn_level != level) {
            ret |= (worn_level < level) ? 2 : 4;
        }
        *out_level = level + 1;
    }
    return ret;
}

/* 0x80319178 - pack the active and the next non-empty slot id (and their two colour bytes) into
 * the caller's arrays; returns how many were written. */
u8 equip_deco_skills_get(_EQUIP* info, u16* ids, s8* colors) {
    u8 idx;
    u8 n;
    n = 0;
    idx = info->deco_count;
    if (idx < 3) {
        if (info->skill_id[idx] != 0) {
            ids[0] = info->skill_id[idx];
            colors[0] = (s8)((u8)info->deco_level - 0xA);
            n = 1;
        }
    }
    idx++;
    if (idx < 3) {
        u16 v = info->skill_id[idx];
        if (v != 0) {
            ids[n] = v;
            colors[n] = (s8)(((info->deco_level >> 8) & 0xFF) - 0xA);
            n++;
        }
    }
    return n;
}

/* 0x80319204 (0x298): Lists the shown piece's two active decoration skills (id, level, and in `marks` bit 0 a
 * skill, bit 5 one the hunter's class cannot use, bits 1/2/3 its level against the worn piece's or a new skill);
 * returns how many it has. */
u8 equip_deco_skills_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* ids, s8* levels, u8* marks) {
    _EQUIP* worn = view->worn[0];
    _EQUIP* shown = view->shown[0];
    u16 shown_ids[2];
    s8 shown_levels[2];
    u16 worn_ids[2];
    s8 worn_levels[2];
    s32 i;
    u8 count;
    s32 found;

    for (i = 0; i < 2; i++) {
        ids[i] = 0;
        levels[i] = 0;
        marks[i] = 0;
        shown_ids[i] = 0;
        shown_levels[i] = 0;
        worn_ids[i] = 0;
        worn_levels[i] = 0;
    }
    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        equip_deco_skills_get(worn, worn_ids, worn_levels);
        count = equip_deco_skills_get(shown, shown_ids, shown_levels);
        for (i = 0; i < 2; i++) {
            if (shown_ids[i] != 0) {
                ids[i] = shown_ids[i];
                levels[i] = shown_levels[i];
                marks[i] |= 1;
                if (pl_skill_valid_ck(plw, shown_ids[i]) == 0) {
                    marks[i] |= 0x20;
                }
                if ((flags & 8) && (flags & 1)) {
                    found = 0;
                    if (worn_ids[0] != 0 && shown_ids[i] == worn_ids[0]) {
                        if (shown_levels[i] != worn_levels[0]) {
                            marks[i] |= (worn_levels[0] < shown_levels[i]) ? 2 : 4;
                        }
                        found = 1;
                    }
                    if (worn_ids[1] != 0 && shown_ids[i] == worn_ids[1]) {
                        if (shown_levels[i] != worn_levels[1]) {
                            marks[i] |= (worn_levels[1] < shown_levels[i]) ? 2 : 4;
                        }
                        found = 1;
                    }
                    if (found == 0) {
                        marks[i] |= 8;
                    }
                }
            }
        }
        break;
    case 5:
    case 6:
        count = equip_deco_skills_get(shown, shown_ids, shown_levels);
        for (i = 0; i < 2; i++) {
            if (shown_ids[i] != 0) {
                ids[i] = shown_ids[i];
                levels[i] = shown_levels[i];
                marks[i] |= 1;
                if (pl_skill_valid_ck(plw, shown_ids[i]) == 0) {
                    marks[i] |= 0x20;
                }
            }
        }
        break;
    }
    return count;
}

/* 0x8031949C (0x4B0): Lists every skill the shown piece (or the whole bowgun set) adds points to: the skill id, its
 * points, the points its decorations give, and in `marks` bit 0 a skill, bits 1/2/3 its points against the worn
 * piece's or a new skill, bit 5 one the hunter's class cannot use; returns how many it lists. */
u8 equip_skill_trees_list(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u8* skills, s8* points, s8* deco_points, u8* marks) {
    _EQUIP* frame = view->shown[0];
    _EQUIP* barrel = view->shown[1];
    _EQUIP* stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    _EQUIP* worn_stock = view->worn[2];
    u8 count = 0;
    u16 decos[5];
    s32 i;
    s32 skill;
    s32 j;
    u8 deco_skill;
    s8 value;
    s8 worn_value;

    for (i = 0; i < 10; i++) {
        skills[i] = 0;
        points[i] = 0;
        deco_points[i] = 0;
        marks[i] = 0;
    }
    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
    case 5:
    case 6:
        equip_decos_sort(frame, NULL, decos);
        count = 0;
        for (skill = 0; skill < 0x55; skill++) {
            value = equip_skill_points_get(frame, skill);
            if (value != 0) {
                skills[count] = skill;
                marks[count] |= 1;
                if ((flags & 8) && (flags & 1)) {
                    worn_value = equip_skill_points_get(worn_frame, skill);
                    if (worn_value != 0) {
                        if (worn_value != value) {
                            marks[count] |= (worn_value < value) ? 2 : 4;
                        }
                    } else {
                        marks[count] |= 8;
                    }
                }
                if (pl_equip_skill_valid_ck(plw, frame, barrel, skill) == 0) {
                    marks[count] |= 0x20;
                }
                points[count] = value;
                for (j = 0; j < 5; j++) {
                    deco_skill = deco_skill_id_get(decos[j], 0);
                    if (deco_skill != 0 && skill == deco_skill) {
                        deco_points[count] += deco_skill_points_get(decos[j], 0);
                    }
                    deco_skill = deco_skill_id_get(decos[j], 1);
                    if (deco_skill != 0 && skill == deco_skill) {
                        deco_points[count] += deco_skill_points_get(decos[j], 1);
                    }
                }
                count++;
            }
        }
        break;
    case 3:
    case 4:
    case 7:
    case 8:
        if (frame->kind == 0xB && barrel->kind == 0xC && stock->kind == 0xD) {
            equip_decos_sort(barrel, stock, decos);
            count = 0;
            for (skill = 0; skill < 0x55; skill++) {
                value = equip_skill_points_get(frame, skill);
                value += equip_skill_points_get(barrel, skill);
                value += equip_skill_points_get(stock, skill);
                if (value != 0) {
                    skills[count] = skill;
                    marks[count] |= 1;
                    if ((flags & 8) && (flags & 1) && worn_frame->kind == 0xB && worn_barrel->kind == 0xC &&
                        worn_stock->kind == 0xD) {
                        worn_value = equip_skill_points_get(worn_frame, skill);
                        worn_value += equip_skill_points_get(worn_barrel, skill);
                        worn_value += equip_skill_points_get(worn_stock, skill);
                        if (worn_value != 0 && worn_value != value) {
                            marks[count] |= (worn_value < value) ? 2 : 4;
                        }
                    }
                    if (pl_equip_skill_valid_ck(plw, frame, barrel, skill) == 0) {
                        marks[count] |= 0x20;
                    }
                    points[count] = value;
                    for (j = 0; j < 5; j++) {
                        deco_skill = deco_skill_id_get(decos[j], 0);
                        if (deco_skill != 0 && skill == deco_skill) {
                            deco_points[count] += deco_skill_points_get(decos[j], 0);
                        }
                        deco_skill = deco_skill_id_get(decos[j], 1);
                        if (deco_skill != 0 && skill == deco_skill) {
                            deco_points[count] += deco_skill_points_get(decos[j], 1);
                        }
                    }
                    count++;
                }
            }
        }
        break;
    }
    return count;
}

/* 0x8031994C (0x5CC): Lists the shown piece's five slot ids into `ids` (bit 0 of `marks` per filled slot); for the
 * classes compared against the worn set, bit 3 marks an id the worn set does not carry.  Returns the slot count. */
u8 equip_decos_compare(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* ids, u8* marks) {
    _EQUIP* shown_frame = view->shown[0];
    _EQUIP* shown_barrel = view->shown[1];
    _EQUIP* shown_stock = view->shown[2];
    _EQUIP* worn_frame = view->worn[0];
    _EQUIP* worn_barrel = view->worn[1];
    _EQUIP* worn_stock = view->worn[2];
    u8 ret = 0;
    u16 worn[5];
    u16 shown[5];
    u32 i;
    u32 j;
    u8 used;
    u32 found;

    for (i = 0; i < 5; i++) {
        ids[i] = 0;
        marks[i] = 0;
        shown[i] = 0;
        worn[i] = 0;
    }
    switch (equip_compare_class(flags)) {
    case 1:
    case 2:
        equip_decos_sort(worn_frame, NULL, worn);
        ret = equip_decos_sort(shown_frame, NULL, shown);
        used = 0;
        for (i = 0; i < 5; i++) {
            if (shown[i] != 0) {
                ids[i] = shown[i];
                marks[i] |= 1;
                if ((flags & 8) && (flags & 1)) {
                    found = 0;
                    for (j = 0; j < 5; j++) {
                        if (shown[i] == worn[j] && !(used & (1 << j))) {
                            used |= 1 << j;
                            found = 1;
                        }
                    }
                    if (found == 0) {
                        marks[i] |= 8;
                    }
                }
            }
        }
        break;
    case 3:
    case 4:
        if (shown_frame->kind == 0xB && shown_barrel->kind == 0xC && shown_stock->kind == 0xD) {
            if (worn_frame->kind == 0xB && worn_barrel->kind == 0xC && worn_stock->kind == 0xD) {
                equip_decos_sort(worn_barrel, worn_stock, worn);
            } else {
                equip_decos_sort(worn_frame, NULL, worn);
            }
            ret = equip_decos_sort(shown_barrel, shown_stock, shown);
            used = 0;
            for (i = 0; i < 5; i++) {
                if (shown[i] != 0) {
                    ids[i] = shown[i];
                    marks[i] |= 1;
                    if ((flags & 8) && (flags & 1)) {
                        found = 0;
                        for (j = 0; j < 5; j++) {
                            if (shown[i] == worn[j] && !(used & (1 << j))) {
                                used |= 1 << j;
                                found = 1;
                            }
                        }
                        if (found == 0) {
                            marks[i] |= 8;
                        }
                    }
                }
            }
        }
        break;
    case 5:
    case 6:
        ret = equip_decos_sort(shown_frame, NULL, shown);
        for (i = 0; i < 5; i++) {
            if (shown[i] != 0) {
                ids[i] = shown[i];
                marks[i] |= 1;
            }
        }
        break;
    case 7:
    case 8:
        if (shown_frame->kind == 0xB && shown_barrel->kind == 0xC && shown_stock->kind == 0xD) {
            ret = equip_decos_sort(shown_barrel, shown_stock, shown);
            for (i = 0; i < 5; i++) {
                if (shown[i] != 0) {
                    ids[i] = shown[i];
                    marks[i] |= 1;
                }
            }
        }
        break;
    }
    return ret;
}

/* 0x80319F18 (0x32C): Lists the skills the hunter would have with the shown equipment (flag bit 0) or has now: the
 * skill ids, and in `marks` bit 3 a new skill, bit 4 a negative one, bit 5 one the hunter's class cannot use;
 * returns how many it lists. */
u8 active_skills_list(_PLW* plw, _EQUIP_INDEX* view, u8 flags, u16* ids, u8* marks) {
    s32 i;
    u8 count = 0;
    s32 k;
    s32 found;
    u16 skill_ids[8];
    s8 skill_points[8];
    u8 skill_trees[8];
    _EQUIP columns[9];

    if (flags & 1) {
        for (i = 0; i < 8; i++) {
            ids[i] = 0;
            marks[i] = 0;
            skill_ids[i] = 0;
            skill_points[i] = 0;
            skill_trees[i] = 0;
        }
        Set_equip_column_arrangement(plw, view, columns);
        equip_columns_gun_view_set(plw, view, columns);
        equip_columns_skills_get(columns, skill_ids, skill_points, skill_trees);
        for (i = 0; i < 8; i++) {
            if (skill_ids[i] != 0) {
                ids[i] = skill_ids[i];
                count++;
                if (equip_columns_tree_points_get(columns, skill_trees[i]) < 0) {
                    marks[i] |= 0x10;
                }
                if (pl_equip_skill_valid_ck(plw, &columns[6], &columns[7], skill_trees[i]) == 0) {
                    marks[i] |= 0x20;
                }
                found = 0;
                for (k = 0; k < 8; k++) {
                    if (plw->active_skill_id[k] != 0 && plw->active_skill_tree[k] != 0 &&
                        plw->active_skill_id[k] == ids[i]) {
                        found = 1;
                    }
                }
                if (found == 0) {
                    marks[i] |= 8;
                }
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            ids[i] = 0;
            marks[i] = 0;
            if (plw->active_skill_id[i] != 0 && plw->active_skill_tree[i] != 0) {
                ids[i] = plw->active_skill_id[i];
                if (pl_skill_tree_points_get(plw, plw->active_skill_tree[i]) < 0) {
                    marks[i] |= 0x10;
                }
                if (pl_skill_id_valid_ck(plw, plw->active_skill_id[i]) == 0) {
                    marks[i] |= 0x20;
                }
                count++;
            }
        }
    }
    return count;
}

/* 0x8031A244 (0x1CC): Copies the hunter's nine worn pieces into the column array, then overwrites the column the
 * view's first shown piece belongs to (the bowgun's barrel and stock too); returns 0 for an unknown kind. */
u8 Set_equip_column_arrangement(_PLW* plw, _EQUIP_INDEX* index, _EQUIP* equip) {
    equip_record_copy(&equip[0], &plw->equipA[0]);
    equip_record_copy(&equip[1], &plw->equipA[1]);
    equip_record_copy(&equip[2], &plw->equipA[2]);
    equip_record_copy(&equip[3], &plw->equipA[3]);
    equip_record_copy(&equip[4], &plw->equipA[4]);
    equip_record_copy(&equip[5], &plw->equipA[5]);
    equip_record_copy(&equip[6], &plw->equipB);
    equip_record_copy(&equip[7], &plw->equipC);
    equip_record_copy(&equip[8], &plw->equipD);
    if (index == NULL) {
        return 1;
    }
    _EQUIP* frame = index->shown[0];
    _EQUIP* barrel = index->shown[1];
    _EQUIP* stock = index->shown[2];
    switch (frame->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        equip_record_copy(&equip[6], frame);
        break;
    case 11:
        equip_record_copy(&equip[6], frame);
        if (barrel != NULL) {
            if (barrel->kind == 0xC) {
                equip_record_copy(&equip[7], barrel);
            }
            if (stock != NULL && stock->kind == 0xD) {
                equip_record_copy(&equip[8], stock);
            }
        }
        break;
    case 12:
        equip_record_copy(&equip[7], frame);
        break;
    case 13:
        equip_record_copy(&equip[8], frame);
        break;
    case 1:
        equip_record_copy(&equip[0], frame);
        break;
    case 2:
        equip_record_copy(&equip[1], frame);
        break;
    case 3:
        equip_record_copy(&equip[2], frame);
        break;
    case 4:
        equip_record_copy(&equip[3], frame);
        break;
    case 5:
        equip_record_copy(&equip[4], frame);
        break;
    case 6:
        equip_record_copy(&equip[5], frame);
        break;
    default:
#line 10920
        nw4r::db::Panic("menu_infomation.cpp", __LINE__, "NW4R:Failed assertion 0");
        return 0;
    }
    return 1;
}

/* 0x8031A410 (0x18): Unpacks the view's three shown pieces and tail-calls the bowgun column update. */
void equip_columns_gun_view_set(_PLW* plw, _EQUIP_INDEX* view, _EQUIP* columns) {
    equip_columns_gun_set(plw, view->shown[0], view->shown[1], view->shown[2], columns);
}

/* 0x8031A428 (0x164): Puts a ranged frame into the bowgun columns: a frame whose options are off takes the frame
 * column and clears the barrel/stock columns to the frame's id; otherwise the parts follow the worn frame. */
void equip_columns_gun_set(_PLW* plw, _EQUIP* frame, _EQUIP* barrel, _EQUIP* stock, _EQUIP* columns) {
    if ((u32)(frame->kind - 1) <= 5) {
        return;
    }
    if (frame->kind == 0xB && Gunner_opt_ok_ck(frame) == 0) {
        equip_record_copy(&columns[6], frame);
        columns[7].item_id = frame->item_id;
        columns[7].deco_count = 0;
        columns[7].deco_level = 0;
        columns[7].skill_id[0] = 0;
        columns[7].skill_id[1] = 0;
        columns[7].skill_id[2] = 0;
        columns[8].item_id = frame->item_id;
        columns[8].deco_count = 0;
        columns[8].deco_level = 0;
        columns[8].skill_id[0] = 0;
        columns[8].skill_id[1] = 0;
        columns[8].skill_id[2] = 0;
    } else if (plw->equipB.kind == 0xB) {
        if ((u8)(frame->kind + 0xF5) > 2) {
            columns[7].kind = 0;
            columns[7].item_id = 0;
            columns[8].kind = 0;
            columns[8].item_id = 0;
        } else if (Gunner_opt_ok_ck(&plw->equipB) == 0) {
            equip_record_copy(&columns[6], frame);
            columns[7].kind = 0;
            columns[7].item_id = 0;
            columns[8].kind = 0;
            columns[8].item_id = 0;
        }
    } else if (barrel == NULL && stock == NULL && (u8)(frame->kind + 0xF5) <= 2) {
        equip_record_copy(&columns[6], frame);
        columns[7].kind = 0;
        columns[7].item_id = 0;
        columns[8].kind = 0;
        columns[8].item_id = 0;
    }
}

/* 0x8031A58C - resets a menu slot into its "equipment panel" state: the row height from its +0x10 flag, the panel's
 * own fields, and the embedded +0x1EC record handed to its constructor. */
void equip_panel_slot_reset(MenuSlot* self) {
    u8 size = 0x18;
    if (self->field_0x010 != 0) {
        size = 0x20;
    }
    self->field_0x014 = 1;
    self->field_0x23A = 0;
    self->field_0x1B0 = 0;
    self->field_0x19E = 0;
    self->entry_count_b = 8;
    self->field_0x1A3 = 8;
    self->field_0x1A0 = 0;
    s16 h = menu_page_count(size, 8);
    self->field_0x1A1 = (s8)h;
    self->field_0x1A2 = 0;
    menu_slot_entry_b_select(self, 0);
    menu_equip_entries_update(self);
    menu_sel_cursor_init(&self->field_0x1EC[0], 0, self);
}

/* 0x8031A638 - when a menu slot is a ready bowgun/gunner panel, refresh both of its option rows. */
void menu_slot_items_return(MenuSlot* self) {
    s8 st = self->field_0x014;
    if ((u32)(st - 1) <= 1) {
        if (self->field_0x001 == 3) {
            if (GameMode_ck() != 2) {
                if (self->field_0x1F0 != 0) {
                    pl_item_add(self->worker, self->field_0x1F0, 1);
                }
                if (self->field_0x1F2 != 0) {
                    pl_item_add(self->worker, self->field_0x1F2, 1);
                }
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The unwritten bodies, in address order.  Each notes the shape its target's codegen proves where
 * that shape is not obvious from the call list.
 * --------------------------------------------------------------------------------------------------- */

/* 0x80308FB4 - the equip-information screen's per-frame dispatch on its page mode: builds the two sub-screens for
 * modes 0 and 1, or refreshes the detail page for modes 2..10 with the mode as the page delta. */
void equip_info_update(StatusScreenWork* self) {
    u16* table;
    _mh_ivec2_ pos;

    s8 mode = self->field_0x1AE;

    switch (mode) {
    case 0:
        put_equip_info_upper_page(self);
        break;
    case 1:
        put_equip_info_piece_page(self);
        get_lsp_data(1482, &pos);
        draw_sprite_idx(1497, &pos);
        get_lsp_data(1322, &pos);
        table = (u16*)get_menu_lsp_tbl(0);
        PutPageArrow(table, self->field_0x1AE, self->field_0x1AF, self->pad_repeat, &pos, 0);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        equip_page_count_step(self->equip, 0, 0, (u8)mode - 2);
        equip_detail_page_refresh(self->equip, 0, self->field_0x1AE, self->field_0x1AF,
                    self->pad_repeat);
        break;
    }
}

/* 0x8030FAC0 - draw the sprite `id` at the 0x915 frame, offset from the 0x914 anchor (or from zero
 * when the id is the anchor itself). */
void put_lsp_sprite_offset(u16 id) {
    _mh_ivec2_ pos;
    _mh_ivec2_ d;

    if (id != 0x914) {
        get_lsp_data(0x914, &pos);
        get_lsp_data(id, &d);
        d.x -= pos.x;
        d.y -= pos.y;
    } else {
        d.x = 0;
        d.y = 0;
    }
    u32 tmp = *(u32*)&d;
    put_lsp_anchor_offset(0x914, &pos, &tmp);
    draw_sprite_idx(0x915, &pos);
}

/* 0x80311A88 - draw the sprite row `id` at the 0x917 anchor's offset when the id is set, or the
 * caller's packed position otherwise. */
void put_lsp_sprite_at_anchor(u32* p, u8 id) {
    _mh_ivec2_ pos;
    u32 v;

    if (id == 0) {
        v = *p;
        put_lsp_anchor_offset(0x917, &pos, &v);
    } else {
        get_lsp_data(id, &pos);
    }
    draw_sprite_idx(0x918, &pos);
}

/* 0x80312F18 - draw the 0xC7 and 0x89 sprite runs at the 0x955 and 0x966 anchors, each offset by the
 * caller's position. */
void put_lsp_sprite_runs(u32* p) {
    _mh_ivec2_ pos;
    _mh_ivec2_ pos2;
    u32 v = *p;
    u32 v2;

    put_lsp_anchor_offset(0x955, &pos, &v);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0xC7), &pos);
    v2 = *(u32*)&pos;
    put_lsp_anchor_offset(0x966, &pos2, &v2);
    draw_sprite_ary((u16*)get_menu_lsp_tbl(0x89), &pos2);
}

/* 0x803115E0 - the `put_equip_panel_row_ex` panel's six-register resolve entry. */
void put_equip_panel_row_variant(_PLW* s0, _EQUIP* s1, _EQUIP* s2, _EQUIP* s3, u16 a, u8 b, u8 c) {
    _EQUIP_INDEX out;

    if (equip_variant_resolve_ex(s0, s1, s2, s3, b, &out) == 1) {
        return;
    }
    put_equip_panel_row_ex(s0, &out, a, b, c);
}

/* 0x80311840 - the `put_equip_panel_row_base` panel's entry whose resolve word is forced to zero and whose flag
 * byte keeps only its low three bits. */
void put_equip_panel_row_zero(_PLW* s0, _EQUIP* s1, u16 a, u8 b) {
    _EQUIP_INDEX out;
    u8 flag = b;

    if (equip_variant_resolve(s0, s1, 0, &flag, &out) == 1) {
        return;
    }
    flag = (u8)(flag & ~6);
    put_equip_panel_row_base(s0, &out, a, flag);
}

/* 0x803118B4 - the `put_equip_panel_row_base` panel's six-register resolve entry. */
void put_equip_panel_row(_PLW* s0, _EQUIP* s1, _EQUIP* s2, _EQUIP* s3, u16 a, u8 b) {
    _EQUIP_INDEX out;

    if (equip_variant_resolve_ex(s0, s1, s2, s3, b, &out) == 1) {
        return;
    }
    put_equip_panel_row_base(s0, &out, a, b);
}

/* The equipment panel's column layout: for each of the two panel kinds, eleven rows of three column ids
 * (0xFF = no column) that `ammo_table_draw` draws.  size: 0x42 */
u8 equip_panel_row_layout[22][3] = {
    { 0, 1, 2 },
    { 3, 4, 5 },
    { 6, 7, 8 },
    { 9, 10, 11 },
    { 12, 13, 14 },
    { 39, 40, 41 },
    { 20, 21, 0xFF },
    { 22, 23, 0xFF },
    { 24, 25, 0xFF },
    { 26, 27, 0xFF },
    { 34, 35, 0xFF },
    { 30, 31, 0xFF },
    { 32, 33, 0xFF },
    { 36, 0xFF, 0xFF },
    { 15, 0xFF, 0xFF },
    { 16, 0xFF, 0xFF },
    { 18, 0xFF, 0xFF },
    { 17, 0xFF, 0xFF },
    { 19, 0xFF, 0xFF },
    { 38, 0xFF, 0xFF },
    { 28, 0xFF, 0xFF },
    { 29, 0xFF, 0xFF },
};
