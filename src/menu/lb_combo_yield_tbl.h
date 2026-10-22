/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `menu/menu_placeinfo.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_LB_COMBO_YIELD_TBL_H
#define MHTRI_MENU_LB_COMBO_YIELD_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8060461C - the combine yields by recipe group.  GUESS name. */
extern u8 lb_combo_yield_tbl[12];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_COMBO_YIELD_TBL_H */
