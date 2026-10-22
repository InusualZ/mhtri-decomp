/* menu/menu_scroll.h - the menu band's list-scroll record, shared by `menu/menu_item.h` (the `MenuSlot` that embeds
 *   it) and `menu/movie.h` (the friend picker's work). */
#ifndef MHTRI_MENU_MENU_SCROLL_H
#define MHTRI_MENU_MENU_SCROLL_H

#include "types.h"

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
    /* +0x00A */ union {               /* the option card's two colours, or the equipment page's cursor pair */
        struct {
            /* +0x00A */ s8 light_colour;    /* the option card's light colour */
            /* +0x00B */ s8 dark_colour;     /* its dark colour */
        }; /* size: 0x2 */
        struct {
            /* +0x00A */ s8 equip_page;      /* the equipment page the status page shows */
            /* +0x00B */ s8 equip_page_count; /* its page count (`equip_list_page_count` + 2) */
        }; /* size: 0x2 */
    }; /* size: 0x2 */
}; /* size: 0xC */

#endif /* MHTRI_MENU_MENU_SCROLL_H */
