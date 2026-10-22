/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80219260.cpp`'s `lb_award_check_all`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_AWARD_CHECK_ALL_H
#define MHTRI_LOBBY_LB_AWARD_CHECK_ALL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021B144 - checks the award conditions and sets the award bits (userdata_award_bit_set) */
void lb_award_check_all(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_AWARD_CHECK_ALL_H */
