/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/lb_companion_ui.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_PAGE_ROW_PAIR_GET_H
#define MHTRI_LOBBY_LB_PAGE_ROW_PAIR_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033ADD0 - recipe `index` when it applies, its two ingredients written out (0 when not). */
struct LbComboRecipe* lb_page_row_pair_get(u16 index, u16* item_a, u16* item_b, s32 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PAGE_ROW_PAIR_GET_H */
