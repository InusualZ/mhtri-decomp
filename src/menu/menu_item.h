/*
 * menu/menu_item.h - `menu/menu_item.cpp`'s outbound declarations and the records its bodies read: the item-record
 *   accessors (`ItemName`, `ItemExp`, `GetItemData`), the menu tables (`get_menu_tbl_ptr`, `get_menu_lsp_tbl`), the hit
 *   helpers (`body_set`, `hit_flag_set`, `hit_result_check`; `_HIT_W` is `Pl/hit_w.h`'s) and the menu slot `MenuSlot`.
 */
#ifndef MHTRI_MENU_MENU_ITEM_H
#define MHTRI_MENU_MENU_ITEM_H

#include "types.h"
#include "pl.h"
#include "Pl/hit_w.h"
#include "id_value.h"   /* `_HIT_W` / `HitRegistry`, shared with `Pl/pl_coll.cpp` (rule 1) */


/* The record `body_set` copies the hunter's armour/appearance bytes out of; only the four bytes
 * this unit reads are named, and the record is at least 0x1E2 bytes (the highest offset touched).
 * size: 0x1E2 (approximate: max touched offset + 1) */
struct _BODY_DATA {
    /* +0x000 */ u8 unused_0x000[0x008];
    /* +0x008 */ u8 chunk_ofs;        /* kind 2's source byte */
    /* +0x009 */ u8 unused_0x009[0x016 - 0x009];
    /* +0x016 */ u8 area;             /* kind 0's source byte */
    /* +0x017 */ u8 unused_0x017[0x1A4 - 0x017];
    /* +0x1A4 */ u8 field_0x1A4;      /* kind 3's source byte */
    /* +0x1A5 */ u8 unused_0x1A5[0x1E1 - 0x1A5];
    /* +0x1E1 */ u8 field_0x1E1;      /* kind 1's source byte */
};

/* The body slot `body_set` fills in from a `_BODY_DATA`.  Only the bytes this unit writes are
 * named; +0x03F is the last one written, and no function here reads past it.
 * size: 0x40 (approximate: max touched offset + 1) */
struct _BODY_W {
    /* +0x000 */ u8 unused_0x000[0x005];
    /* +0x005 */ u8 field_0x005;      /* set to 1 on every call */
    /* +0x006 */ u8 field_0x006;      /* cleared on every call */
    /* +0x007 */ u8 kind;             /* the kind argument, kept */
    /* +0x008 */ u8 source;           /* the byte the kind's source field supplied */
    /* +0x009 */ u8 source_kind;      /* the kind's sub-code */
    /* +0x00A */ u16 field_0x00A;
    /* +0x00C */ u32 data;            /* the `_BODY_DATA` the slot was filled from */
    /* +0x010 */ u32 field_0x010;
    /* +0x014 */ u16 field_0x014;
    /* +0x016 */ u8 unused_0x016[0x03F - 0x016];
    /* +0x03F */ u8 field_0x03F;
};

/* One item record (0x14 bytes): `.data`-backed, 747 of them, behind the pointer at +0x00 of
 * `lbl_806AC8B8`.  Offsets from `GetItemData`'s 0x14 stride and the byte loads the accessors do. */
