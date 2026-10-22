/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s pop_spare_item_tbl */
#ifndef MHTRI_MENU_POP_SPARE_ITEM_TBL_H
#define MHTRI_MENU_POP_SPARE_ITEM_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794C64 (.sbss) - per map kind, the spare points' drop tables by item index (GUESS name). */
struct EmWeightEntry;
extern const struct EmWeightEntry*** pop_spare_item_tbl;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_POP_SPARE_ITEM_TBL_H */
