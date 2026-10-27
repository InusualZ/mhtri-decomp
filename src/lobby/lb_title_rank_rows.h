/* `lb_title_rank_rows`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_TITLE_RANK_ROWS_H
#define MHTRI_LOBBY_LB_TITLE_RANK_ROWS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The hunter-rank title rows, three halfwords each: the rank a run of titles needs, the run's first title and its length (0xFFFF ends the table). */
extern u16 lb_title_rank_rows[22];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TITLE_RANK_ROWS_H */
