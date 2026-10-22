/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E7530.cpp`'s `lb_fq_result_items_take`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_FQ_RESULT_ITEMS_TAKE_H
#define MHTRI_LOBBY_LB_FQ_RESULT_ITEMS_TAKE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E91C4 - takes the free-hunt result's items into the user data (get_FqResult_work, item_take) */
void lb_fq_result_items_take(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_FQ_RESULT_ITEMS_TAKE_H */
