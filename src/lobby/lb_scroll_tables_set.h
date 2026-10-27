/* Leaf header (docs/plan.md 6.5 rule 2): the smithy helpers `lobby/fn_80212810.cpp` defines and
 * `lobby/lb_screen_step.cpp` calls; the owner has no header of its own.  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_LB_SCROLL_TABLES_SET_H
#define MHTRI_LOBBY_LB_SCROLL_TABLES_SET_H

#include "types.h"
#include "lobby/lb_list.h"

struct _EQUIP;
struct LbScrollWork;
struct _mh_ivec2_;
struct _SPR_DATA_;
struct LbSmithRecipe;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80212FFC - the index of the next make row of kind `kind` after `idx` (-1: the first) in sort order `order`, or -1. */
s32 lb_seisan_row_next(u8 kind, u8 order, s32 idx, struct LbSmithRecipe* table);
/* 0x802130BC - the same walk over a gun-part table (0x24-byte rows). */
s32 lb_seisan_gun_row_next(u8 kind, u8 order, s32 idx, struct LbSmithRecipe* table);
/* 0x80213870 - the refusal bits of make row `idx` of kind `kind` (2 materials, 4 money, 0x10 locked). */
u16 lb_seisan_row_flags_get(u16 kind, u16 idx, u8 level);
/* 0x80217DA0 - the make price of item `item` of kind `kind`. */
s32 lb_seisan_price_get(u8 kind, u16 item);
/* 0x802180D8 - the group of equipment kind `kind`: 1 blade, 2 gun, 3 gun part, 4 other, 0 none. */
u8 lb_equip_kind_group_get(u8 kind);
/* 0x80213D70 - fills `out` with the four materials item `item` of kind `kind` needs at level `level`. */
void lb_seisan_materials_get(u8 kind, u16 item, u8 level, LbItemCount* out);
/* 0x80213A70 - the refusal bits of decoration recipe `idx` (2 materials, 4 money). */
u16 lb_deco_row_flags_get(u16 idx);
/* 0x80212A2C - whether a decoration of species `species` passes the slot-size filter `filter`. */
u32 lb_deco_class_ck(u8 filter, u16 species);
/* 0x80213B38 - fills `out` with the materials armour `item` of kind `kind` needs to reach level `level`. */
void lb_kakou_materials_get(u8 kind, u16 item, u8 level, LbItemCount* out);
/* 0x80213FB4 - the refusal bits of upgrading gun part `item` of kind `kind` at level `level`. */
u16 lb_kakou_gun_flags_get(u8 kind, u16 item, u8 level);
/* 0x80214134 - the refusal bits of upgrading the box piece `equip` (item `item` of kind `kind`). */
u16 lb_kakou_weapon_flags_get(u8 kind, u16 item, struct _EQUIP* equip);
/* 0x80213CA0 - the refusal bits of upgrading armour `item` of kind `kind` at level `level`. */
u16 lb_kakou_armor_flags_get(u8 kind, u16 item, u8 level);
/* 0x802190FC - copies the compared and previewed equipment records into the scroll list. */
void lb_scroll_tables_set(struct LbScrollWork* scroll, struct _EQUIP* compare, struct _EQUIP* compare_sub,
                          struct _EQUIP* base, struct _EQUIP* base_sub);
/* 0x802190F4 - sets the scroll list's flag byte. */
void lb_scroll_flag_set(struct LbScrollWork* scroll, u8 flag);
/* 0x80219254 - points the scroll list at `count` material rows. */
void lb_scroll_materials_set(struct LbScrollWork* scroll, LbItemCount* rows, u16 count);
/* 0x8021920C - sets the scroll list's item and rebuilds its rows. */
void lb_scroll_item_set(struct LbScrollWork* scroll, u16 item);
/* 0x80217F4C - fills the equipment record `row` with kind `kind` and item `item`. */
s32 lb_row_init(struct _EQUIP* row, u8 kind, u16 item);
/* 0x802174E0 - draws the item detail window of item `item` at `pos`. */
void lb_item_window_draw(u16 item, struct _mh_ivec2_* pos);
/* 0x80216608 - draws an item cell in layout 5. */
void lb_item_cell_draw_layout5(u16 item, s32 count, struct _mh_ivec2_* pos, s32 enabled, s32 picked, s32 active,
                               u8 mode);
/* 0x80216560 - draws an item cell in layout 1. */
void lb_item_cell_draw_layout1(u16 item, s32 count, struct _mh_ivec2_* pos, s32 enabled, s32 picked, s32 active,
                               u8 mode);
/* 0x80212810 - the label of sort filter `filter`. */
s8* lb_sort_filter_name_get(u8 filter);
/* 0x80215310 - prints the number `value` with the sprite record `spr` at `pos`. */
void lb_sprite_num_draw(struct _SPR_DATA_* spr, s8 value, u32 flags, struct _mh_ivec2_* pos);
/* 0x8021555C - prints `text` centred in panel `panel`. */
void lb_text_center_print(u16 panel, s8* text, s16 flag, s16 size, struct _mh_ivec2_* pos, u8 mode);
/* 0x802152A4 - prints the number `value` in panel `panel`. */
void draw_font_idx_num(u16 panel, u8 value, u32 flags, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCROLL_TABLES_SET_H */