struct ItemDataRecord {
    /* +0x000 */ union { /* the record's first pair: the pre-merge `u16` view and the two bytes the
                          * option list's pick test reads, at the same offset (rule 5: one member
                          * per offset, so no later field moves).  The folded half of this unit
                          * (`menu_item_page_draw`/`menu_item_use_ck`) reads it as `kind_0x00`/`level_0x01` and the
                          * retail bytes load each one separately (`lbz r0,0x0(r3)`, `cmplwi r0,0x1`). */
        /* +0x000 */ u16 unused_0x000;   /* the pre-merge spelling, kept as a union member (rule 5) */
        struct {
            /* +0x000 */ u8 kind_0x00;   /* `1` marks the entry the cursor may not pass */
            /* +0x001 */ u8 level_0x01;  /* the same test, against 3 */
        };
    };
    /* +0x002 */ u8 field_0x002;      /* the mask `item_category_ck` ANDs its second argument with */
    /* +0x003 */ u8 max_num_0x003;    /* the cap a slot's count is clamped to (`quest/quest_entry.cpp`'s
                                       * `quest_item_slot_add` compares a slot's own count against it
                                       * as a signed byte, and clamps both on overflow and on a negative
                                       * merge) */
    /* +0x004 */ u8 tex_idx_0x004;    /* the texture index `draw_itemicon_item_id`/`draw_itemicon_item_id_idx`
                                       * (both in `hud/layout.cpp`) hand to `atlas_digit_uv_get`, and the
                                       * byte `fn_802E1024`'s callers read next to `kind`.  Spliced
                                       * out of the `unused_0x003[2]` filler this record carried, one
                                       * byte at +0x004, so no member's offset moved (playbook 56). */
    /* +0x005 */ u8 kind;             /* indexes the .data colour table `item_kind_color_tbl` */
    /* +0x006 */ u16 sort_order_0x006; /* the high half of the key `item_pair_sort_key` orders a list by */
    /* +0x008 */ u8 unused_0x008[0x00A - 0x008];
    /* +0x00A */ u16 species;        /* indexes the .data record table `lbl_805DBFB8` */
    /* +0x00C */ s32 field_0x00C;      /* the value `eft052_item_half_get` halves (added by
                                       * `ef/eft052.cpp`) */
    /* +0x010 */ u32 field_0x010;      /* the per-item value `eft052_item_value_get` hands back (added by the
                                       * same unit) */
};


/* The item table pointers at `.bss:0x806AC8B8` (0x10 B), the block `get_item_data_ptr` returns.
 * `items` is the 747 x 0x14 record array, `names`/`exp` the two 747-entry word tables. */
struct ItemDataHead {
    /* +0x000 */ ItemDataRecord* items;
    /* +0x004 */ u32* names;          /* the name-id table `ItemName` indexes */
    /* +0x008 */ u32* exp;            /* the experience table `ItemExp` indexes */
    /* +0x00C */ u32 unused_0x00C;
};

/* The menu table set at `.bss:0x806ACF28` (0x770 B, the whole `.bss` run between the item head and
 * `shell_pool`).  The three accessors below index `lsp_tbl`, `tab_0x66C` and `array_0x758`; the
 * 0xE8 bytes between `tab_0x66C` and `field_0x754` are not touched by this unit. */
struct MenuTables {
    /* +0x000 */ u32* lsp_tbl[0x66C / 4];
    /* +0x66C */ u32* tab_0x66C[0xE8 / 4];
    /* +0x754 */ u32 field_0x754;
    /* +0x758 */ u32 array_0x758[0x14 / 4];
    /* +0x76C */ u32 unused_0x76C;
};

/* One 0x0C-byte record of the `.data:0x805DBFB8` table (132 of them): the `+0x08` byte is what
 * `item_species_name_get` passes on. */
struct ItemSpeciesRecord {
    /* +0x000 */ u8 slot_size;        /* GUESS: the decoration slots the item takes; the readers compare it to 1/2/3 */
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ u16 item_id;        /* the item the record is for (`menu/menu_infomation.cpp` looks up its class) */
    /* +0x004 */ u32 unused_0x004;
    /* +0x008 */ u8 kind;
    /* +0x009 */ u8 kind_points;     /* the points the decoration adds to skill `kind` (signed, read as `s8`) */
    /* +0x00A */ u8 kind2;           /* its second skill (0 = none) */
    /* +0x00B */ u8 kind2_points;    /* the points it adds to `kind2` (signed, read as `s8`) */
};

/* One 0x18-byte menu entry: the three entry arrays inside a `MenuSlot` are made of these, and the
 * flag at +0x01 is the selection the setters below write.  Offsets from `menu_top_reset`'s
 * `memset(self + 36, 0, 72)` (three entries at +0x24) and the 24-byte stride of the loops. */
