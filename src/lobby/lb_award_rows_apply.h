/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80219260.cpp`'s `lb_award_rows_apply`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_AWARD_ROWS_APPLY_H
#define MHTRI_LOBBY_LB_AWARD_ROWS_APPLY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021B690 - applies the award rows tables to the user data */
void lb_award_rows_apply(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_AWARD_ROWS_APPLY_H */
