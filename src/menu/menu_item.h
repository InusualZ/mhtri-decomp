/*
 * menu/menu_item.h - `menu/menu_item.cpp`'s outbound declarations and the records its bodies read: the item-record
 *   accessors (`ItemName`, `ItemExp`, `GetItemData`), the menu tables (`get_menu_tbl_ptr`, `get_menu_lsp_tbl`), the hit
 *   helpers (`body_set`, `hit_flag_set`, `hit_result_check`; `_HIT_W` is `Pl/hit_w.h`'s) and the menu slot `MenuSlot`.
 */
#ifndef MHTRI_MENU_MENU_ITEM_H
#define MHTRI_MENU_MENU_ITEM_H

#include "types.h"
#include "pl.h"
#include "Pl/hit_w.h"   /* `_HIT_W` / `HitRegistry`, shared with `Pl/pl_coll.cpp` (rule 1) */


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
                          * (`fn_802A5E64`/`fn_802A64B0`) reads it as `kind_0x00`/`level_0x01` and the
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
    /* +0x004 */ u8 tex_idx_0x004;    /* the texture index `draw_itemicon_item_id`/`fn_802E1190`
                                       * (both in `hud/layout.cpp`) hand to `fn_80055C5C`, and the
                                       * byte `fn_802E1024`'s callers read next to `kind`.  Spliced
                                       * out of the `unused_0x003[2]` filler this record carried, one
                                       * byte at +0x004, so no member's offset moved (playbook 56). */
    /* +0x005 */ u8 kind;             /* indexes the .data colour table `lbl_805CDE78` */
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
 * `fn_8029F680` passes on. */
struct ItemSpeciesRecord {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ u32 unused_0x004;
    /* +0x008 */ u8 kind;
    /* +0x009 */ u8 unused_0x009[0x00C - 0x009];
};

/* One 0x18-byte menu entry: the three entry arrays inside a `MenuSlot` are made of these, and the
 * flag at +0x01 is the selection the setters below write.  Offsets from `fn_8029FA74`'s
 * `memset(self + 36, 0, 72)` (three entries at +0x24) and the 24-byte stride of the loops. */
struct MenuEntry {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ s8 selected;         /* 1 for the entry the caller's index names, else 0 */
    /* +0x02 */ s8 field_0x02;       /* nonzero when the row may be picked */
    /* +0x03 */ u8 unused_0x03[0x006 - 0x003];
    /* +0x06 */ s16 field_0x06;      /* the inventory row the entry draws (the kind tables' index) */
    /* +0x08 */ u32 field_0x08;      /* the word `fn_8029FB00` copies in from the menu table */
    /* +0x0C */ u32 field_0x0C;      /* cleared by the item page's row fill */
    /* +0x10 */ u8 unused_0x10[0x018 - 0x010];
};

/* The 0x0C-byte list-scroll work record embedded in a `MenuSlot` at +0x1A4: the two cursors (`row`,
 * `col`) the menu band's scroller (`menu_scroll_step`) moves by a button word, clamped against the row
 * and column counts, plus the `move_flags` word it sets for the frame and the two option-card
 * colours.  `menu/menu_item_page.cpp` is the record's own reader (`item_page_fill_rows` reads the counts and
 * the cursor pair); the fields it does not touch keep their offsets.
 * size: 0x0C (its own byte run: `MenuSlot`'s +0x1A4 pad and this record's +0x0C end agree) */
struct MenuScroll {
    /* +0x000 */ u16 index;          /* the flat entry index: `row + rows_per_page * col` */
    /* +0x002 */ u8 rows_per_page;    /* the row count `menu_scroll_step` clamps `row` against */
    /* +0x003 */ u8 row;             /* the row cursor */
    /* +0x004 */ u8 col;             /* the column cursor */
    /* +0x005 */ u8 row_limit;       /* the row count the cursor is bounded by */
    /* +0x006 */ u8 col_count;       /* the column count `col` wraps at */
    /* +0x007 */ u8 field_0x007;     /* `menu_scroll_step` copies it into `row_limit` at the last column */
    /* +0x008 */ u16 move_flags;     /* the move bits `menu_scroll_step` ORs in (1/2/4/8) */
    /* +0x00A */ s8 light_colour;    /* the option card's light colour */
    /* +0x00B */ s8 dark_colour;     /* its dark colour */
};

struct _mh_ivec2_;
/* The 0x24-byte placement record `MenuSlot::place_entries` points at: `menu/menu_row.cpp` is its
 * only consumer, so its definition lives in that unit and this header only names the pointer's type. */
struct MenuPlaceRec;

