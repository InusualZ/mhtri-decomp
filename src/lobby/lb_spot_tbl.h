/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_menu_scratch.cpp`'s two lobby spot tables, the declarations
 * `lobby/lb_npc.cpp`'s spot search needs.  GUESS names (registry). */
#ifndef MHTRI_LOBBY_LB_SPOT_TBL_H
#define MHTRI_LOBBY_LB_SPOT_TBL_H

#include "types.h"

struct LbSpot;   /* `lobby/lb_npc.h` */

/* 0x805BA490 - the online lobby spots, one list per area. */
extern struct LbSpot* lb_spot_net_tbl[];
/* 0x805BA530 - the offline lobby spots, one list per area. */
extern struct LbSpot* lb_spot_tbl[];

#endif /* MHTRI_LOBBY_LB_SPOT_TBL_H */
