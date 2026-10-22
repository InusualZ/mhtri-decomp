/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s pop_place_item_tbl */
#ifndef MHTRI_MENU_POP_PLACE_ITEM_TBL_H
#define MHTRI_MENU_POP_PLACE_ITEM_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794C60 (.sbss) - per map kind, the pop-file points' drop tables by item index (GUESS name). */
struct EmWeightEntry;
extern const struct EmWeightEntry*** pop_place_item_tbl;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_POP_PLACE_ITEM_TBL_H */
