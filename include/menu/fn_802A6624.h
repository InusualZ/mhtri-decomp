/*
 * The unit's own view of the menu list work the first (0x802A6624-0x802A7CC8) block operates on,
 * plus the message/frame entry points' declarations.
 *
 * The block is the menu's list/cursor layer: `fn_802A674C` fills the list at +0x1BE from one of two
 * source tables and `fn_802A6A64`/`fn_802A6B6C` step the cursor over it.  The record is the same
 * object `menu/menu_item.h` views as `MenuSlot` (0x330 B): the kind byte at +0x0F and the count at
 * +0x16 are that struct's `field_0x00F`/`entry_count_a`, and the list/name fields here sit inside
 * `MenuSlot`'s untraced +0x19C..+0x23A run.  The two views are folded into one definition by a later
 * pass, not here (the same rule-1 residual `menu_item.h` records for `_HIT_W`), because the sibling
 * unit's header is the natural home and this range only needs the offsets it reads.
 */
#ifndef MHTRI_MENU_FN_802A6624_H
#define MHTRI_MENU_FN_802A6624_H

#include "types.h"

/* One record of the item-record table `lbl_806BE340` (two blocks of five, 0x130 B each): a validity
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

/* The 2D integer position the menu/HUD helpers exchange.  Same record as `include/unsplit/lobby.h`'s
 * `_mh_ivec2_` (0x4 B); named locally so the calls can keep the map's `...P10_mh_ivec2_` mangling
 * (via `struct _mh_ivec2_` forward declarations) without including that header, whose
 * `menu_cursor_step` declaration this range owns - see the unit header's rule-2 note.
 * size: 0x4 */
typedef struct MenuLspPos {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} MenuLspPos;

/* One 0x18-byte entry of the list block `fn_802A6C28`/`fn_802A6DB4` build before they hand it to the
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
    /* +0x010 */ u8 unused_0x010[0x016 - 0x010];
    /* +0x016 */ u8 count_0x016;      /* the list count `fn_802A66BC` recomputes */
    /* +0x017 */ u8 unused_0x017[0x1BB - 0x017];
    /* +0x1BB */ u8 list_count_0x1BB; /* the count `fn_802A674C` fills in */
    /* +0x1BC */ u8 unused_0x1BC[0x1BE - 0x1BC];
    /* +0x1BE */ s8 list_0x1BE[0x0A]; /* the ten list ids `fn_802A674C` writes */
    /* +0x1C8 */ char short_name_0x1C8[0x0A];
    /* +0x1D2 */ char long_name_0x1D2[0x0E];
};

/* The range's own entry points the neighbouring units call.  `include/unsplit/lobby.h` and
 * `include/lobby/fn_801F3294.h` published them while the band had no registered unit; this range now
 * owns 0x802A6624-0x802AD9C0, so the declarations live here and both headers include this one
 * (docs/plan.md 6.5 rule 2).  The spellings are this range's own definitions' (the `s32` first two
 * parameters are what the retail call sites need: a narrow argument must not be narrowed back to `s16`
 * for the call).  `fn_802A8F50` is unwritten - its callers spell it as below. */
#ifdef __cplusplus
extern "C" {
#endif

void fn_802A7C04(u16 id, u16* item);
s32 fn_802A8EC0(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f);
s32 fn_802A8ED8(s32 a, s32 b, u16 c, u16 d, u16 e, s32 f);
s32 menu_cursor_step(s32 a, s32 b, u16 c, u16 d, u16 e);
s32 fn_802A8F50(void* state, u16 pad, s32 a, s32 b, s32 c);

#ifdef __cplusplus
}

/* 0x802AA3EC - the `bool` row's colour, the front-end's spelling of the map's
 * `GetMenuFontColor__Fbbbb` (rule 9). */
s32 GetMenuFontColor(bool a, bool b, bool c, bool d);
#endif

#endif /* MHTRI_MENU_FN_802A6624_H */
