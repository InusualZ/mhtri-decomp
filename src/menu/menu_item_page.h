/*
 * menu/menu_item_page.h - `menu/menu_item_page.cpp`'s entry points, records and callees.  The callees are declared
 *   here or come from `unsplit/` because the owner headers collide in one TU: `hud/layout.h` and `lobby/lb_pane_ui.h`
 *   define `_mh_ivec2_`/`_SPR_DATA_` differently from `unsplit/lobby.h`, and `menu/menu_item.h` pulls `pl.h`.
 */
#ifndef MHTRI_MENU_MENU_ITEM_PAGE_H
#define MHTRI_MENU_MENU_ITEM_PAGE_H

#include "types.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The 0x10-byte row record `fn_8004EA24` hands back, indexed by the scroll cursor: the item/equipment
 * fields the page-0 detail panel renders.  Its +0 word is the kind the panel switches on, and the
 * pair at +0x04/+0x06 is passed to `GetEquipName` (a `u16` read of +0x04 narrows to the `u8` it takes). */
typedef struct MenuRowData {
    /* +0x00 */ s32 kind;
    /* +0x04 */ u16 equip_kind;
    /* +0x06 */ u16 equip_id;
    /* +0x08 */ u16 item_id;
    /* +0x0A */ u16 field_0x0A;
    /* +0x0C */ u16 field_0x0C;
    /* +0x0E */ u8 unused_0x0E[0x10 - 0x0E];
} MenuRowData; /* size: 0x10 */

/* The 4-byte row record the item list's data table (`fn_8029F808()`) holds, indexed by
 * `MenuEntry::field_0x06`: the kind the row's damage/quality block switches on (+0) and the row id
 * `get_menu_lsp_tbl(0x55)` is indexed with (+1). */
/* The item record `fn_8033ADD0` selects: the item id the panel's icon/name/exp lines take. */
typedef struct ItemRec {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u16 id;
} ItemRec; /* size: 0x4 */

typedef struct MenuRowRec {
    /* +0x00 */ u8 kind;             /* the damage/species kind the value block switches on */
    /* +0x01 */ u8 mode;             /* the value block's variant, and the `get_menu_lsp_tbl(0x55)` index */
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;       /* how many bars the value block blits */
} MenuRowRec; /* size: 0x4 */

/* The 2D integer vector the menu/HUD helpers exchange (`_mh_ivec2_` in the map's manglings) and the
 * sprite-data tag the `draw_*` family takes by reference (`_SPR_DATA_`).  Both are this unit's view:
 * `unsplit/lobby.h` and `hud/layout.h` define the tag and the vector as well, and
 * including either here collides with this unit's own `_SPR_DATA_` view in the source. */
struct _SPR_DATA_;
typedef struct _mh_ivec2_ {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} _mh_ivec2_; /* size: 0x4 */

/* ---- this unit's entry points and its same-file helpers (the map's `fn_` stems) ---- */
void item_page_scroll_rows(MenuScroll* scroll, MenuEntry* rows, u16 buttons);  /* 0x80349DD8 */
void fn_80349C9C(MenuScroll* scroll, MenuEntry* rows);               /* 0x80349C9C, the list's own row build */
void item_page_fill_rows(MenuSlot* slot);                                    /* 0x80349E30 */
void item_page_update_rows(MenuSlot* slot, u16 buttons);                       /* 0x80349F8C */
void item_page_draw(MenuSlot* slot);                                    /* 0x80349FF0 */
void item_page_draw_page_arrow(u16* table, u32 id, s32 flag, _mh_ivec2_* pos); /* 0x8034A398 */
void item_page_draw_page_widget(s8 page, _mh_ivec2_* pos);                          /* 0x8034A448 */
void item_page_draw_rows(MenuEntry* entries, u8 sel, u8 count, u8 id, u8 id2, u8 first, u8 blend);
void item_page_draw_blank_rows(MenuEntry* entries, u8 sel, u8 count, u8 id, u8 id2);
u32 item_page_item_price(MenuSlot* slot, u16 id);                             /* 0x8034A814 */
void item_page_draw_detail0(MenuSlot* slot);                                    /* 0x8034A914 */
void item_page_draw_column0(MenuSlot* slot);                                    /* 0x8034AF48 */
void item_page_draw_wrapped_text(u16 id, u16 part, s8* text, const _mh_ivec2_* pos);  /* 0x8034B024 */
void item_page_draw_detail1(MenuSlot* slot);                                    /* 0x8034B104 */
void item_page_draw_column1(MenuSlot* slot);                                    /* 0x8034B560 */
void item_page_draw_detail2(MenuSlot* slot);                                    /* 0x8034B59C */
void item_page_draw_column2(MenuSlot* slot);                                    /* 0x8034B7EC */
void item_page_draw_column3(MenuSlot* slot);                                    /* 0x8034B828 */
void item_page_draw_closed_column(u8 kind, u8 index, s8 light, s8 dark, u8 id, u16 word, s32 flags);
s8 item_page_option_row_index(u8 kind, u8 index, s8 light);                         /* 0x8034BBB0 */
s8* item_page_option_value_string(u8 kind, u8 index, s8 light, s8* out);               /* 0x8034BC58 */
void item_page_draw_option_frame(s8 a, s8 b, u16 c, u16 d, u8 flags, _mh_ivec2_* pos);
u32 item_page_option_available(s8 index);                                           /* 0x8034BFC8 */

