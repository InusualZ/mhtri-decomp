/* The item hand-over request and item box records of `ef/eft052.cpp` (rule 1: one definition, included by
 * `ef/eft052_item_get_open.h` and the screens that keep them). */
#ifndef MHTRI_EF_EFT052_BOX_H
#define MHTRI_EF_EFT052_BOX_H

#include "types.h"

/* The request the item hand-over window shows: up to two items with their counts.  size: 0x2C (the block the
 * caller clears) */
typedef struct Eft052ItemGetReq {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ u16 count_0x02;
    /* +0x04 */ u16 item_0x04;
    /* +0x06 */ u16 count_0x06;
    /* +0x08 */ u8 unused_0x08[0x2C - 0x08];
} Eft052ItemGetReq;

/* One cell of the item box: its item and count.  size: 0x4 */
typedef struct Eft052BoxCell {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ s16 count_0x02;
} Eft052BoxCell;

/* Three option texts of the window data (copied as one record).  size: 0xC */
typedef struct Eft052BoxTexts {
    /* +0x00 */ s8* text[3];
} Eft052BoxTexts;

/* The window data the caller hands the item box: the cell grid, the option labels and their help lines.
 * size: 0x20 */
typedef struct Eft052BoxGrid {
    /* +0x00 */ s16 columns_0x00;
    /* +0x02 */ s16 rows_0x02;
    /* +0x04 */ s32 field_0x04;
    /* +0x08 */ Eft052BoxTexts labels_0x08;   /* take, store, leave */
    /* +0x14 */ Eft052BoxTexts help_0x14;     /* the help line of each option */
} Eft052BoxGrid;

/* The item box the window runs (`.bss` 0x806BF310): its state, the menu and cell cursors, the blink frames, the
 * picked flag and cell, the pointed cell, the question's answer, the item total, its cells and sort buffer, and the
 * window data.  size: 0x58 */
typedef struct Eft052ItemBox {
    /* +0x00 */ u8 state_0x00;         /* 0 menu, 1 take, 2 store, 3 leave question, 4 sort question */
    /* +0x01 */ u8 column_0x01;        /* the cell cursor's column */
    /* +0x02 */ u8 menu_0x02;          /* the menu cursor: take, store, leave */
    /* +0x03 */ u8 unused_0x03[0x08 - 0x03];
    /* +0x08 */ u16 frame_0x08;        /* the pointed cell's blink frame */
    /* +0x0A */ u16 frame_0x0A;        /* the picked cell's blink frame */
    /* +0x0C */ u16 picked_0x0C;
    /* +0x0E */ u16 cursor_0x0E;
    /* +0x10 */ u16 hover_0x10;
    /* +0x12 */ s16 row_0x12;          /* the cell cursor's row */
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ s32 confirm_0x18;      /* the question's yes/no state */
    /* +0x1C */ s32 total_0x1C;        /* the summed cell counts */
    /* +0x20 */ u8 unused_0x20[0x24 - 0x20];
    /* +0x24 */ Eft052BoxCell* cells_0x24;
    /* +0x28 */ Eft052BoxCell* sorted_0x28;  /* the sort buffer, NULL without sorting */
    /* +0x2C */ s16 cell_count_0x2C;
    /* +0x2E */ u8 unused_0x2E[0x38 - 0x2E];
    /* +0x38 */ Eft052BoxGrid grid_0x38;
} Eft052ItemBox;

#endif /* MHTRI_EF_EFT052_BOX_H */
