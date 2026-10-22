/* userdata_hunter_rank_info_get.h - leaf header for `userdata_hunter_rank_info_get`, which `src/fn_8004CAD8.cpp` defines
 * with C linkage (docs/plan.md 6.5 rule 2).  A leaf because `lobby/lb_quest_board.h` declares the same row with a
 * `void*` first parameter and a unit that included both would see two spellings. */
#ifndef MHTRI_USERDATA_HUNTER_RANK_INFO_GET_H
#define MHTRI_USERDATA_HUNTER_RANK_INFO_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004DF10 - the hunter rank, its points and the points the next rank needs (0 at the top rank). */
void userdata_hunter_rank_info_get(u16* rank, s32* points, s32* next_points);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_HUNTER_RANK_INFO_GET_H */