struct MenuEntry {
    /* +0x00 */ s8 page_kind;        /* the page kind a sub-list row opens (`MenuSlot::field_0x014`) */
    /* +0x01 */ s8 selected;         /* 1 for the entry the caller's index names, else 0 */
    /* +0x02 */ s8 field_0x02;       /* nonzero when the row may be picked */
    /* +0x03 */ u8 unused_0x03[0x006 - 0x003];
    /* +0x06 */ s16 field_0x06;      /* the inventory row the entry draws (the kind tables' index) */
    /* +0x08 */ u32 field_0x08;      /* the word `menu_sub_rows_build` copies in from the menu table */
    /* +0x0C */ u32 field_0x0C;      /* cleared by the item page's row fill; the row's light colour when it is marked */
    /* +0x10 */ u32 colour_0x10;     /* the marked row's dark colour */
    /* +0x14 */ u32 colour_0x14;     /* the marked row's text colour */
};

#include "menu/menu_scroll.h" /* `MenuScroll` (shared with `menu/movie.h`) */

struct _mh_ivec2_;

/* One 0x24-byte record of the placement list `MenuWork::slot[0].place_entries` holds: the record's
 * rectangle, the four halves its creator stores, and the 16-byte body copied in at +0x14.  `menu/menu_row.cpp`
 * walks it and `menu_item_work_init` (this unit) clears and numbers it.
 * size: 0x24 */
struct MenuPlaceRec {
    /* +0x00 */ u8 active;
    /* +0x01 */ u8 index;          /* the byte `place_rec_alloc` hands back to its caller */
    /* +0x02 */ u8 pad_0x02[0x6];
    /* +0x08 */ s16 pos_x;          /* the rectangle `place_list_overlap_find`'s overlap test reads */
    /* +0x0A */ s16 pos_y;
    /* +0x0C */ s16 width;
    /* +0x0E */ s16 height;
    /* +0x10 */ s16 field_0x10;
    /* +0x12 */ s16 field_0x12;
    /* +0x14 */ u32 key;            /* the id `place_list_key_count` counts and `place_rec_key_ptr` hands out */
    /* +0x18 */ u8 body[0x0C];      /* the 16-byte body `place_rec_copy` copies over +0x14 */
}; /* size: 0x24 */

/* One of the two 0x330-byte working records of the menu work area at `.bss:0x806AC8C8`; the area is
 * `MenuSlot slot[2]` (0x660 = the whole `.bss` run, and `menu_item_close` walks it with an 816-byte
 * stride).  The three entry arrays and their counts are the ones the setters below walk; the other
 * fields are the ones this unit's bodies touch.  The item page's draw layer
 * (`menu/menu_item_page.cpp`) named the bytes it introduced: the page index at +0x01C, the per-kind
 * selected-row offsets at +0x01D, the embedded `MenuScroll` at +0x1A4 and the per-kind row base at
 * +0x2ED. */
