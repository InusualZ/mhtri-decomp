/*
 * menu/menu_message.h - `menu/menu_message.cpp`'s list work record and its message/frame/dialog entry points.
 *   `MenuListWork` is the same object `menu/menu_item.h` views as `MenuSlot` (0x330 B): its +0x0F kind and +0x16 count
 *   are `MenuSlot`'s `field_0x00F`/`entry_count_a`, its list and name fields sit in `MenuSlot`'s untraced +0x19C..+0x23A.
 */
#ifndef MHTRI_MENU_MENU_MESSAGE_H
#define MHTRI_MENU_MENU_MESSAGE_H

#include "types.h"
#include "id_value.h"

/* One record of the item-record table `lobby_hunter_cards` (two blocks of five, 0x130 B each): a validity
 * byte, the short (10-byte) name the list keeps, and the long name at +0x0D.  Field sizes are the
 * gaps the table's own `strcpy` call sites walk; the rest of the record is not touched by this range.
 * size: 0x130 */
typedef struct MenuSourceRecord {
    /* +0x000 */ u8 valid_0x000;
    /* +0x001 */ u8 unused_0x001[0x003 - 0x001];
    /* +0x003 */ char short_name_0x003[0x00A];
    /* +0x00D */ char long_name_0x00D[0x010];
    /* +0x01D */ u8 unused_0x01D[0x130 - 0x01D];
} MenuSourceRecord;

/* One record of the move table `get_move_work_adrs` hands back (0xB20 B, `get_move_work_max` of them):
 * the same validity byte, with the two names at the far end the move-table `strcpy` sites use.
 * size: 0xB20 */
typedef struct MenuMoveRecord {
    /* +0x000 */ u8 valid_0x000;
    /* +0x001 */ u8 unused_0x001[0x5CA - 0x001];
    /* +0x5CA */ char long_name_0x5CA[0x011];
    /* +0x5DB */ char short_name_0x5DB[0x00A];
    /* +0x5E5 */ u8 unused_0x5E5[0xB20 - 0x5E5];
} MenuMoveRecord;

/* The 2D integer position the menu/HUD helpers exchange.  Same record as `unsplit/lobby.h`'s
 * `_mh_ivec2_` (0x4 B); named locally so the calls can keep the map's `...P10_mh_ivec2_` mangling
 * (via `struct _mh_ivec2_` forward declarations) without including that header, whose
 * `menu_cursor_step` declaration this range owns - see the unit header's rule-2 note.
 * size: 0x4 */
typedef struct MenuLspPos {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} MenuLspPos;

/* One 0x18-byte entry of the list block `menu_frame_entries_build`/`menu_frame_entries_build_row` build before they hand it to the
 * draw helpers: the entry's ordinal, a zeroed flag byte and the "the source slot is in use" byte.
 * The remaining bytes are not touched by this range.
 * size: 0x18 */
typedef struct MenuListEntry {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 field_0x01;       /* zeroed on every entry */
    /* +0x02 */ u8 present_0x02;     /* 1 while the source record's word is non-zero */
    /* +0x03 */ u8 unused_0x03[0x006 - 0x003];
    /* +0x06 */ s16 index_0x06;      /* the entry's ordinal */
    /* +0x08 */ u8 unused_0x08[0x018 - 0x008];
} MenuListEntry;

/* The menu list work record.  Only the bytes this range touches are named; +0x1E0 is the last
 * offset any function in the range writes.
 * size: 0x330 (the `MenuSlot` extent; this view stops at +0x1E0) */
