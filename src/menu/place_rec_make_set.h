/* `place_rec_make_set`, defined by `menu/menu_row.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_PLACE_REC_MAKE_SET_H
#define MHTRI_MENU_PLACE_REC_MAKE_SET_H

#include "types.h"

struct PlaceRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034D124 / 0x8034D150 / 0x8034D17C - fill a help record of kind 1 (a make recipe), 2 (an upgrade recipe) and 3
 * (a material); each returns its kind. */
s32 place_rec_make_set(struct PlaceRec* rec, u16 a, u16 b, u16 item, u16 count);
s32 place_rec_upgrade_set(struct PlaceRec* rec, u16 a, u16 b, u16 item, u16 count);
s32 place_rec_material_set(struct PlaceRec* rec, u16 a, u16 item, u16 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACE_REC_MAKE_SET_H */
