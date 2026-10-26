/* ef/cockpit_hold_entry.h - the item-hold types and block `ef/eft052.cpp` defines and its callers build (rule 1: one
 * definition; `menu/get_pop_dat_ptr.cpp` builds one on the stack for `eft052_hold_entry_set`). */
#ifndef MHTRI_EF_COCKPIT_HOLD_ENTRY_H
#define MHTRI_EF_COCKPIT_HOLD_ENTRY_H

#include "types.h"

/* One item-hold entry: the strip's help lines, its item list and kind. */
struct CockpitHoldEntry {
    /* +0x00 */ s32 limit_0x00;         /* the most one pick takes; 0 takes one without a count step */
    /* +0x04 */ s16 msg_0x04;           /* the browse help line, -1 none */
    /* +0x06 */ s16 msg2_0x06;          /* its second line, -1 none */
    /* +0x08 */ s16 title_0x08;         /* the empty strip's line */
    /* +0x0A */ s16 count_msg_0x0A;     /* the count step's help line, -1 none */
    /* +0x0C */ s16 item_count_0x0C;    /* the length of the item list */
    /* +0x0E */ u16 sprite_0x0E;        /* the strip's title sprite */
    /* +0x10 */ u16 highlight_0x10;     /* the item drawn marked, 0 none */
    /* +0x12 */ u16 pad_0x12;
    /* +0x14 */ u16* items_0x14;        /* the item list */
    /* +0x18 */ u8 kind_0x18;           /* 0 owned items, 1 priced items, 2 box room */
    /* +0x19 */ u8 quiet_0x19;          /* no pick sound */
    /* +0x1A */ u16 pad_0x1A;
}; /* size: 0x1C */

/* One row of the hold strip's page.  size: 0x4 */
struct CockpitHoldRow {
    /* +0x00 */ u16 id_0x00;      /* the row's item */
    /* +0x02 */ s16 count_0x02;   /* the count it shows (owned, or the price for kind 1) */
}; /* size: 0x4 */

/* The cockpit hold block at `.bss:0x806BF368` (0x58 B), in `ef/eft052.cpp`'s `.bss` claim (0x806BF310-0x806BF3C0):
 * the row cursor `eft052_hold_cursor_step` advances, the per-row table `eft052_hold_row_get` reads and the
 * current-entry copy `eft052_hold_entry_copy` fills. */
struct CockpitHoldBlock {
    /* +0x00 */ u8 mode_0x00;           /* 0 browse, 1 count step */
    /* +0x01 */ u8 row_0x01;            /* the cursor row of the page */
    /* +0x02 */ s16 page_0x02;          /* the shown page */
    /* +0x04 */ s16 pages_0x04;         /* the page count (at least 1) */
    /* +0x06 */ u16 moved_0x06;         /* the cursor helpers' moved flags */
    /* +0x08 */ s16 rows_0x08;          /* the rows on the page */
    /* +0x0A */ s16 amount_0x0A;        /* the picked count */
    /* +0x0C */ s32 max_0x0C;           /* the count step's limit */
    /* +0x10 */ CockpitHoldRow rows_0x10[8];
    /* +0x30 */ u8 pad_0x30[0x8];
    /* +0x38 */ CockpitHoldEntry entry_0x38;
    /* +0x54 */ u8 pad_0x54[0x4];
}; /* size: 0x58 */

#endif /* MHTRI_EF_COCKPIT_HOLD_ENTRY_H */