/* One of the two 0x330-byte working records of the menu work area at `.bss:0x806AC8C8`; the area is
 * `MenuSlot slot[2]` (0x660 = the whole `.bss` run, and `fn_802A0188` walks it with an 816-byte
 * stride).  The three entry arrays and their counts are the ones the setters below walk; the other
 * fields are the ones this unit's bodies touch.  The item page's draw layer
 * (`menu/menu_item_page.cpp`) named the bytes it introduced: the page index at +0x01C, the per-kind
 * selected-row offsets at +0x01D, the embedded `MenuScroll` at +0x1A4 and the per-kind row base at
 * +0x2ED. */
struct MenuSlot {
    /* +0x000 */ u8 field_0x000;      /* the slot's mode/state the predicates below compare */
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 unused_0x003[0x008 - 0x003];
    /* +0x008 */ u16 field_0x008;     /* the option card's arrow word (`item_page_draw_closed_column`'s `arg5`) */
    /* +0x00A */ u8 unused_0x00A[0x00E - 0x00A];
    /* +0x00E */ u8 active;           /* the "slot in use" flag every predicate below tests */
    /* +0x00F */ u8 field_0x00F;      /* 1 selects the `fn_802A008C` arm that reads `worker` */
    /* +0x010 */ u8 field_0x010;
    /* +0x011 */ u8 field_0x011;
    /* +0x012 */ u8 unused_0x012[0x014 - 0x012];
    /* +0x014 */ s8 field_0x014;      /* the value `fn_802A04EC`/`fn_802A0464` compare */
    /* +0x015 */ u8 field_0x015;
    /* +0x016 */ u8 entry_count_a;    /* `entries_a`'s count (`fn_8029FFB8`) */
    /* +0x017 */ u8 entry_count_b;    /* `entries_b`'s count (`fn_8029FFFC`, `fn_802A3190`) */
    /* +0x018 */ u8 entry_count_c;    /* `entries_c`'s count (`fn_8029FB00`, `fn_802A0040`) */
    /* +0x019 */ u8 field_0x019;
    /* +0x01A */ u8 field_0x01A;
    /* +0x01B */ u8 field_0x01B;
    /* +0x01C */ s8 page_index;       /* 0..3: the page the detail draw switches on */
    /* +0x01D */ u8 field_0x01D;      /* kind 1's selected row offset (the page-0 blend index) */
    /* +0x01E */ u8 field_0x01E;      /* kind 2's */
    /* +0x01F */ u8 field_0x01F;      /* kind 3's */
    /* +0x020 */ u8 unused_0x020[0x021 - 0x020];
    /* +0x021 */ u8 field_0x021;      /* the byte `fn_802A053C`/`fn_802A054C` set */
    /* +0x022 */ u8 unused_0x022[0x024 - 0x022];
    /* +0x024 */ MenuEntry entries_a[3];
    /* +0x06C */ MenuEntry entries_b[8];
    /* +0x12C */ MenuEntry entries_c[4];
    /* +0x18C */ u32 field_0x18C;     /* set to 1 by `fn_802A441C`/`fn_802A47F4` */
    /* +0x190 */ _PLW* worker;        /* the player work the slot displays */
    /* +0x194 */ u32 field_0x194;
    /* +0x198 */ u32 field_0x198;
    /* +0x19C */ u8 unused_0x19C[0x19E - 0x19C];
    /* +0x19E */ u16 field_0x19E;     /* the value `menu/menu_item.cpp`'s view calls `field_0x19E` */
    /* +0x1A0 */ u8 field_0x1A0;      /* the column the equipment panel puts the cursor in */
    /* +0x1A1 */ s8 field_0x1A1;      /* its row (`menu_page_count` resolves the height into it) */
    /* +0x1A2 */ u8 field_0x1A2;
    /* +0x1A3 */ u8 field_0x1A3;
    /* +0x1A4 */ MenuScroll scroll;   /* the list-scroll cursors `menu_scroll_step` moves */
    /* +0x1B0 */ u8 field_0x1B0;
    /* +0x1B1 */ u8 unused_0x1B1[0x1EC - 0x1B1];
    /* +0x1EC */ u8 field_0x1EC[0x1F0 - 0x1EC];  /* the embedded selection cursor */
    /* +0x1F0 */ u16 field_0x1F0;    /* the cursor's two item ids */
    /* +0x1F2 */ u16 field_0x1F2;
    /* +0x1F4 */ u8 unused_0x1F4[0x23A - 0x1F4];
    /* +0x23A */ u8 field_0x23A;     /* the sprite id every draw of the item page takes */
    /* +0x23B */ u8 field_0x23B;     /* 0 = the page draws no detail panel */
    /* +0x23C */ u8 field_0x23C;     /* the blend colour the item page's rows are handed */
    /* +0x23D */ u8 unused_0x23D[0x2ED - 0x23D];
    /* +0x2ED */ u8 kind_row_base[8]; /* `kind_row_base[kind * 2]`: the first row the item kind shows */
    /* +0x2F5 */ u8 field_0x2F5;     /* the item kind (0..3) the page is showing */
    /* +0x2F6 */ u8 unused_0x2F6[0x31E - 0x2F6];
    /* +0x31E */ u8 field_0x31E;
    /* +0x31F */ u8 field_0x31F;
    /* +0x320 */ u8 unused_0x320[0x4];
    /* +0x324 */ MenuPlaceRec* place_entries; /* the 0x24-byte placement list `menu/menu_row.cpp` walks */
    /* +0x328 */ s32 place_count;
    /* +0x32C */ u8 unused_0x32C[0x1];
    /* +0x32D */ u8 field_0x32D;      /* the flag `fn_802A0188` clears after `kbd_close_call` */
    /* +0x32E */ u8 unused_0x32E[0x330 - 0x32E];
};

