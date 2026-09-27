/*
 * `menu/menu_item.cpp`'s outbound declarations and the records its bodies read.
 *
 * The unit is the item menu's data/hit layer: the item-record accessors (`ItemName`, `ItemExp`,
 * `GetItemData`), the menu-table accessors (`get_menu_tbl_ptr`, `get_menu_lsp_tbl`) and the hit
 * helpers the menu's attack preview uses (`body_set`, `hit_flag_set`, `hit_result_check`).
 *
 * Two of the records here are this unit's view of a record another unit also views: `_HIT_W` is
 * also defined by `Pl/pl_act.cpp` (its own offsets for the same attack entry).  Folding the two
 * views into one shared definition is a union-aware merge (the fields this unit names sit inside
 * `pl_act.cpp`'s `unk00[0x31]` run), so it is recorded as a rule-1 residual for the unit header.
 *
 * `extern` declarations for symbols no registered unit owns live in `include/unsplit/`; the ones
 * this unit needs are included from there where a band header already carries them.
 */
#ifndef MHTRI_MENU_MENU_ITEM_H
#define MHTRI_MENU_MENU_ITEM_H

#include "types.h"
#include "pl.h"

/* The attack entry the hit helpers below take.  Only the bytes this unit touches are named; the
 * view stops at +0x05B, the last byte any of these functions reads (the record's real end is not
 * pinned by this unit).  4-aligned, so +0x05B leaves the total at 0x5C.
 * size: 0x5C (approximate: max touched offset + 1) */
struct _HIT_W {
    /* +0x000 */ u8 unused_0x000[0x005];
    /* +0x005 */ u8 result_valid;      /* zero means the entry carries no result yet */
    /* +0x006 */ u8 unused_0x006[0x00A - 0x006];
    /* +0x00A */ u16 state;            /* the state bits `fn_8029F5E4`/`fn_8029F5F8` test */
    /* +0x00C */ u8 unused_0x00C[0x00E - 0x00C];
    /* +0x00E */ u8 result;            /* the result id `hit_result_check` reports */
    /* +0x00F */ u8 unused_0x00F[0x018 - 0x00F];
    /* +0x018 */ u16 field_0x018;      /* the four values `fn_8029F4C4` stores */
    /* +0x01A */ u16 field_0x01A;
    /* +0x01C */ u16 field_0x01C;
    /* +0x01E */ u16 field_0x01E;
    /* +0x020 */ u32 flags;            /* the hit flag word the setters OR/ANDC into */
    /* +0x024 */ u8 unused_0x024[0x031 - 0x024];
    /* +0x031 */ u8 field_0x031;       /* the mask byte `fn_8029F57C` tests */
    /* +0x032 */ u8 unused_0x032[0x03C - 0x032];
    /* +0x03C */ f32 field_0x03C;      /* the s16 `fn_8029F5B4` widens and stores */
    /* +0x040 */ u8 unused_0x040[0x05B - 0x040];
    /* +0x05B */ u8 field_0x05B;       /* the mask byte `fn_8029F51C` tests */
};

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
    /* +0x002 */ u8 field_0x002;      /* the mask `fn_8029F73C` ANDs its second argument with */
    /* +0x003 */ u8 unused_0x003;
    /* +0x004 */ u8 tex_idx_0x004;    /* the texture index `draw_itemicon_item_id`/`fn_802E1190`
                                       * (both in `hud/layout.cpp`) hand to `fn_80055C5C`, and the
                                       * byte `fn_802E1024`'s callers read next to `kind`.  Spliced
                                       * out of the `unused_0x003[2]` filler this record carried, one
                                       * byte at +0x004, so no member's offset moved (playbook 56). */
    /* +0x005 */ u8 kind;             /* indexes the .data colour table `lbl_805CDE78` */
    /* +0x006 */ u8 unused_0x006[0x00A - 0x006];
    /* +0x00A */ u16 species;         /* indexes the .data record table `lbl_805DBFB8` */
    /* +0x00C */ u8 unused_0x00C[0x014 - 0x00C];
};

/* The item table head at `.bss:0x806AC8A8` (0x10 B), the block `fn_8029F60C` reads one word of. */
struct ItemWorkHead {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ u32 field_0x004;     /* the item id `fn_8029F60C` hands back */
    /* +0x008 */ u32 unused_0x008;
    /* +0x00C */ u32 unused_0x00C;
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
 * `lbl_806AD698`).  The three accessors below index `lsp_tbl`, `tab_0x66C` and `array_0x758`; the
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
    /* +0x01 */ u8 selected;         /* 1 for the entry the caller's index names, else 0 */
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 unused_0x03[0x008 - 0x003];
    /* +0x08 */ u32 field_0x08;      /* the word `fn_8029FB00` copies in from the menu table */
    /* +0x0C */ u8 unused_0x0C[0x018 - 0x00C];
};