struct MenuListWork {
    /* +0x000 */ u16 input_0x000;     /* the pad/step bits the cursor step reads */
    /* +0x002 */ u16 step_0x002;      /* set to 4/8 by the vertical cursor step */
    /* +0x004 */ s8 cursor_0x004;     /* the cursor row */
    /* +0x005 */ s8 rows_0x005;       /* rows per page */
    /* +0x006 */ s8 column_0x006;     /* the cursor column */
    /* +0x007 */ s8 columns_0x007;    /* columns per row */
    /* +0x008 */ u8 unused_0x008[0x00F - 0x008];
    /* +0x00F */ u8 kind_0x00F;       /* 1: the move table, 2: the record table */
    /* +0x010 */ u8 two_page_0x010;  /* set when a slot index of 24 or more addresses `page_b_0x198` */
    /* +0x011 */ u8 unused_0x011[0x016 - 0x011];
    /* +0x016 */ u8 count_0x016;      /* the list count `menu_list_count_update` recomputes */
    /* +0x017 */ u8 unused_0x017[0x194 - 0x017];
    /* +0x194 */ IdValue* page_a_0x194; /* the 24 slots of the first page */
    /* +0x198 */ IdValue* page_b_0x198; /* the 8 slots of the second page (`two_page_0x010`) */
    /* +0x19C */ u8 unused_0x19C[0x1BB - 0x19C];
    /* +0x1BB */ u8 list_count_0x1BB; /* the count `menu_list_fill` fills in */
    /* +0x1BC */ u8 unused_0x1BC[0x1BE - 0x1BC];
    /* +0x1BE */ s8 list_0x1BE[0x0A]; /* the ten list ids `menu_list_fill` writes */
    /* +0x1C8 */ char short_name_0x1C8[0x0A];
    /* +0x1D2 */ char long_name_0x1D2[0x0E];
};

/* The range's own entry points the neighbouring units call; `unsplit/lobby.h` and `lobby/lb_pane_ui.h` include this
 * header for them (rule 2).  The spellings are this range's own definitions' (the `s32` first two
 * parameters are what the retail call sites need: a narrow argument must not be narrowed back to `s16`
 * for the call).  `toggle_word_step*` take their caller-owned state word untyped; the cursor steps' `moved`
 * tail is the `u16*` the callers hand in.  The record-typed helpers (`menu_list_*`, `menu_slot_*`,
 * `menu_item_slot_accepts`, `menu_cursor_column_step`) are not declared here: their consumers still view
 * the record as their own struct (`MENU_ITEM_W`, `MenuSel`) and declare them locally. */
struct MenuScroll;

#ifdef __cplusplus
extern "C" {
#endif

void menu_hold_row_draw_by_lsp(u16 id, u16* item);
void draw_dialog_piece(struct _SPR_DATA_* spr, s16 x, s16 y, s16 width, s16 height, u16 index,
                         const struct _mh_ivec2_* pos);
void put_frame_dialog(s16 x, s16 y, s16 width, s16 height, u32 color, u32 frame_color);

s32 menu_cursor_step_fixed_tail(s32 a, s32 b, u16 c, u16 d, u16 e, u16* moved);
s32 menu_cursor_step_open_last(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f);
s32 menu_cursor_step_forward(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f, u16* moved);
s32 menu_cursor_step(s32 a, s32 b, u16 c, u16 d, u16 e);
s16 menu_page_count(s16 a, s16 b);
s32 menu_cursor_page_move(s16* cursor, s16 max, u16 keys, u16 held, u16* moved);
void menu_scroll_init(struct MenuScroll* scroll, u16 index, u16 total, u8 rows_per_page);
void menu_scroll_step(struct MenuScroll* scroll, u16 buttons, s32 sfx);
void item_pairs_compact_sort(IdValue* pairs, s32 count);
void item_pages_sort(IdValue* pool, IdValue* page);
/* untyped: caller-owned payload - the callers pass their own `s32 stepper_*` / `u32` state word */
s32 toggle_word_step_dpad(void* state, u16 keys, s32 dec_mask, s32 inc_mask);
/* untyped: caller-owned payload - the callers pass their own `s32 stepper_*` / `u32` state word */
s32 toggle_word_step(void* state, u16 pad, s32 a, s32 b, s32 c);
s8* get_item_name_str(u8 index);
s8* get_player_name_str(u8 index);
s8* get_group2_name_str(u8 index);
s8* get_digit_str(u8 index);

#ifdef __cplusplus
}

/* 0x802AA3EC - the `bool` row's colour, the front-end's spelling of the map's
 * `GetMenuFontColor__Fbbbb` (rule 9). */
s32 GetMenuFontColor(bool a, bool b, bool c, bool d);
s32 GetMenuFontColorRed(bool a, bool b, bool c, bool d);
s32 GetMenuIconColor(bool a, bool b, bool c, bool d);

/* 0x802A8D74 - the system dialog's OK-button row. */
void put_message_sys_ok_button(void);
#endif

#endif /* MHTRI_MENU_MENU_MESSAGE_H */