/* The menu work area itself: the two slots, `.bss` 0x806AC8C8..0x806ACF28 (0x660 B - exactly two
 * 0x330-byte slots, which is what `fn_802A0188`'s 816-byte stride walks). */
struct MenuWork {
    /* +0x000 */ MenuSlot slot[2];
};

/* The small per-entry state record `fn_802A16F8` clears from +0x04 and `fn_802A2550` copies the
 * first pair of.  Its owner is unclaimed, so the name is descriptive.
 * size: 0x0E (approximate: max touched offset + 1) */
struct MenuEntryState {
    /* +0x000 */ u16 field_0x000;
    /* +0x002 */ u16 field_0x002;
    /* +0x004 */ u16 field_0x004;
    /* +0x006 */ u16 field_0x006;
    /* +0x008 */ u16 field_0x008;
    /* +0x00A */ u16 field_0x00A;
    /* +0x00C */ u16 field_0x00C;
};

/* The frame work this unit's per-frame entry points (`menu_item_frame_update`, `fn_802A0404`) carry; only the
 * three bytes they read are named.
 * size: 0x5C0 (approximate: max touched offset + 1) */
struct MenuFrameWork {
    /* +0x000 */ u8 unused_0x000[0x008];
    /* +0x008 */ u8 slot_index;       /* the `MenuSlot` index `menu_item_frame_update` selects */
    /* +0x009 */ u8 unused_0x009[0x5BC - 0x009];
    /* +0x5BC */ u8 field_0x5BC;
    /* +0x5BD */ u8 unused_0x5BD;
    /* +0x5BE */ u8 field_0x5BE;
};

/* The `.bss`/`.data` blocks the accessors read: this unit's own sections, declared never defined (playbook 29),
 * so the accessors take their addresses instead of defining them. */
extern HitRegistry lbl_806AC8A8;
extern ItemDataHead lbl_806AC8B8;
extern MenuWork menu_work;
extern MenuTables lbl_806ACF28;
extern u32 lbl_805CDE78[];              /* .data:0x805CDE78 - 0x30 B, indexed by `ItemDataRecord::kind` */
extern ItemSpeciesRecord lbl_805DBFB8[]; /* .data:0x805DBFB8 - 132 x 0x0C B */

/* This unit's own entry points, in address order, and the callees its written bodies call.
 *
 * The callees split three ways by whose header can supply a declaration (rule 2):
 *   * `Pl/pl_act.cpp` (`fn_8027EB18`, `fn_8027E120`), `pad_connect.cpp` (`screen_split_mode_ck`) and
 *     `fn_80040598.cpp` (`kbd_close_call`) have no header that declares these, so the shapes here are this unit's
 *     call sites'; `mh3_pad.h` cannot be included here because it and `ef.h` (which `pl.h` pulls in) collide.
 *   * `GameMode_ck` keeps its owner's (`ef/fn_800CDB2C.h`) `u8` spelling: a `u32` view is `(10505) illegal
 *     overloading` beside the owner's header.
 *   * the rest (`fn_802FBA60`, `fn_8031A638`, `fn_802DA2D4`, `fn_802DB26C`,
 *     `fn_802DE238`, `fn_802DE670`, `fn_80384380`, `game_ready_ck`, `fn_804273EC`) are declared in their call
 *     sites' shapes; their owners' headers do not declare them yet.
 *
 * The `fn_*` declarations keep C linkage: the map's names for them are placeholders, not manglings.
 */
#ifndef MHTRI_MENU_MENU_ITEM_DECLARED
#define MHTRI_MENU_MENU_ITEM_DECLARED