struct MenuSlot {
    /* +0x000 */ u8 field_0x000;      /* the slot's mode/state the predicates below compare */
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 unused_0x003;
    /* +0x004 */ u16 buttons_trig;    /* the pad's pressed-this-frame word (`Psw` +0x2C4) `menu_pad_read` copies */
    /* +0x006 */ u16 buttons_0x006;   /* the pad's +0x2C0 word */
    /* +0x008 */ u16 field_0x008;     /* the option card's arrow word (`item_page_draw_closed_column`'s `arg5`);
                                       * `menu_pad_read` stores the repeat word here (`Psw` +0x2D4 | +0x2C4) */
    /* +0x00A */ u16 buttons_0x00A;   /* the pad's +0x2CC word */
    /* +0x00C */ u16 buttons_0x00C;   /* the pad's edge bytes (`+0x2DF | (+0x2DD & ~+0x2DE)`) */
    /* +0x00E */ u8 active;           /* the "slot in use" flag every predicate below tests */
    /* +0x00F */ u8 field_0x00F;      /* 1 selects the `menu_item_input_ok` arm that reads `worker` */
    /* +0x010 */ u8 field_0x010;
    /* +0x011 */ u8 field_0x011;
    /* +0x012 */ u8 pad_no;           /* the controller the slot's player sits on (`player_pad_channel_get`) */
    /* +0x013 */ u8 open_wait;        /* frames the menu ignores its close button after opening (10) */
    /* +0x014 */ s8 field_0x014;      /* the value `menu_page_open_ck`/`menu_item_list_ready_ck` compare */
    /* +0x015 */ u8 field_0x015;
    /* +0x016 */ u8 entry_count_a;    /* `entries_a`'s count (`menu_tab_select`) */
    /* +0x017 */ s8 entry_count_b;    /* `entries_b`'s count (`menu_slot_entry_b_select`, `menu_split_row_select`) */
    /* +0x018 */ s8 entry_count_c;    /* `entries_c`'s count (`menu_sub_rows_build`, `menu_sub_row_select`) */
    /* +0x019 */ s8 field_0x019;      /* the top menu's list (0 = items, 1 = quest, 2 = system) */
    /* +0x01A */ s8 field_0x01A;      /* the sub-list row cursor (`entries_c`) */
    /* +0x01B */ s8 field_0x01B;      /* the top-menu row cursor (`entries_b`) */
    /* +0x01C */ s8 page_index;       /* 0..3: the page the detail draw switches on */
    /* +0x01D */ u8 field_0x01D;      /* kind 1's selected row offset (the page-0 blend index) */
    /* +0x01E */ u8 field_0x01E;      /* kind 2's */
    /* +0x01F */ u8 field_0x01F;      /* kind 3's */
    /* +0x020 */ u8 sub_list_open;    /* 1 while the third entry array (`entries_c`) is shown */
    /* +0x021 */ u8 field_0x021;      /* the byte `menu_slot0_lock_set`/`menu_slot_lock_set` set */
    /* +0x022 */ u8 unused_0x022[0x024 - 0x022];
    /* +0x024 */ MenuEntry entries_a[3];
    /* +0x06C */ MenuEntry entries_b[8];
    /* +0x12C */ MenuEntry entries_c[4];
    /* +0x18C */ s32 field_0x18C;     /* set to 1 by `menu_quit_open`/`menu_lobby_exit_open` */
    /* +0x190 */ _PLW* worker;        /* the player work the slot displays */
    /* +0x194 */ IdValue* page_a;     /* the first item page: the player's pouch, or the lobby box's slots */
    /* +0x198 */ IdValue* page_b;     /* the second page, or null when the player has one page */
    /* +0x19C */ u16 field_0x19C;     /* the key word `menu_cursor_column_step` is handed */
    /* +0x19E */ u16 field_0x19E;     /* the value `menu/menu_item.cpp`'s view calls `field_0x19E` */
    /* +0x1A0 */ s8 field_0x1A0;      /* the column the equipment panel puts the cursor in */
    /* +0x1A1 */ s8 field_0x1A1;      /* its row (`menu_page_count` resolves the height into it) */
    /* +0x1A2 */ s8 field_0x1A2;
    /* +0x1A3 */ s8 field_0x1A3;
    /* +0x1A4 */ MenuScroll scroll;   /* the list-scroll cursors `menu_scroll_step` moves */
    /* +0x1B0 */ u8 field_0x1B0;
    /* +0x1B1 */ u8 flag_0x1B1;
    /* +0x1B2 */ u16 item_id_0x1B2;   /* the item the use/move panel names */
    /* +0x1B4 */ s16 field_0x1B4;
    /* +0x1B6 */ s16 count_0x1B6;     /* the panel's count (`menu_cursor_page_move`'s cursor) */
    /* +0x1B8 */ s16 count_max_0x1B8; /* its bound */
    /* +0x1BA */ s8 item_cursor;
    /* +0x1BB */ u8 item_count;
    /* +0x1BC */ u8 panel_on;         /* the use/move panel is shown */
    /* +0x1BD */ u8 panel_frame;      /* the panel's animation frame, wrapped at the sprite's last frame */
    /* +0x1BE */ u8 items_0x1BE[0x0A];
    /* +0x1C8 */ char label_0x1C8[0x0A]; /* the panel's label text */
    /* +0x1D2 */ char name_0x1D2[0x14];  /* the panel's name text */
    /* +0x1E6 */ s8 field_0x1E6;
    /* +0x1E7 */ u8 field_0x1E7;
    /* +0x1E8 */ s16 ticket;          /* the cockpit message ticket (`cockpit_ticket_acquire`) */
    /* +0x1EA */ u8 unused_0x1EA[0x1EC - 0x1EA];
    /* +0x1EC */ u8 field_0x1EC[0x1F0 - 0x1EC];  /* the embedded selection cursor */
    /* +0x1F0 */ u16 field_0x1F0;    /* the cursor's two item ids */
    /* +0x1F2 */ u16 field_0x1F2;
    /* +0x1F4 */ u8 unused_0x1F4[0x234 - 0x1F4];
    /* +0x234 */ u8 option_value_0x234;  /* the option pages' value before the change (restored on cancel) */
    /* +0x235 */ u8 unused_0x235;
    /* +0x236 */ s16 option_move_0x236;  /* the arrow (4/8) that changed the option value this frame */
    /* +0x238 */ u16 move_result_0x238; /* which button last moved the cursor (`menu_cursor_page_move`) */
    /* +0x23A */ u8 field_0x23A;     /* the sprite id every draw of the item page takes */
    /* +0x23B */ u8 field_0x23B;     /* 0 = the page draws no detail panel */
    /* +0x23C */ u8 field_0x23C;     /* the blend colour the item page's rows are handed */
    /* +0x23D */ u8 unused_0x23D[0x2ED - 0x23D];
    /* +0x2ED */ u8 kind_row_base[8]; /* `kind_row_base[kind * 2]`: the first row the item kind shows */
    /* +0x2F5 */ u8 field_0x2F5;     /* the item kind (0..3) the page is showing */
    /* +0x2F6 */ char text_0x2F6[0x28]; /* the player-search keyboard text (cleared by `plsearch_menu_init`) */
    /* +0x31E */ u8 field_0x31E;
    /* +0x31F */ u8 field_0x31F;
    /* +0x320 */ s8 net_result_0x320;  /* the player-search network request's result (0 pending, 1 done, else failed) */
    /* +0x321 */ u8 unused_0x321[0x3];
    /* +0x324 */ MenuPlaceRec* place_entries; /* the 0x24-byte placement list `menu/menu_row.cpp` walks */
    /* +0x328 */ s32 place_count;
    /* +0x32C */ u8 field_0x32C;      /* the mailbox list's row offset, counted up by `mailbox_row_offset_inc` */
    /* +0x32D */ u8 field_0x32D;      /* the flag `menu_item_close` clears after `kbd_close_call` */
    /* +0x32E */ s16 wait_0x32E;      /* the player-search menu's message wait (15 frames) */
};