/* One of the two 0x330-byte working records of the menu work area at `.bss:0x806AC8C8`; the area is
 * `MenuSlot slot[2]` (0x660 = the whole `.bss` run, and `fn_802A0188` walks it with an 816-byte
 * stride).  The three entry arrays and their counts are the ones the setters below walk; the other
 * fields are the ones this unit's bodies touch. */
struct MenuSlot {
    /* +0x000 */ u8 field_0x000;      /* the slot's mode/state the predicates below compare */
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 unused_0x003[0x00E - 0x003];
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
    /* +0x01C */ u8 unused_0x01C[0x021 - 0x01C];
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
    /* +0x1A1 */ s8 field_0x1A1;      /* its row (`fn_802A8F14` resolves the height into it) */
    /* +0x1A2 */ u8 field_0x1A2;
    /* +0x1A3 */ u8 field_0x1A3;
    /* +0x1A4 */ u8 unused_0x1A4[0x1B0 - 0x1A4];
    /* +0x1B0 */ u8 field_0x1B0;
    /* +0x1B1 */ u8 unused_0x1B1[0x1EC - 0x1B1];
    /* +0x1EC */ u8 field_0x1EC[0x1F0 - 0x1EC];  /* the embedded selection cursor */
    /* +0x1F0 */ u16 field_0x1F0;    /* the cursor's two item ids */
    /* +0x1F2 */ u16 field_0x1F2;
    /* +0x1F4 */ u8 unused_0x1F4[0x23A - 0x1F4];
    /* +0x23A */ u8 field_0x23A;
    /* +0x23B */ u8 unused_0x23B[0x31E - 0x23B];
    /* +0x31E */ u8 field_0x31E;
    /* +0x31F */ u8 field_0x31F;
    /* +0x320 */ u8 unused_0x320[0x32D - 0x320];
    /* +0x32D */ u8 field_0x32D;      /* the flag `fn_802A0188` clears after `fn_8004082C` */
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

/* The frame work this unit's per-frame entry points (`fn_802A0304`, `fn_802A0404`) carry; only the
 * three bytes they read are named.
 * size: 0x5C0 (approximate: max touched offset + 1) */
struct MenuFrameWork {
    /* +0x000 */ u8 unused_0x000[0x008];
    /* +0x008 */ u8 slot_index;       /* the `MenuSlot` index `fn_802A0304` selects */
    /* +0x009 */ u8 unused_0x009[0x5BC - 0x009];
    /* +0x5BC */ u8 field_0x5BC;
    /* +0x5BD */ u8 unused_0x5BD;
    /* +0x5BE */ u8 field_0x5BE;
};

/* The three `.bss`/`.data` blocks the accessors read.  They are other units' data (no registered
 * unit emits them), so the accessors take their addresses instead of defining them. */
extern ItemWorkHead lbl_806AC8A8;
extern ItemDataHead lbl_806AC8B8;
extern MenuWork lbl_806AC8C8;
extern MenuTables lbl_806ACF28;
extern u32 lbl_805CDE78[];              /* .data:0x805CDE78 - 0x30 B, indexed by `ItemDataRecord::kind` */
extern ItemSpeciesRecord lbl_805DBFB8[]; /* .data:0x805DBFB8 - 132 x 0x0C B */

/* This unit's own entry points and the callees its written bodies call.
 *
 * The callees split three ways by whose header can supply a declaration (rule 2):
 *   * `Pl/fn_8027D684.cpp` (`fn_8027EB18`, `fn_8027E120`, `fn_8027D738`), `mh3_pad.cpp`
 *     (`fn_80047058` - including `mh3_pad.h` here is impossible: it and `ef.h`, which `pl.h` pulls
 *     in for `_HIT_W`'s sibling records, collide in one TU) and `fn_80040598.cpp` (`fn_8004082C`)
 *     have no header that declares these, so the shapes here are this unit's call sites' - the
 *     practice `include/Pl/fn_8028F66C.h` documents for the same situation.
 *   * `ef/fn_800CDB2C.cpp`'s header declares `fn_800CF208` as `u8`, while the retail caller keeps a
 *     `clrlwi` on the widened form (the same per-consumer-view split `include/Pl/pl_act.h`'s
 *     `fn_8027D050` note records), so the declaration here is the call site's 32-bit view.
 *   * the rest (`fn_800D0708`, `fn_8004082C`'s neighbours, `fn_804273EC`, ...) sit in no registered
 *     range, so they are rule 2's unsplit case.
 */
/* This unit's own entry points, in address order, and the callees its written bodies call.
 *
 * The callees split three ways by whose header can supply a declaration (rule 2):
 *   * `Pl/fn_8027D684.cpp` (`fn_8027EB18`, `fn_8027E120`, `fn_8027D738`), `mh3_pad.cpp`
 *     (`fn_80047058`) and `fn_80040598.cpp` (`fn_8004082C`) have no header that declares these, so
 *     the shapes here are this unit's call sites' - the practice `include/Pl/fn_8028F66C.h`
 *     documents for the same situation, with one hard constraint: including `mh3_pad.h` here is
 *     impossible because it and `ef.h` (which `pl.h` pulls in for the records above) collide in one
 *     translation unit.
 *   * `ef/fn_800CDB2C.cpp`'s header declares `fn_800CF208` as `u8`, while the retail caller keeps a
 *     `clrlwi` on the widened form (the per-consumer-view split `include/Pl/pl_act.h`'s
 *     `fn_8027D050` note records), so the declaration here is the call site's 32-bit view.
 *   * the rest (`fn_803AAEC0`, `fn_802FBA60`, `fn_8031A638`, `fn_802DA2D4`, `fn_802DB26C`,
 *     `fn_802DE238`, `fn_802DE670`, `fn_80384380`, `fn_800D0708`, `fn_804273EC`) sit in no
 *     registered range - rule 2's unsplit case.
 *
 * The `fn_*` declarations keep C linkage: the map's names for them are placeholders, not manglings.
 */
#ifndef MHTRI_MENU_MENU_ITEM_DECLARED
#define MHTRI_MENU_MENU_ITEM_DECLARED

/* This unit's own C++-mangled entry points (the map names are these manglings, rule 9).
 * `put_menu_cursor` (0x802A2564) is one of them but has no body yet - it was declared in
 * `include/unsplit/lobby.h` until this range was registered, and its consumer (`lobby/fn_801E7530.cpp`)
 * includes this header for it now (rule 2).  Its third parameter is the lobby band's 2D vector, whose
 * tag is declared here rather than including a band header for it. */
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

void fn_8029F4C4(_HIT_W* hit, u16 a, u16 b, u16 c, u16 d);
u32 fn_8029F51C(_HIT_W* hit, u32 mask);
void fn_8029F538(_HIT_W* hit);
void fn_8029F554(_HIT_W* hit, u32 flags);
u32 fn_8029F564(_HIT_W* hit, u32 flags);
u32 fn_8029F57C(_HIT_W* hit, u32 mask);
void fn_8029F5B4(_HIT_W* hit, s16 value);
u32 fn_8029F5E4(_HIT_W* hit);
u32 fn_8029F5F8(_HIT_W* hit);
u32 fn_8029F60C(void);
void fn_8029F680(u16 id);
ItemSpeciesRecord* fn_8029F6B4(u16 id);
u32 fn_8029F704(u16 id);
s32 fn_8029F73C(u16 id, s32 index);
u32 fn_8029F774(u16 id);
u32 fn_8029F788(u16 kind);
u32 fn_8029F7C4(u16 idx);
u32 fn_8029F7E0(u16 idx, u16 sub);
u32 fn_8029F808(void);
u32 fn_8029F818(u16 idx);
void fn_8029FCFC(void);
void fn_8029FFB8(MenuSlot* slot, s32 index);
void fn_8029FFFC(MenuSlot* slot, s32 index);
void fn_802A0040(s32 index);
u32 fn_802A008C(MenuSlot* slot);
u32 fn_802A0148(void);
u32 fn_802A02CC(void);
u32 fn_802A02D4(u8 idx);
u32 fn_802A0304(MenuFrameWork* self);
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
void fn_8027EB18(u8 kind);
u32 fn_8027E120(_PLW* worker);
/* `fn_8027D738` is `Pl/fn_8027D684.cpp`'s (its address is inside that unit's range) and it is written
 * there, so its declaration is the owner's header `include/Pl/fn_8027D684.h` (rule 2).  The two
 * unwritten siblings above have no owner header entry yet and keep this unit's call-site shape. */
u32 fn_80047058(void);
void fn_8004082C(void);
u8 fn_800CF208(void);
u32 fn_800CF280(void);
u32 fn_800D0708(void);
u32 fn_803AAEC0(void);
u32 fn_802FBA60(void);
void fn_8031A638(MenuSlot* slot);
void fn_802DA2D4(s32 flag);
void fn_802DB26C(void);
void fn_802DE238(void);
u32 fn_802DE670(u8 idx);
void fn_80384380(void);
void fn_804273EC(s32 a, s32 b, s32 c);

/* 0x802A2620 / 0x802A26F4 - the two menu-band entries the cockpit band above this range
 * (`menu/fn_802E4978.cpp`, 0x802E4978-0x802E7408) calls (rule 2: this range owns the addresses).
 * `fn_802A2620(0)` redraws the menu frame; `fn_802A26F4` is registered with `subTransSetPrio` by
 * address, so it is declared as the function it is.  Both signatures are that consumer's call sites
 * (neither body is written yet). */
void fn_802A2620(s32 a);
void fn_802A26F4(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ITEM_DECLARED */

#endif /* MHTRI_MENU_MENU_ITEM_H */
