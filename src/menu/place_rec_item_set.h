/* `place_rec_item_set` and `place_rec_zenny_set`, defined by `menu/menu_row.cpp` (leaf header, rule 2; the record type
 * is the owner's `menu/menu_row.h`).  C linkage (the map rows are plain names). */
#ifndef MHTRI_MENU_PLACE_REC_ITEM_SET_H
#define MHTRI_MENU_PLACE_REC_ITEM_SET_H

#include "types.h"
#include "menu/menu_row.h"   /* PlaceRec */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034D1A8 / 0x8034D1D4 - fill a help record of kind 4 (an item cost) / kind 5 (a zenny cost) and return the kind.
 * GUESS names. */
s32 place_rec_item_set(PlaceRec* rec, u16 source, u16 item, u16 count);
s32 place_rec_zenny_set(PlaceRec* rec, u16 source, u16 item, u16 amount);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACE_REC_ITEM_SET_H */