/* The menu work area itself: the two slots, `.bss` 0x806AC8C8..0x806ACF28 (0x660 B - exactly two
 * 0x330-byte slots, which is what `menu_item_close`'s 816-byte stride walks). */
struct MenuWork {
    /* +0x000 */ MenuSlot slot[2];
};


/* The frame work this unit's per-frame entry points (`menu_item_frame_update`, `menu_frame_busy_ck`) carry; only the
 * three bytes they read are named.
 * size: 0x5C0 (approximate: max touched offset + 1) */
struct MenuFrameWork {
    /* +0x000 */ u8 unused_0x000[0x008];
    /* +0x008 */ u8 slot_index;       /* the `MenuSlot` index `menu_item_frame_update` selects */
    /* +0x009 */ u8 unused_0x009[0x5BC - 0x009];
    /* +0x5BC */ u8 field_0x5BC;
    /* +0x5BD */ u8 field_0x5BD;
    /* +0x5BE */ u8 field_0x5BE;
};

/* The `.bss`/`.data` blocks the accessors read: this unit's own sections, declared never defined (playbook 29),
 * so the accessors take their addresses instead of defining them. */
extern HitRegistry lbl_806AC8A8;
extern ItemDataHead lbl_806AC8B8;
extern MenuWork menu_work;
extern MenuTables lbl_806ACF28;
extern u32 item_kind_color_tbl[];              /* .data:0x805CDE78 - 0x30 B, indexed by `ItemDataRecord::kind` */
extern ItemSpeciesRecord lbl_805DBFB8[]; /* .data:0x805DBFB8 - 132 x 0x0C B */