/* This unit's own C++-mangled entry points (the map names are these manglings, rule 9).
 * `put_menu_cursor` (0x802A2564) is one of them but has no body yet; its consumer
 * (`lobby/fn_801E7530.cpp`) includes this header for it (rule 2).  Its third parameter is the lobby band's
 * 2D vector, whose tag is declared here rather than including a band header for it. */
struct _mh_ivec2_;
/* The 0x24-byte placement record `MenuSlot::place_entries` points at: `menu/menu_row.cpp` is its
 * only consumer, so its definition lives in that unit and this header only names the pointer's type. */
struct MenuPlaceRec;

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

void fn_8029F4C4(_HIT_W* hit, u16 a, u16 b, u16 c, u16 d);
u32 hit_mask_ck(_HIT_W* hit, u32 mask);
void hit_flags_clear(_HIT_W* hit);
void fn_8029F554(_HIT_W* hit, u32 flags);
u32 fn_8029F564(_HIT_W* hit, u32 flags);
u32 fn_8029F57C(_HIT_W* hit, u32 mask);
void hit_knock_set(_HIT_W* hit, s16 value);
u32 fn_8029F5E4(_HIT_W* hit);
u32 fn_8029F5F8(_HIT_W* hit);
u32 fn_8029F60C(void);
void fn_8029F680(u16 id);
ItemSpeciesRecord* fn_8029F6B4(u16 id);
u32 fn_8029F704(u16 id);
s32 item_category_ck(u16 id, s32 index);
u32 fn_8029F774(u16 id);
u32 fn_8029F788(u16 kind);
u32 fn_8029F7C4(u16 idx);
u32 fn_8029F7E0(u16 idx, u16 sub);
u32 menu_row_table_get(void);
u32 fn_8029F818(u16 idx);
void fn_8029FCFC(void);
void fn_8029FFB8(MenuSlot* slot, s32 index);
void fn_8029FFFC(MenuSlot* slot, s32 index);
void fn_802A0040(s32 index);
u32 fn_802A008C(MenuSlot* slot);
u32 fn_802A0148(void);
u32 menu_busy_ck(void);
u32 fn_802A02D4(u8 idx);
u32 menu_item_frame_update(MenuFrameWork* self);
u32 fn_802A03A4(void);
u32 fn_802A0404(MenuFrameWork* self);
u32 fn_802A0464(void);
u32 fn_802A04B0(void);
u32 fn_802A04EC(s8 value, u8 idx);
void fn_802A053C(u8 value);
void fn_802A054C(u8 idx, u8 value);
void fn_802A16F8(MenuEntryState* state);
void fn_802A2550(MenuEntryState* dst, const MenuEntryState* src);
void fn_802A2C98(u8 idx);
void fn_802A3190(MenuSlot* slot, s32 index);
void fn_802A441C(MenuSlot* self);
void fn_802A47F4(MenuSlot* self);
/* 0x802A4D98 - one of the range's own symbols and still unwritten.  The folded half of this unit
 * calls it (`fn_802A598C`'s tail) and ignores the result; retail passes the record pointer with no
 * extension (`mr r3,r29; bl fn_802A4D98`). */
void fn_802A4D98(MenuSlot* slot);

/* The callees above this unit. */
/* `fn_8027D738` is `Pl/pl_act.cpp`'s (its address is inside that unit's range) and it is written
 * there, so its declaration is the owner's header `Pl/fn_8027D684.h` (rule 2).  The two
 * unwritten siblings above have no owner header entry yet and keep this unit's call-site shape. */
/* 0x80047058 is `pad_connect.cpp`'s (no argument, `Screen_w+0x1A != 0`); it keeps the owner's `s32`, since a
 * `u32` view is `(10505) illegal overloading` once both headers are visible in one TU. */
s32 screen_split_mode_ck(void);
void kbd_close_call(void);
u8 GameMode_ck(void);
u32 move_work_state_ck(void);
u32 game_ready_ck(void);
u32 fn_802FBA60(void);
void fn_8031A638(MenuSlot* slot);
void fn_802DA2D4(s32 flag);
void fn_802DB26C(void);
void fn_802DE238(void);
u32 fn_802DE670(u8 idx);
void fn_80384380(void);
void fn_804273EC(s32 a, s32 b, s32 c);

/* 0x802A2620 / 0x802A26F4 - the two menu-band entries `hud/cockpit_quest.cpp`'s cockpit band
 * (0x802E4978-0x802E7408) calls (rule 2: this range owns the addresses).
 * `fn_802A2620(0)` redraws the menu frame; `menu_slot_panel_draw` is registered with `subTransSetPrio` by
 * address, so it is declared as the function it is.  Both signatures are that consumer's call sites
 * (neither body is written yet). */
void fn_802A2620(s32 a);
void menu_slot_panel_draw(void);

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

#endif /* MHTRI_MENU_MENU_ITEM_H */