#ifdef __cplusplus
}
#endif

/* ---- the callees whose map names are plain `fn_` stems (C linkage, rule 9's placeholder case) ---- */
#ifdef __cplusplus
extern "C" {
#endif

void sprite_frame_apply(_SPR_DATA_* rec, u16 id, u16 part, _mh_ivec2_* out); /* 0x802E0AD4, hud/layout.cpp */
void fn_802E1A7C(u16 id, u16 part, u16 arg2, const _mh_ivec2_* pos);
void fn_802E23D0(u32 id, u32 part, s8* text, u8 flag, const _mh_ivec2_* pos);
u32 color_lerp(u32 value, u32 mask);
void fn_802A9F48(u8 value, u16 kind, s8* text, const _mh_ivec2_* pos, s32 flag);
u8 GameMode_ck(void);
u16 fn_8004AE70(void* userdata);
s32 fn_8004AF0C(u8 idx);
void* fn_8004AF60(void* userdata, u8 idx);
u32 item_count_find(u16 id, void* a, s32 b);
u32 fn_8004B0A4(u16 id, void* userdata);
u32 fn_8004B70C(u16 id, void* a, u16 b);
u8 fn_8004E634(u8 a, u16 b);
MenuRowData* fn_8004EA24(void);
char* flfntStrChr(char* text, s32 c);   /* owner: `g3d/g3d_anmchr.cpp` */
void fn_800CEE74(u8 a, u16 b, f32* out);
u32 fn_8026FE44(s32 worker);
s32 Pl_item_timer_get(s32 worker, u16 id);
void fn_802D9EA8(void);
u32 fn_802DA20C(s32 a, u8 b);
void fn_802DA2D4(s32 flag);
s32 hud_notice_spawn(s32 a, u8 b, s16 c, s16 d, s32 e, s32 f, s32 g);
s8* fn_802DFB18(u8 idx);
u16* fn_8033ADD0(u16 a, u16* b, u16* c, u16 d);
s8 fn_8033AED0(void* a, u16 b);
u32 fn_8033B67C(void* a, void* b);
void* get_userdata(void);

/* The lobby item database `item_page_item_price` reads the price block out of: a 4-byte pointer at
 * `.sbss:0x80794880`, loaded through sda21.  Only the two offsets this unit names are fields.
 * size: 0x3F06 (approximate: the highest offset this unit touches + 2) */
typedef struct LbItemDb {
    /* +0x0000 */ u8 unused_0x0000[0x0180];
    /* +0x0180 */ u8 field_0x0180;     /* the sub-block `fn_8004B70C` is handed */
    /* +0x0181 */ u8 unused_0x0181[0x3F04 - 0x0181];
    /* +0x3F04 */ u16 field_0x3F04;    /* the value the "%d" price line prints */
} LbItemDb;
extern LbItemDb* lobby_world_block;

s32 item_page_option_item_id(u8 kind, u8 row);

/* libc (the map carries the plain names). */
s32 sprintf(s8* dst, const char* fmt, ...);
s8* strcpy(s8* dst, const s8* src);
s32 strcmp(const s8* a, const s8* b);

#ifdef __cplusplus
}
#endif

/* ---- the callees whose map names are manglings (rule 9: declare the real signature) ---- */
#ifdef __cplusplus

/* The sprite/font library `hud/layout.cpp` owns (the shapes `unsplit/lobby.h` carries, minus
 * its `lobby_world_block` array view - this unit needs the pointer the map records). */
void* get_lsp_data(u16 id, _mh_ivec2_* out);
void draw_sprite(const _SPR_DATA_& spr, const _mh_ivec2_* pos);
void draw_font(const _SPR_DATA_& spr, s8* text, u32 flag, const _mh_ivec2_* pos);
void draw_font_idx(u16 id, s8* text, u32 flag, const _mh_ivec2_* pos);
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos);
void draw_sprite_anim_idx(u16 id, u16 anim, const _mh_ivec2_* pos);
void draw_sprite_anim_ary(const u16* ids, u16 anim, const _mh_ivec2_* pos);
void draw_itemicon_item_id(const _SPR_DATA_& spr, u16 id, const _mh_ivec2_* pos);
void font_set_size(s16 w, s16 h);
s32 chk_pointer(void);
s32 spr_data_copy(s16* dst, void* src);
u8 get_option_cfg(u8 index);

/* `lobby/lb_pane_ui.cpp`'s page arrow and the menu band's own text helpers. */
void PutPageArrow(u16* table, s16 a, s16 b, u16 c, const _mh_ivec2_* pos, u8 flags);
u8** get_str_tbl(s32 idx);
void set_blendmode(u8 a, u8 b, u8 c);
void flfntSetColor(u32 colour);
void font_flush(void);
void font_print_ex(s16 x, s16 y, s16 flag, const char* fmt, ...);
u32 GetEquipName(u8 kind, u16 id);


#endif /* __cplusplus */

#endif /* MHTRI_MENU_MENU_ITEM_PAGE_H */