/* This unit's own entry points, in address order, and the callees its written bodies call.
 *
 * The callees split three ways by whose header can supply a declaration (rule 2):
 *   * `Pl/pl_act.cpp` (`fn_8027EB18`, `fn_8027E120`), `pad_connect.cpp` (`screen_split_mode_ck`) and
 *     `fn_80040598.cpp` (`kbd_close_call`) have no header that declares these, so the shapes here are this unit's
 *     call sites'; `mh3_pad.h` cannot be included here because it and `ef.h` (which `pl.h` pulls in) collide.
 *   * `GameMode_ck` keeps its owner's (`ef/fn_800CDB2C.h`) `u8` spelling: a `u32` view is `(10505) illegal
 *     overloading` beside the owner's header.
 *   * the rest (`fn_802FBA60`, `fn_8031A638`, `ainpc_entry_kill_kind`, `ainpc_page_state_init`,
 *     `ainpc_page_hold_clear`, `game_ready_ck`) are declared in their call sites' shapes; the page callees the menu
 *     work dispatches to come from their leaf headers, included by `menu/menu_item.cpp`.
 *
 * The `fn_*` declarations keep C linkage: the map's names for them are placeholders, not manglings.
 */
#ifndef MHTRI_MENU_MENU_ITEM_DECLARED
#define MHTRI_MENU_MENU_ITEM_DECLARED

/* This unit's own C++-mangled entry points (the map names are these manglings, rule 9).
 * `put_menu_cursor` (0x802A2564) is one of them; its consumer (`lobby/fn_801E7530.cpp`) includes this header for it
 * (rule 2).  Its third parameter is the 2D vector, whose tag is declared here rather than including a band header. */
struct _mh_ivec2_;

void body_set(_BODY_W* body, _BODY_DATA* data, u8 kind, u32 work, u8 mode);
void hit_flag_set(_HIT_W* hit, u32 flags);
u8 hit_result_check(_HIT_W* hit);
u32 ItemName(u16 id);
u32 ItemExp(u16 id);
ItemDataRecord* GetItemData(u16 id);
ItemDataHead* get_item_data_ptr(void);
MenuTables* get_menu_tbl_ptr(void);
u32* get_menu_lsp_tbl(u16 idx);
void put_menu_cursor(u16* rows, u16 index, const _mh_ivec2_* pos);

