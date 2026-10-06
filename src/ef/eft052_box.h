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

/* The item box the window runs (`.bss` 0x806BF310): its state, the blink frames, the picked flag, the cursor and the
 * pointed cell, and its cells.  size: 0x58 */
typedef struct Eft052ItemBox {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 unused_0x01[0x08 - 0x01];
    /* +0x08 */ u16 frame_0x08;        /* the pointed cell's blink frame */
    /* +0x0A */ u16 frame_0x0A;        /* the picked cell's blink frame */
    /* +0x0C */ u16 picked_0x0C;
    /* +0x0E */ u16 cursor_0x0E;
    /* +0x10 */ u16 hover_0x10;
    /* +0x12 */ u8 unused_0x12[0x24 - 0x12];
    /* +0x24 */ Eft052BoxCell* cells_0x24;
    /* +0x28 */ u8 unused_0x28[0x58 - 0x28];
} Eft052ItemBox;

#endif /* MHTRI_EF_EFT052_BOX_H */