#ifdef __cplusplus
extern "C" {
#endif

void hit_motion_set(_HIT_W* hit, u16 a, u16 b, u16 c, u16 d);
u32 hit_mask_ck(_HIT_W* hit, u32 mask);
void hit_flags_clear(_HIT_W* hit);
void fn_8029F554(_HIT_W* hit, u32 flags);
u32 fn_8029F564(_HIT_W* hit, u32 flags);
u32 fn_8029F57C(_HIT_W* hit, u32 mask);
void hit_knock_set(_HIT_W* hit, s16 value);
u32 fn_8029F5E4(_HIT_W* hit);
u32 fn_8029F5F8(_HIT_W* hit);
u32 fn_8029F60C(void);
void item_species_name_get(u16 id);
ItemSpeciesRecord* get_item_species_data(u16 id);
u32 item_rare_color_get(u16 id);
s32 item_category_ck(u16 id, s32 index);
u32 item_is_53_ck(u16 id);
u32 item_kind_color_by_kind(u16 kind);
u32 get_menu_msg_tbl(u16 idx);
u32 get_menu_msg(u16 idx, u16 sub);
u32 menu_row_table_get(void);
u32 menu_row_limit_get(u16 idx);
void menu_top_rows_build(void);
void menu_tab_select(MenuSlot* slot, s32 index);
void menu_slot_entry_b_select(MenuSlot* slot, s32 index);
void menu_sub_row_select(s32 index);
u32 menu_item_input_ok(MenuSlot* slot);
u32 menu_quest_state_ck(void);
u32 menu_busy_ck(void);
u32 menu_slot_busy_ck(u8 idx);
u32 menu_item_frame_update(MenuFrameWork* self);
u32 menu_top_busy_ck(void);
u32 menu_frame_busy_ck(MenuFrameWork* self);
u32 menu_item_list_ready_ck(void);
u32 menu_closing_ck(void);
u32 menu_page_open_ck(s8 value, u8 idx);
void menu_slot0_lock_set(u8 value);
void menu_slot_lock_set(u8 idx, u8 value);
void menu_pad_clear(MenuSlot* slot);
void menu_word_pair_copy(u16* dst, const u16* src);
void menu_split_close(u8 idx);
s32 menu_page_step(MenuSlot* slot);
void menu_main_draw(u32 idx);
void menu_work_update(void);
void menu_sub_list_frame_draw(MenuSlot* slot, u16 frame, const _mh_ivec2_* pos);
void menu_split_panel_draw(u8 idx);
void menu_split_draw(u32 idx);
void menu_split_item_page_draw(MenuSlot* slot);
void menu_split_status_page_draw(MenuSlot* slot);
void menu_split_quit_draw(MenuSlot* slot);
s32 menu_quit_step(MenuSlot* slot);
void menu_quit_draw(MenuSlot* slot);
s32 menu_lobby_exit_step(MenuSlot* slot);
void menu_lobby_exit_draw(MenuSlot* slot);
void menu_item_page_open(MenuSlot* slot);
s32 menu_split_item_page_step(MenuSlot* slot);
void menu_split_top_rows_build(MenuSlot* slot);
void menu_split_update(u8 idx);
void menu_main_open(u32 sound);
void menu_split_item_row_update(u8 idx);
void menu_split_open(u8 idx);
void menu_split_top_row_select(u8 idx, s32 index);
void menu_split_update_all(void);
void menu_split_item_page_open(MenuSlot* slot);
s32 menu_split_status_page_step(MenuSlot* slot);
s32 menu_split_quit_step(MenuSlot* slot);
s32 menu_top_shortcut_ck(MenuSlot* slot, u16 keys);
s32 menu_split_shortcut_ck(MenuSlot* slot, u16 keys);
void menu_pad_read(MenuSlot* slot);
void menu_split_row_select(MenuSlot* slot, s32 index);
void menu_quit_open(MenuSlot* self);
void menu_lobby_exit_open(MenuSlot* self);
/* 0x802A4D98 - fills the item grid's column rows. */
void menu_item_grid_rows_build(MenuSlot* slot);

/* The callees above this unit.  `fn_8027D738` is `Pl/pl_act.cpp`'s; its declaration is the owner's header
 * `Pl/fn_8027D684.h` (rule 2). */
/* 0x80047058 is `pad_connect.cpp`'s (no argument, `Screen_w+0x1A != 0`); it keeps the owner's `s32`, since a
 * `u32` view is `(10505) illegal overloading` once both headers are visible in one TU. */
s32 screen_split_mode_ck(void);
void kbd_close_call(void);
u8 GameMode_ck(void);
u32 move_work_state_ck(void);
u32 game_ready_ck(void);
u32 lb_unlock_cond5_on_ck(void);
void fn_8031A638(MenuSlot* slot);
void ainpc_entry_kill_kind(s32 flag);
void ainpc_page_state_init(void);
void ainpc_page_hold_clear(void);

/* 0x802A2620 / 0x802A26F4 - the two menu-band entries `hud/cockpit_quest.cpp`'s cockpit band
 * (0x802E4978-0x802E7408) calls (rule 2: this range owns the addresses).
 * `menu_slot_anim_update(0)` redraws the menu frame; `menu_slot_panel_draw` is registered with `subTransSetPrio` by
 * address, so it is declared as the function it is.  Both signatures are that consumer's call sites
 * and the bodies are this unit's. */
void menu_slot_anim_update(u8 idx);
void menu_slot_panel_draw(u8 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ITEM_DECLARED */

#ifdef __cplusplus
extern "C" {
#endif
/* 0x802A0568 - clears the item box work (GUESS name). */
void menu_item_work_init(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x802A0188 - closes the item menu (the move-state dispatcher's quest exit). */
void menu_item_close(void);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ITEM_H */
